/*
 * transmitter.c
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

#include "server_common.h"
#include "transmitter.h"

extern struct netif gnetif;
extern osMessageQueueId_t OutboxQueueHandle;

static struct netconn *transmitter_conn = NULL;
static OutgoingMessage_t current_message = {0};

void transmitter_task_init(void)
{
	serial_debug_enqueue("Ethernet Transmitter Task started.");

	transmitter_conn = netconn_new(NETCONN_UDP);

	if (transmitter_conn == NULL)
	{
		serial_debug_enqueue("Failed to create transmitter connection.");
		vTaskDelay(pdMS_TO_TICKS(1000));
		HAL_NVIC_SystemReset();
	}

	while(ip4_addr_isany_val(*netif_ip4_addr(&gnetif)))
	{
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void transmitter_task_loop(void)
{
	for (;;)
	{
		serial_debug_enqueue("Transmitter waiting for outgoing messages.");
		if (osOK == osMessageQueueGet(OutboxQueueHandle, &current_message, 0, HAL_MAX_DELAY))
		{
			serial_debug_enqueue("Transmitter handling outgoing message.");
			uint8_t packet_size;
			switch (current_message.message[TEST_PACKET_MSG_BYTE_OFFSET])
			{
			case TESTMSG_PAIRING_BEACON:
				packet_size = PAIRING_PACKET_SIZE_BYTES;
				break;
			case TESTMSG_TEST_OVER_RESULTS:
				packet_size = TEST_REQUEST_PACKET_MIN_SIZE_BYTES;
				break;
			default:
				packet_size = TEST_MSG_PACKET_SIZE_BYTES;
				break;
			}

			struct netbuf *out_netbuf = netbuf_new();
			void *out_pbuf = netbuf_alloc(out_netbuf, packet_size);
			memcpy(out_pbuf, current_message.message, packet_size);
			netconn_sendto(transmitter_conn, out_netbuf, &current_message.addr, current_message.port);
			netbuf_delete(out_netbuf);
		}
	}
}
