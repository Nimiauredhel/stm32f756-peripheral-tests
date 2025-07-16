/*
 * server_common.h
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

/**
 * @file server_common.h
 * @brief Header file for resources common to the server (or 'unit under test') module.
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

/**
 * @brief A data structure for variables holding an incoming test request.
 */
typedef struct TestRequest
{
    /// Source IP address of the requesting client.
	ip_addr_t client_addr;
    /// Source port of the requesting client.
	u16_t client_port;
    /// Storage buffer for the request packet received from the client.
	uint8_t request[TEST_REQUEST_PACKET_MAX_SIZE_BYTES];
} TestRequest_t;

/**
 * @brief A data structure for variables holding an outbound message.
 */
typedef struct OutgoingMessage
{
    /// Destination IP address of the outbound message.
	ip_addr_t addr;
    /// Destination port of the outbound message.
	u16_t port;
    /// Storage buffer for the complete outbound message packet.
	uint8_t message[TEST_MSG_PACKET_SIZE_BYTES];
} OutgoingMessage_t;

#endif /* SERVER_COMMON_H_ */
