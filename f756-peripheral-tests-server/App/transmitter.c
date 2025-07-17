/*
 * transmitter.c
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

/**
 * @file transmitter.c
 * @brief The 'Transmitter' task is in charge of fetching outbound packets from the outbox queue
 * and sending them to the intended recipient, utilizing best-effort UDP over an ethernet link.
 */

#include "server_common.h"
#include "transmitter.h"

extern struct netif gnetif;
extern osMessageQueueId_t OutboxQueueHandle;

static struct netconn *transmitter_conn = NULL;
static OutgoingMessage_t current_message = {0};

static char debug_buff[SERIAL_DEBUG_MAX_LEN] = {0};

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
	static const uint16_t outbox_timeout = pdMS_TO_TICKS(100);

	static osStatus_t outbox_ret;
	static uint8_t packet_size;

	static struct netbuf *out_netbuf = NULL;
	static void *out_pbuf = NULL;

	serial_debug_enqueue("Transmitter waiting for outgoing messages.");

	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
		outbox_ret = osMessageQueueGet(OutboxQueueHandle, &current_message, 0, outbox_timeout);

		switch(outbox_ret)
		{
		case osOK:
			serial_debug_enqueue("Transmitter handling outgoing message.");

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

			snprintf(debug_buff, sizeof(debug_buff), "Outgoing packet size: %u.", packet_size);
			serial_debug_enqueue(debug_buff);

			out_netbuf = netbuf_new();
			out_pbuf = netbuf_alloc(out_netbuf, packet_size);
			memcpy(out_pbuf, current_message.message, packet_size);
			netconn_sendto(transmitter_conn, out_netbuf, &current_message.addr, current_message.port);
			netbuf_delete(out_netbuf);

			serial_debug_enqueue("Transmitter waiting for outgoing messages.");
			break;
		case osErrorTimeout:
			break;
		default:
			serial_debug_enqueue("Error fetching from Outbox queue.");
			break;
		}
	}
}
