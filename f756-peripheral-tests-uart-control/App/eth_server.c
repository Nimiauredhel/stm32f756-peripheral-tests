/*
 * eth_server.c
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */

#include "eth_server.h"
#include "lwip.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"

extern struct netif gnetif;
extern CRC_HandleTypeDef hcrc;

typedef struct TestRequestSlot
{
	bool used;
	ip_addr_t client_addr;
	u16_t client_port;
	uint8_t request[TEST_REQUEST_PACKET_MAX_SIZE_BYTES];
} TestRequestSlot_t;

static uint16_t next_test_id_server_half = 0;

static struct netconn *listener_conn = NULL;

static TestRequestSlot_t request_slot = {0};

void test_listener_task_init(void)
{
	serial_print_line("\r\nEthernet Listener Task started.", 0);

	explicit_bzero(&request_slot, sizeof(request_slot));

	listener_conn = netconn_new(NETCONN_UDP);

	if (listener_conn == NULL)
	{
		serial_print_line("Failed to create listener connection.", 0);
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}

	while(ip4_addr_isany_val(*netif_ip4_addr(&gnetif)))
	{
		serial_print_line("Waiting for valid IP address.", 0);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
    serial_print("Server IP is:", 0);
    serial_print_line(ip4addr_ntoa(netif_ip4_addr(&gnetif)), 0);

	if (ERR_OK != netconn_bind(listener_conn, netif_ip4_addr(&gnetif), SERVER_PORT))
	{
		serial_print_line("Failed to bind listener connection.", 0);
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}
	else
	{
		serial_print_line("Bound netconn.", 0);
	}
}

void test_listener_task_loop(void)
{
	static struct netbuf *listener_netbuf = NULL;
	static uint8_t *listener_pbuf = NULL;
	static uint16_t listener_pbuf_len = 0;

	static char debug_buff[256] = {0};

	static uint8_t response_packet[TEST_MSG_PACKET_SIZE_BYTES];

	serial_print("Awaiting Test Requests.", 0);

	/* Infinite loop */
	for(;;)
	{
		if (ERR_OK == netconn_recv(listener_conn, &listener_netbuf))
		{
			netbuf_data(listener_netbuf, (void **)&listener_pbuf, &listener_pbuf_len);

			if (listener_pbuf[0] == TEST_PACKET_START_BYTE_VALUE
				&& (TestPacketMsg_t)listener_pbuf[TEST_PACKET_MSG_BYTE_OFFSET] == TESTMSG_TEST_NEW_REQUEST)
			{
				// save client data to slot
				request_slot.used = true;
				request_slot.client_addr = listener_netbuf->addr;
				request_slot.client_port = listener_netbuf->port;
				memcpy(request_slot.request, listener_pbuf, listener_pbuf_len);
				netbuf_delete(listener_netbuf);

				request_slot.request[TEST_PACKET_ID_BYTE_OFFSET] |= (uint32_t)next_test_id_server_half;
				next_test_id_server_half = (next_test_id_server_half == UINT16_MAX) ? 0 : next_test_id_server_half + 1;

				snprintf(debug_buff, sizeof(debug_buff), "\r\nDevice received test string: %s", request_slot.request+TEST_PACKET_STRING_HEAD_OFFSET);
				serial_print_line(debug_buff, 0);

				// confirm reception
				bzero(response_packet, TEST_MSG_PACKET_SIZE_BYTES);
				response_packet[0] = TEST_PACKET_START_BYTE_VALUE;
				*(uint32_t *)(response_packet+TEST_PACKET_ID_BYTE_OFFSET)
				= *(uint32_t *)(request_slot.request+TEST_PACKET_ID_BYTE_OFFSET);
				response_packet[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_NEW_ACK;
				response_packet[TEST_PACKET_STRING_HEAD_OFFSET] = TEST_PACKET_END_BYTE_VALUE;

				struct netbuf *response_netbuf = netbuf_new();
				void *response_pbuf = netbuf_alloc(response_netbuf, TEST_MSG_PACKET_SIZE_BYTES);
				memcpy(response_pbuf, response_packet, TEST_MSG_PACKET_SIZE_BYTES);
				netconn_sendto(listener_conn, response_netbuf, &request_slot.client_addr, request_slot.client_port);
				netbuf_delete(response_netbuf);

				// prepare test reference
				test_string_len = request_slot.request[TEST_PACKET_STRING_LEN_OFFSET];
				explicit_bzero(test_string_buff, sizeof(test_string_buff));
				strncpy(test_string_buff, (char *)(request_slot.request+TEST_PACKET_STRING_HEAD_OFFSET), test_string_len);
				test_string_crc = HAL_CRC_Calculate(&hcrc, test_string_buff, test_string_len);

				uint8_t test_selection_byte = request_slot.request[TEST_PACKET_SELECTION_BYTE_OFFSET];
				uint8_t ordered_test_count = 0;
				uint8_t completed_tests = 0;

				for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
				{
					if (0x01 & (test_selection_byte >> (uint8_t)i))
					{
					    sprintf(debug_buff, "%s Test Ordered.", test_defs[i].name);
					    serial_print_line(debug_buff, 0);
						ordered_test_count++;
						test_defs[i].iterations = request_slot.request[TEST_PACKET_ITERATIONS_BYTE_OFFSET];
						test_defs[i].state = TESTSTATE_PENDING;
					}
					else
					{
						test_defs[i].state = TESTSTATE_READY;
					}
				}

				// prepare results packet
				response_packet[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_OVER_RESULTS;
				response_packet[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0x00;

				serial_print_line("Awaiting test completion.", 0);

				while(completed_tests < ordered_test_count)
				{
				    vTaskDelay(pdMS_TO_TICKS(250));

					for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
					{
					switch(test_defs[i].state)
					{
					case TESTSTATE_SUCCESS:
						  response_packet[TEST_PACKET_SELECTION_BYTE_OFFSET]
						      |= (1 << (uint8_t)i);
						  sprintf(debug_buff, "%s Test Success.", test_defs[i].name);
						  serial_print_line(debug_buff, 0);
						  test_defs[i].state = TESTSTATE_READY;
						  completed_tests++;
						  break;
					case TESTSTATE_FAILURE:
						  sprintf(debug_buff, "%s Test Failure.", test_defs[i].name);
						  serial_print_line(debug_buff, 0);
						  test_defs[i].state = TESTSTATE_READY;
						  completed_tests++;
						  break;
					default:
						  break;
					  }
					}
				}

				serial_print_line("Tests concluded.", 0);

				response_netbuf = netbuf_new();
				response_pbuf = netbuf_alloc(response_netbuf, TEST_MSG_PACKET_SIZE_BYTES);
				memcpy(response_pbuf, response_packet, TEST_MSG_PACKET_SIZE_BYTES);
				netconn_sendto(listener_conn, response_netbuf, &request_slot.client_addr, request_slot.client_port);
				netbuf_delete(response_netbuf);
				serial_print_line("Results sent.", 0);
				serial_print("Awaiting Test Requests.", 0);
		    }
			else
			{
				serial_print(".", 1);
			}
	  }
	  else
	  {
			serial_print_line("\nnetconn_recv error.", 0);
	  }
  }
}
