/*
 * test_runner.c
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

/**
 * @file test_runner.c
 * @brief The 'Test Runner' task is in charge of fetching test requests from the test queue,
 * running them one request at a time (as a single request may span several test types and many iterations),
 * and finally, composing the test results into a packet and forwarding them to the outbox queue.
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
	static const uint16_t test_runner_delay_ticks = (pdMS_TO_TICKS(250));

	static char debug_buff[256] = {0};

	for(;;)
	{
		vTaskDelay(test_runner_delay_ticks);

		if (osOK == osMessageQueueGet(TestQueueHandle, &current_test, 0, HAL_MAX_DELAY))
		{
			serial_debug_enqueue("Test Runner executing requested test.");

			/**
			 * Preparing the test reference variables.
			 */
			test_reference_prepare(current_test.request+TEST_PACKET_STRING_HEAD_OFFSET, current_test.request[TEST_PACKET_STRING_LEN_OFFSET]);

			uint8_t test_selection_byte = current_test.request[TEST_PACKET_SELECTION_BYTE_OFFSET];
			uint8_t ordered_test_count = 0;
			uint8_t completed_tests = 0;

			/**
			 * Signaling that the tests should start running
			 * by updating the test data structs.
			 */
			for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
			{
				if (0x01 & (test_selection_byte >> (uint8_t)i))
				{
					ordered_test_count++;
					test_instances[i].iterations = current_test.request[TEST_PACKET_ITERATIONS_BYTE_OFFSET];
					test_instances[i].state = TESTSTATE_PENDING;

					snprintf(debug_buff, sizeof(debug_buff), "%s Test Ordered.", test_definitions[i].name);
					serial_debug_enqueue(debug_buff);
				}
				else
				{
					test_instances[i].state = TESTSTATE_READY;
				}
			}

			/**
			 * Preparing and sending a "test started" confirmation packet,
			 * based on the currently processed "new test" request packet.
			 */
			explicit_bzero(&message_scratch, sizeof(message_scratch));
			message_scratch.addr = current_test.client_addr;
			message_scratch.port = current_test.client_port;
			memcpy(message_scratch.message, current_test.request, TEST_MSG_PACKET_SIZE_BYTES);
			message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_START_ACK;
			message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0x01;
			message_scratch.message[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
			osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, HAL_MAX_DELAY);

			/**
			 * Preparing the base "test results" packet.
			 */
			message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_OVER_RESULTS;
			message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0x00;

			serial_debug_enqueue("Test Runner awaiting test completion.");

			while(completed_tests < ordered_test_count)
			{
				vTaskDelay(test_runner_delay_ticks);

				for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
				{
				switch(test_instances[i].state)
				{
				case TESTSTATE_SUCCESS:
					  message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET]
						  |= (1 << (uint8_t)i);
					  snprintf(debug_buff, sizeof(debug_buff), "%s Test Success.", test_definitions[i].name);
					  serial_debug_enqueue(debug_buff);
					  test_instances[i].state = TESTSTATE_READY;
					  completed_tests++;
					  break;
				case TESTSTATE_FAILURE:
					  snprintf(debug_buff, sizeof(debug_buff), "%s Test Failure.", test_definitions[i].name);
					  serial_debug_enqueue(debug_buff);
					  test_instances[i].state = TESTSTATE_READY;
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
