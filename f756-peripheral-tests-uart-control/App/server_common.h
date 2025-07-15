/*
 * server_common.h
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

#ifndef SERVER_COMMON_H_
#define SERVER_COMMON_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lwipopts.h"
#include "lwip.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "test_packet_def.h"

#define CLIENT_PORT (34567)
#define SERVER_PORT (45678)

typedef struct TestRequest
{
	ip_addr_t client_addr;
	u16_t client_port;
	uint8_t request[TEST_REQUEST_PACKET_MAX_SIZE_BYTES];
} TestRequest_t;

typedef struct OutgoingMessage
{
	ip_addr_t addr;
	u16_t port;
	uint8_t message[TEST_MSG_PACKET_SIZE_BYTES];
} OutgoingMessage_t;

#endif /* SERVER_COMMON_H_ */
