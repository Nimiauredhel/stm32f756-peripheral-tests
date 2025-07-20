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
 * @brief Type of variables representing the current state of a test unit.
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
 * @brief Data structure holding reference variables used to evaluate currently running tests.
 */
typedef struct TestReferenceData
{
	/// @brief The test string used by the currently running peripheral tests.
	char test_string_buff[TEST_PACKET_STR_MAX_LEN];
	/// @brief Reference CRC value of test string used by the currently running peripheral tests.
	uint32_t test_string_crc;
	/// @brief Length of the test string used by the currently running peripheral tests.
	uint8_t test_string_len;
} TestReferenceData_t;

/**
 * @brief Type of variables holding static data used by a specific test.
 */
typedef struct TestUnitDefinition
{
	const char name[16];
	bool (*func)(void);
} TestUnitDefinition_t;

/**
 * @brief Type of variables holding dynamic data used by a specific test.
 */
typedef struct TestUnitInstance
{
	volatile PeripheralTestState_t state : 8;
	volatile uint8_t iterations;
} TestUnitInstance_t;

/**
 * @brief Array of variables holding static data about test units.
 */
extern const TestUnitDefinition_t test_definitions[NUM_POSSIBLE_TESTS];
/**
 * @brief Array of variables holding the dynamic state of test units.
 */
extern TestUnitInstance_t test_instances[NUM_POSSIBLE_TESTS];

void test_reference_prepare(char *test_str, uint8_t test_str_len);

/**
 * @brief A generic loop used by the tasks running individual peripheral tests.
 * @details
 * This generic loop manages the test instance associated with the given index.
 * When it detects a state change to PENDING, it progresses the state to BUSY.
 * It then runs the bespoke test implementation, whose return value is finally
 * assigned to the state field when finished.
 * @param [in] test_index Index key to the data used by the task to run tests.
 */
void test_task_loop(uint8_t test_index);

#endif /* INC_PERIPHERAL_TESTS_H_ */
