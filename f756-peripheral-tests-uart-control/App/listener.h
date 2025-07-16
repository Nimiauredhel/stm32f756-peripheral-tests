/*
 * eth_server.h
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */

#ifndef LISTENER_H_
#define LISTENER_H_

#include "main.h"
#include "test_packet_def.h"
#include "peripheral_tests.h"

/**
 * @brief Initialization of the ethernet listener task.
 */
void test_listener_task_init(void);
/**
 * @brief Operation loop of the ethernet listener task.
 */
void test_listener_task_loop(void);

#endif /* LISTENER_H_ */
