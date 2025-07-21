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

static const uint16_t test_runner_delay_ticks = (pdMS_TO_TICKS(250));

extern osMessageQueueId_t TestQueueHandle;
extern osMessageQueueId_t OutboxQueueHandle;
extern CRC_HandleTypeDef hcrc;

static TestRequest_t current_test = {0};
static OutgoingMessage_t message_scratch = {0};
static char debug_buff[256] = {0};

/**
 * @brief Sets the prepared outbound message packet
 * to carry the test results, and sends it to the out queue.
 */
static void send_test_results(uint8_t results_byte)
{
	message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_OVER_RESULTS;
	message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = results_byte;
	osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, HAL_MAX_DELAY);
	serial_debug_enqueue("Results forwarded to outbox.");
}

/**
 * @brief Sets the prepared outbound message packet
 * to carry a "test start" confirmation, and sends it to the out queue.
 */
static void send_test_start_confirmation(void)
{
	message_scratch.message[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_TEST_START_ACK;
	message_scratch.message[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0x01;
	osMessageQueuePut(OutboxQueueHandle, &message_scratch, 0, HAL_MAX_DELAY);
}

/**
 * @brief Prepares the outbound message buffer by using
 * the currently processed "new test" request packet as a base.
 */
static void prepare_out_message(void)
{
	explicit_bzero(&message_scratch, sizeof(message_scratch));
	message_scratch.addr = current_test.client_addr;
	message_scratch.port = current_test.client_port;
	memcpy(message_scratch.message, current_test.request, TEST_MSG_PACKET_SIZE_BYTES);
	message_scratch.message[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
}

/**
 * @brief Repeatedly polls test instance data until
 * all requested tests are marked finished.
 * @retval the byte encoding the test results
 */
static uint8_t await_tests_completion(uint8_t ordered_test_count)
{
	uint8_t completed_tests = 0;
	uint8_t test_results_byte = 0;

	serial_debug_enqueue("Test Runner awaiting test completion.");

	while(completed_tests < ordered_test_count)
	{
		vTaskDelay(test_runner_delay_ticks);

		for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
		{
		switch(test_instances[i].state)
		{
		case TESTSTATE_SUCCESS:
			  test_results_byte |= (1 << (uint8_t)i);
			  // intentional fallthrough, success & failure operation nearly identical
		case TESTSTATE_FAILURE:
			  if (SERIAL_DEBUG_ENABLED)
			  {
				  snprintf(debug_buff, sizeof(debug_buff), "%s Test %s.", test_definitions[i].name,
					  test_instances[i].state == TESTSTATE_SUCCESS ? "Success" : "Failure");
				  serial_debug_enqueue(debug_buff);
			  }
			  test_instances[i].state = TESTSTATE_READY;
			  completed_tests++;
			  break;
		default:
			  break;
		  }
		}
	}

	serial_debug_enqueue("Tests concluded.");
	return test_results_byte;
}

/**
 * @brief Signals that the tests should start running
 * by updating the test instance data.
 * @retval the count of selected tests
 */
static uint8_t signal_tests_start()
{
	uint8_t ordered_test_count = 0;
	uint8_t test_selection_byte = current_test.request[TEST_PACKET_SELECTION_BYTE_OFFSET];

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

	return ordered_test_count;
}

void test_runner_task_init(void)
{
	serial_debug_enqueue("Test Runner task initialized.");
}

void test_runner_task_loop(void)
{
	uint8_t test_results_byte = 0x00;
	uint8_t ordered_test_count = 0;
	osStatus queue_ret;

	for(;;)
	{
		vTaskDelay(test_runner_delay_ticks);
		queue_ret = osMessageQueueGet(TestQueueHandle, &current_test, 0, HAL_MAX_DELAY);

		if (osOK == queue_ret)
		{
			serial_debug_enqueue("Test Runner executing requested test.");

			test_reference_prepare((char *)(current_test.request+TEST_PACKET_STRING_HEAD_OFFSET), current_test.request[TEST_PACKET_STRING_LEN_OFFSET]);
			ordered_test_count = signal_tests_start();

			prepare_out_message();
			send_test_start_confirmation();

			test_results_byte = await_tests_completion(ordered_test_count);

			send_test_results(test_results_byte);
		}
		else if (queue_ret == osErrorTimeout)
		{
		}
		else
		{
			snprintf(debug_buff, sizeof(debug_buff), "CMSIS error code %ld fetching test request from queue.", queue_ret);
			serial_debug_enqueue(debug_buff);
		}	}
}
