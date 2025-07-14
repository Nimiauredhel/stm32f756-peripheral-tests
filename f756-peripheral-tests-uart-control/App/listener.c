/*
 * eth_server.c
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */

#include "server_common.h"
#include "listener.h"

extern struct netif gnetif;
extern osMessageQueueId_t TestQueueHandle;
extern osMessageQueueId_t OutboxQueueHandle;

static uint16_t next_test_id_server_half = 0;

static struct netconn *listener_conn = NULL;

static TestRequest_t request_scratch = {0};
static OutgoingMessage_t message_scratch = {0};

void test_listener_task_init(void)
{
	serial_debug_enqueue("Ethernet Listener Task started.");

	explicit_bzero(&request_scratch, sizeof(request_scratch));

	listener_conn = netconn_new(NETCONN_UDP);

	if (listener_conn == NULL)
	{
		serial_debug_enqueue("Failed to create listener connection.");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}

	serial_debug_enqueue("Waiting for valid IP address.");

	while(ip4_addr_isany_val(*netif_ip4_addr(&gnetif)))
	{
		vTaskDelay(pdMS_TO_TICKS(500));
	}

	serial_debug_enqueue("Server IP acquired.");
	serial_debug_enqueue(ip4addr_ntoa(netif_ip4_addr(&gnetif)));

	if (ERR_OK != netconn_bind(listener_conn, netif_ip4_addr(&gnetif), SERVER_PORT))
	{
		serial_debug_enqueue("Failed to bind listener connection.");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}
	else
	{
		serial_debug_enqueue("Listener bound to local address.");
	}

	netconn_set_recvtimeout(listener_conn, 3000);
}

void test_listener_task_loop(void)
{
	static struct netbuf *listener_netbuf = NULL;
	static uint8_t *listener_pbuf = NULL;
	static uint16_t listener_pbuf_len = 0;

	static char debug_buff[256] = {0};


	/* Infinite loop */
	for(;;)
	{
		serial_debug_enqueue("Awaiting Requests.");

		if (ERR_OK == netconn_recv(listener_conn, &listener_netbuf))
		{
			netbuf_data(listener_netbuf, (void **)&listener_pbuf, &listener_pbuf_len);

			if (listener_pbuf[0] == TEST_PACKET_START_BYTE_VALUE
				&& (TestPacketMsg_t)listener_pbuf[TEST_PACKET_MSG_BYTE_OFFSET] == TESTMSG_TEST_NEW_REQUEST)
			{
				switch((TestPacketMsg_t)listener_pbuf[TEST_PACKET_MSG_BYTE_OFFSET])
				{
				case TESTMSG_TEST_NEW_REQUEST:
					// save request to scratch buffer
					bzero(&request_scratch, sizeof(request_scratch));
					request_scratch.client_addr = listener_netbuf->addr;
					request_scratch.client_port = listener_netbuf->port;
					memcpy(request_scratch.request, listener_pbuf, listener_pbuf_len);
					netbuf_delete(listener_netbuf);

					request_scratch.request[TEST_PACKET_ID_BYTE_OFFSET] |= (uint32_t)next_test_id_server_half;
					next_test_id_server_half = (next_test_id_server_half == UINT16_MAX) ? 0 : next_test_id_server_half + 1;

					snprintf(debug_buff, sizeof(debug_buff), "\r\nDevice received test string: %s", request_scratch.request+TEST_PACKET_STRING_HEAD_OFFSET);
					serial_debug_enqueue(debug_buff);

					// forward request to test queue
					bool forwarded = (osOK == osMessageQueuePut(TestQueueHandle, &request_scratch, 0, pdMS_TO_TICKS(1000)));

					// confirm reception
					bzero(&message_scratch, sizeof(message_scratch));
					message_scratch.addr = request_scratch.client_addr;
					message_scratch.port = request_scratch.client_port;
					message_scratch.message[0] = TEST_PACKET_START_BYTE_VALUE;
					*(uint32_t *)(message_scratch.message+TEST_PACKET_ID_BYTE_OFFSET)
					= *(uint32_t *)(request_scratch.request+TEST_PACKET_ID_BYTE_OFFSET);
					message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_NEW_ACK;
					message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = forwarded ? 1 : 0;
					message_scratch.message[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
					osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, pdMS_TO_TICKS(1000));
					break;
				case TESTMSG_PAIRING_PROBE:
					serial_debug_enqueue("Received a client probe packet.");
					// send out a beacon
					uint32_t broadcast = (~gnetif.netmask.addr)|gnetif.ip_addr.addr;
					bzero(&message_scratch, sizeof(message_scratch));
					message_scratch.port = SERVER_PORT;
					message_scratch.addr.addr = broadcast;
					message_scratch.message[0] = TEST_PACKET_START_BYTE_VALUE;
					message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_PAIRING_BEACON;
					message_scratch.message[TEST_PACKET_ID_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
					osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, pdMS_TO_TICKS(1000));
					break;
				default:
					serial_debug_enqueue("Received unexpected packet.");
					break;
				}
		    }
	  }
	  else
	  {
		  serial_debug_enqueue("No packets received last 3 seconds.");
	  }
  }
}
