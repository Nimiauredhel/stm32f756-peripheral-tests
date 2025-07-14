/*
 * test_runner.c
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

#include "server_common.h"
#include "test_runner.h"

extern osMessageQueueId_t TestQueueHandle;
extern osMessageQueueId_t OutboxQueueHandle;
extern CRC_HandleTypeDef hcrc;

TestRequest_t current_test = {0};
OutgoingMessage_t message_scratch = {0};

void test_runner_task_init(void)
{
	serial_debug_enqueue("Test Runner task initialized.");
}

void test_runner_task_loop(void)
{
	static char debug_buff[256] = {0};

	for(;;)
	{
		vTaskDelay(pdMS_TO_TICKS(500));

		if (osOK == osMessageQueueGet(TestQueueHandle, &current_test, 0, HAL_MAX_DELAY))
		{
			serial_debug_enqueue("Test Runner executing requested test.");

			// prepare test reference
			test_string_len = current_test.request[TEST_PACKET_STRING_LEN_OFFSET];
			explicit_bzero(test_string_buff, sizeof(test_string_buff));
			strncpy(test_string_buff, (char *)(current_test.request+TEST_PACKET_STRING_HEAD_OFFSET), test_string_len);
			test_string_crc = HAL_CRC_Calculate(&hcrc, test_string_buff, test_string_len);

			uint8_t test_selection_byte = current_test.request[TEST_PACKET_SELECTION_BYTE_OFFSET];
			uint8_t ordered_test_count = 0;
			uint8_t completed_tests = 0;

			for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
			{
				if (0x01 & (test_selection_byte >> (uint8_t)i))
				{
					sprintf(debug_buff, "%s Test Ordered.", test_defs[i].name);
					serial_debug_enqueue(debug_buff);
					ordered_test_count++;
					test_defs[i].iterations = current_test.request[TEST_PACKET_ITERATIONS_BYTE_OFFSET];
					test_defs[i].state = TESTSTATE_PENDING;
				}
				else
				{
					test_defs[i].state = TESTSTATE_READY;
				}
			}

			// prepare results packet
			// using request packet as a base
			message_scratch.addr = current_test.client_addr;
			message_scratch.port = current_test.client_port;
			memcpy(message_scratch.message, current_test.request, TEST_MSG_PACKET_SIZE_BYTES);
			message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_OVER_RESULTS;
			message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0x00;
			message_scratch.message[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;

			serial_debug_enqueue("Awaiting test completion.");

			while(completed_tests < ordered_test_count)
			{
				vTaskDelay(pdMS_TO_TICKS(250));

				for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
				{
				switch(test_defs[i].state)
				{
				case TESTSTATE_SUCCESS:
					  message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET]
						  |= (1 << (uint8_t)i);
					  sprintf(debug_buff, "%s Test Success.", test_defs[i].name);
					  serial_debug_enqueue(debug_buff);
					  test_defs[i].state = TESTSTATE_READY;
					  completed_tests++;
					  break;
				case TESTSTATE_FAILURE:
					  sprintf(debug_buff, "%s Test Failure.", test_defs[i].name);
					  serial_debug_enqueue(debug_buff);
					  test_defs[i].state = TESTSTATE_READY;
					  completed_tests++;
					  break;
				default:
					  break;
				  }
				}
			}

			serial_debug_enqueue("Tests concluded.");
			osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, HAL_MAX_DELAY);
		}
	}
}
