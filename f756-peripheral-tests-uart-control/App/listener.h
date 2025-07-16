/*
 * eth_server.h
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */

/**
 * @file listener.h
 * @brief Header file for the ethernet listener.
 */

#ifndef LISTENER_H_
#define LISTENER_H_

/**
 * @brief Initialization of the ethernet listener task.
 */
void test_listener_task_init(void);
/**
 * @brief Operation loop of the ethernet listener task.
 */
void test_listener_task_loop(void);

#endif /* LISTENER_H_ */
