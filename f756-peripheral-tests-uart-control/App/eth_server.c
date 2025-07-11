/*
 * eth_server.c
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */


#include "eth_server.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"

typedef struct TestClientSlot
{
	bool used;
	ip_addr_t addr;
	u16_t port;
	uint8_t request[TEST_PACKET_SIZE_BYTES];
} TestClientSlot_t;

static struct netconn *listener_conn = NULL;

static TestClientSlot_t client_slot = {0};

void test_listener_task_init(void)
{
	serial_print_line("Ethernet Listener Task started.", 0);

	explicit_bzero(&client_slot, sizeof(client_slot));

	listener_conn = netconn_new(NETCONN_UDP);

	if (listener_conn == NULL)
	{
		printf("Failed to create listener connection.\n");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}

	if (ERR_OK != netconn_bind(listener_conn, IP_ADDR_ANY, SERVER_PORT))
	{
		printf("Failed to bind listener connection.\n");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}
}

void test_listener_task_loop(void)
{
	static struct netbuf *listener_netbuf = {0};
	static uint8_t *listener_data_ptr = NULL;
	static uint16_t listener_data_len = 0;

	static char debug_tx_buff[64] = {0};

	serial_print("Awaiting Test Requests.", 0);

	/* Infinite loop */
	for(;;)
	{
		if (ERR_OK == netconn_recv(listener_conn, &listener_netbuf))
		{
			netbuf_data(listener_netbuf, (void **)&listener_data_ptr, &listener_data_len);

			if (listener_data_len == TEST_PACKET_SIZE_BYTES
				&& listener_data_ptr[0] == TEST_PACKET_START_BYTE_VALUE
				&& (TestPacketMsg_t)listener_data_ptr[TEST_PACKET_MSG_BYTE_OFFSET] == TESTMSG_NEWTEST
				&& listener_data_ptr[TEST_PACKET_SIZE_BYTES-1] == TEST_PACKET_END_BYTE_VALUE)
			{
				// copy to global test string buff
				test_string_len = listener_data_ptr[TEST_PACKET_STRING_LEN_OFFSET];
				explicit_bzero(test_string_buff, sizeof(test_string_buff));
				strncpy(test_string_buff, (char *)(listener_data_ptr+TEST_PACKET_STRING_HEAD_OFFSET), TEST_STRING_MAX_LEN);

				// save client data to slot
				client_slot.used = true;
				client_slot.addr = listener_netbuf->addr;
				client_slot.port = listener_netbuf->port;
				memcpy(client_slot.request, listener_data_ptr, TEST_PACKET_SIZE_BYTES);

				// confirm reception
				listener_data_ptr[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_ACK;
				netconn_sendto(listener_conn, listener_netbuf, &client_slot.addr, client_slot.port);
				sprintf(debug_tx_buff, "\r\nDevice received test string: %s", test_string_buff);
				serial_print_line(debug_tx_buff, 0);

				// prepare test reference
				uint8_t test_selection_byte = listener_data_ptr[TEST_PACKET_SELECTION_BYTE_OFFSET];
				uint8_t ordered_test_count = 0;
				uint8_t completed_tests = 0;

				// prepare results packet
				listener_data_ptr[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_RESULT;
				listener_data_ptr[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0x00;

				for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
				{
					if (0x01 & (test_selection_byte >> (uint8_t)i))
					{
					    sprintf(debug_tx_buff, "%s Test Ordered.", test_defs[i].name);
					    serial_print_line(debug_tx_buff, 0);
						ordered_test_count++;
						test_defs[i].state = TESTSTATE_PENDING;
					}
					else
					{
						test_defs[i].state = TESTSTATE_READY;
					}
				}

				while(completed_tests < ordered_test_count)
				{
				    vTaskDelay(pdMS_TO_TICKS(250));

					for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
					{
					switch(test_defs[i].state)
					{
					case TESTSTATE_SUCCESS:
						  listener_data_ptr[TEST_PACKET_SELECTION_BYTE_OFFSET]
						      |= (1 << (uint8_t)i);
						  sprintf(debug_tx_buff, "%s Test Success.", test_defs[i].name);
						  serial_print_line(debug_tx_buff, 0);
						  test_defs[i].state = TESTSTATE_READY;
						  completed_tests++;
						  break;
					case TESTSTATE_FAILURE:
						  sprintf(debug_tx_buff, "%s Test Failure.", test_defs[i].name);
						  serial_print_line(debug_tx_buff, 0);
						  test_defs[i].state = TESTSTATE_READY;
						  completed_tests++;
						  break;
					default:
						  break;
					  }
					}
				}
				netconn_sendto(listener_conn, listener_netbuf, &client_slot.addr, client_slot.port);
				serial_print_line("Tests concluded.", 0);
				explicit_bzero(listener_data_ptr, sizeof(listener_data_ptr));
				netbuf_delete(listener_netbuf);
				serial_print("Awaiting Test Requests.", 0);
		    }
			else
			{
				serial_print(".", 1);
			}
	  }
  }
}
