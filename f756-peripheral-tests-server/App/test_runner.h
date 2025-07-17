/*
 * test_runner.h
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

/**
 * @file test_runner.h
 * @brief Header file for the 'test runner' task functionality.
 */

#ifndef TEST_RUNNER_H_
#define TEST_RUNNER_H_

#include "peripheral_tests.h"

void test_runner_task_init(void);
void test_runner_task_loop(void);

#endif /* TEST_RUNNER_H_ */
