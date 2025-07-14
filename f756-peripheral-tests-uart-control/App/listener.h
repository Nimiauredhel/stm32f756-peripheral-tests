/*
 * eth_server.h
 *
 *  Created on: Jul 11, 2025
 *      Author: mickey
 */

#ifndef LISTENER_H_
#define LISTENER_H_

#define SERVER_PORT (45678)

#include "main.h"
#include "test_packet_def.h"
#include "peripheral_tests.h"

void test_listener_task_init(void);
void test_listener_task_loop(void);

#endif /* LISTENER_H_ */
