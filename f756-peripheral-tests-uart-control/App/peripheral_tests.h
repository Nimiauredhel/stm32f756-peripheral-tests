/*
 * peripheral_tests.h
 *
 *  Created on: Jul 7, 2025
 *      Author: mickey
 */

#ifndef INC_PERIPHERAL_TESTS_H_
#define INC_PERIPHERAL_TESTS_H_

#include "main.h"
#include "packet_ops.h"

#define TEST_STRING_MAX_LEN (TEST_PACKET_STR_MAX_LEN)
#define TEST_TIMEOUT_TICKS (pdMS_TO_TICKS(10000))
#define TEST_GAP_TICKS (pdMS_TO_TICKS(500))

typedef volatile enum PeripheralTestState
{
	TESTSTATE_READY = 0,
	TESTSTATE_PENDING = 1,
	TESTSTATE_BUSY = 2,
	TESTSTATE_SUCCESS = 3,
	TESTSTATE_FAILURE = 4,
} PeripheralTestState_t;

typedef struct TestDefinition
{
	const char name[16];
	volatile PeripheralTestState_t state;
	bool (*func)(const char*, const uint8_t);
} TestDefinition_t;

extern volatile uint8_t test_string_len;
extern char test_string_buff[TEST_STRING_MAX_LEN];
extern uint32_t test_string_crc;

extern TestDefinition_t test_defs[NUM_POSSIBLE_TESTS];

void test_task_loop(TestDefinition_t *def);

#endif /* INC_PERIPHERAL_TESTS_H_ */
