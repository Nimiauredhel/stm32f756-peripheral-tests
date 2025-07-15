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

static const uint16_t recv_timeout_ms = 1000;
static const uint16_t recv_idle_debug_ms = 30000;

static uint32_t recv_idle_counter_ms = 0;

static struct netconn *listener_conn = NULL;
static ip4_addr_t bound_address = {0};
static uint8_t last_link_up_idx = 0;

static uint16_t next_test_id_server_half = 0;
static TestRequest_t request_scratch = {0};
static OutgoingMessage_t message_scratch = {0};

static char debug_buff[SERIAL_DEBUG_MAX_LEN] = {0};

static bool eth_link_was_down(void)
{
	return (eth_link_status_idx != last_link_up_idx
			|| eth_link_status_idx == 0);
}

static void await_eth_link(void)
{
	if (eth_link_status_idx == 0)
	{
		serial_debug_enqueue("Waiting for ethernet link.");

		while (eth_link_status_idx == 0)
		{
			vTaskDelay(pdMS_TO_TICKS(500));
		}
	}

	last_link_up_idx = eth_link_status_idx;
}

static void bind_listener(void)
{
	ip4_addr_t given_address = *netif_ip4_addr(&gnetif);

	if(ip4_addr_isany_val(given_address))
	{
		serial_debug_enqueue("Waiting for valid IP address.");

		while(ip4_addr_isany_val(given_address))
		{
			vTaskDelay(pdMS_TO_TICKS(500));
			given_address = *netif_ip4_addr(&gnetif);
		}
	}

	snprintf(debug_buff, sizeof(debug_buff), "IP acquired: %s", ip4addr_ntoa(&given_address));
	serial_debug_enqueue(debug_buff);

	if (bound_address.addr == given_address.addr)
	{
		serial_debug_enqueue("Given IP already bound.");
		return;
	}
	else
	{
		serial_debug_enqueue("Listener binding new IP.");
	}

	if (ERR_OK != netconn_bind(listener_conn, &given_address, SERVER_PORT))
	{
		serial_debug_enqueue("Failed to bind listener connection.");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}
	else
	{
		bound_address = given_address;
		snprintf(debug_buff, sizeof(debug_buff), "Listener bound to IP %s and port %u.", ip4addr_ntoa(&given_address), SERVER_PORT);
		serial_debug_enqueue(debug_buff);
	}
}

void test_listener_task_init(void)
{
	serial_debug_enqueue("Listener Task started.");

	explicit_bzero(&request_scratch, sizeof(request_scratch));

	listener_conn = netconn_new(NETCONN_UDP);

	if (listener_conn == NULL)
	{
		serial_debug_enqueue("Failed to create listener connection.");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}

	netconn_set_recvtimeout(listener_conn, recv_timeout_ms);
}

void test_listener_task_loop(void)
{
	static struct netbuf *listener_netbuf = NULL;
	static uint8_t *listener_pbuf = NULL;
	static uint16_t listener_pbuf_len = 0;

	static err_t recv_ret;

	/* Infinite loop */
	for(;;)
	{
		if (eth_link_was_down())
		{
			await_eth_link();
			bind_listener();
			serial_debug_enqueue("Listener awaiting requests.");
		}

		recv_ret = netconn_recv(listener_conn, &listener_netbuf);

		switch(recv_ret)
		{
		case ERR_OK:
			recv_idle_counter_ms = 0;
			netbuf_data(listener_netbuf, (void **)&listener_pbuf, &listener_pbuf_len);

			if (listener_pbuf[0] == TEST_PACKET_START_BYTE_VALUE)
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
					bzero(&message_scratch, sizeof(message_scratch));
					message_scratch.port = CLIENT_PORT;
					message_scratch.addr = *IP4_ADDR_BROADCAST;
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
			else
			{
				serial_debug_enqueue("Received invalid packet.");
			}

			serial_debug_enqueue("Listener awaiting requests.");
			break;
		case ERR_TIMEOUT:
			recv_idle_counter_ms += recv_timeout_ms;

			if (recv_idle_counter_ms % recv_idle_debug_ms == 0)
			{
				snprintf(debug_buff, sizeof(debug_buff), "Listener idle for %lu seconds.", recv_idle_counter_ms/1000);
				serial_debug_enqueue(debug_buff);
			}
			break;
		default:
			snprintf(debug_buff, sizeof(debug_buff), "Listener recv() error: %s", lwip_strerr(recv_ret));
			serial_debug_enqueue(debug_buff);
			break;
		}
	}
}
