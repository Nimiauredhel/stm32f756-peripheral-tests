/*
 * peripheral_tests.h
 *
 *  Created on: Jul 7, 2025
 *      Author: mickey
 */

/**
 * @file peripheral_tests.h
 * @brief Header file for the module where the peripheral tests are implemented.
 */

#ifndef INC_PERIPHERAL_TESTS_H_
#define INC_PERIPHERAL_TESTS_H_

/**
 * @brief The timeout value (in ticks) to be used by tested peripherals where applicable.
 */
#define TEST_TIMEOUT_TICKS (pdMS_TO_TICKS(5000))
/**
 * @brief The delay time (in ticks) between test related operations.
 */
#define TEST_GAP_TICKS (pdMS_TO_TICKS(500))

/**
 * @brief Type of variables representing the state of a peripheral under test.
 */
typedef volatile enum PeripheralTestState
{
	TESTSTATE_READY = 0,
	TESTSTATE_PENDING = 1,
	TESTSTATE_BUSY = 2,
	TESTSTATE_SUCCESS = 3,
	TESTSTATE_FAILURE = 4,
} PeripheralTestState_t;

/**
 * @brief Type of variables holding data used by a variable under test.
 */
typedef struct TestData
{
	const char name[16];
	volatile PeripheralTestState_t state;
	volatile uint8_t iterations;
	bool (*func)(const char*, const uint8_t);
} TestData_t;

/**
 * @brief Length of the test string used by the currently running peripheral tests.
 * TODO: refactor into struct
 */
extern volatile uint8_t test_string_len;
/**
 * @brief The test string used by the currently running peripheral tests.
 * TODO: refactor into struct
 */
extern char test_string_buff[TEST_PACKET_STR_MAX_LEN];
/**
 * @brief Reference CRC value of test string used by the currently running peripheral tests.
 * TODO: refactor into struct
 */
extern uint32_t test_string_crc;

/**
 * @brief Array of variables holding up to date information about the peripherals under test.
 * TODO: refactor into struct, formalize access
 */
extern TestData_t test_defs[NUM_POSSIBLE_TESTS];

/**
 * @brief A generic loop used by the tasks running individual peripheral tests.
 * @param [in] *def The specific data used by the task to run individual tests.
 */
void test_task_loop(TestData_t *def);

#endif /* INC_PERIPHERAL_TESTS_H_ */
