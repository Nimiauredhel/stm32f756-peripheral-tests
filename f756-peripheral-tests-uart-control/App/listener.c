/*
 * listener.c
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */

/**
 * @file listener.c
 * @details
 * The 'Listener' task is in charge of receiving incoming UDP packets over an ethernet link
 * and reacting appropriately, either by issuing an immediate response or forwarding them to further processing.
 */

#include "listener.h"
#include "main.h"
#include "server_common.h"


extern struct netif gnetif;
extern osMessageQueueId_t TestQueueHandle;
extern osMessageQueueId_t OutboxQueueHandle;

static const uint16_t recv_timeout_ms = 1000;
static const uint16_t recv_idle_debug_ms = 60000;

static uint64_t recv_idle_counter_ms = 0;

static struct netconn *listener_conn = NULL;
static ip4_addr_t listener_address = {0};
static uint8_t last_link_up_idx = 0;

static uint16_t next_test_id_server_half = 0;
static TestRequest_t request_scratch = {0};
static OutgoingMessage_t message_scratch = {0};

static char debug_buff[SERIAL_DEBUG_MAX_LEN] = {0};

/**
 * @brief Checks whether the ethernet link status changed since the local status index was last updated.
 * @retval true Link status changed since the status idx was last updated.
 * @retval false No change since the status idx was last updated.
 */
static bool eth_link_was_down(void)
{
	uint8_t eth_link_status_idx = lwip_get_eth_link_status_idx();

	return (eth_link_status_idx != last_link_up_idx
			|| eth_link_status_idx == 0);
}

/**
 * @brief Blocks task until ethernet link is up, then updates the local status index.
 */
static void await_eth_link(void)
{
	uint8_t eth_link_status_idx = lwip_get_eth_link_status_idx();

	if (eth_link_status_idx == 0)
	{
		serial_debug_enqueue("Waiting for ethernet link.");

		do
		{
			vTaskDelay(pdMS_TO_TICKS(500));
			eth_link_status_idx = lwip_get_eth_link_status_idx();
		} while (eth_link_status_idx == 0);
	}

	last_link_up_idx = eth_link_status_idx;
}

/**
 * @brief Blocks task until device is assigned an IP address, then binds it to the listener if necessary.
 */
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

	if (listener_address.addr == given_address.addr)
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
		listener_address = given_address;
		snprintf(debug_buff, sizeof(debug_buff), "Listener bound to IP %s and port %u.", ip4addr_ntoa(&given_address), SERVER_PORT);
		serial_debug_enqueue(debug_buff);
	}
}

/**
 * @details
 * The @ref test_listener_task_init function explicitly wipes @ref request_scratch, the buffer used to hold incoming packets.
 * It then initializes the @ref netconn to be used for receiving the packets,
 * and sets its timeout duration according to @ref recv_timeout_ms.
 */
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

/**
 * @details
 * The @ref test_listener_task_loop function is constantly listening for incoming UDP packets.
 * It checks the ethernet link status each iteration with @ref eth_link_was_down and rebinds if necessary.
 * The received packet is then filtered by type. Test requests are held in @ref request_scratch and sent to the test queue.
 * Other packets require an immediate response, which is constructed in @ref message_scratch and sent directly to the outbox queue.
 */
void test_listener_task_loop(void)
{
	static struct netbuf *listener_netbuf = NULL;
	static uint8_t *listener_pbuf = NULL;
	static uint16_t listener_pbuf_len = 0;

	static err_t recv_ret;

	/* Infinite loop */
	for(;;)
	{
		if (listener_netbuf != NULL)
		{
			netbuf_delete(listener_netbuf);
		}

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
					explicit_bzero(&request_scratch, sizeof(request_scratch));
					request_scratch.client_addr = listener_netbuf->addr;
					request_scratch.client_port = listener_netbuf->port;
					memcpy(request_scratch.request, listener_pbuf, listener_pbuf_len);
					netbuf_delete(listener_netbuf);

					*((uint32_t *)request_scratch.request+TEST_PACKET_ID_BYTE_OFFSET) |= (uint32_t)next_test_id_server_half;
					next_test_id_server_half = (next_test_id_server_half == UINT16_MAX) ? 0 : next_test_id_server_half + 1;

					snprintf(debug_buff, sizeof(debug_buff), "\r\nDevice received test string: %s", request_scratch.request+TEST_PACKET_STRING_HEAD_OFFSET);
					serial_debug_enqueue(debug_buff);

					// forward request to test queue
					bool forwarded = (osOK == osMessageQueuePut(TestQueueHandle, &request_scratch, 0, pdMS_TO_TICKS(1000)));

					snprintf(debug_buff, sizeof(debug_buff), "Test request %sforwarded to queue.", forwarded ? "" : "NOT ");
					serial_debug_enqueue(debug_buff);

					// confirm reception
					explicit_bzero(&message_scratch, sizeof(message_scratch));
					message_scratch.addr = request_scratch.client_addr;
					message_scratch.port = request_scratch.client_port;
					message_scratch.message[0] = TEST_PACKET_START_BYTE_VALUE;
					*(uint32_t *)(message_scratch.message+TEST_PACKET_ID_BYTE_OFFSET)
					= *(uint32_t *)(request_scratch.request+TEST_PACKET_ID_BYTE_OFFSET);
					message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_NEW_ACK;
					message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = forwarded ? 1 : 0;
					message_scratch.message[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;

					for (uint8_t i = 0; i < 4; i++)
					{
						osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, pdMS_TO_TICKS(1000));
					}
					break;
				case TESTMSG_PAIRING_PROBE:
					serial_debug_enqueue("Received a client probe packet.");
					// send out a beacon
					explicit_bzero(&message_scratch, sizeof(message_scratch));
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
				snprintf(debug_buff, sizeof(debug_buff), "Listener idle for %lu minutes.", recv_idle_counter_ms/60000);
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
