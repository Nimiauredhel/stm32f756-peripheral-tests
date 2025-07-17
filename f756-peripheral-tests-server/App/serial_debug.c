/*
 * serial_debug.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

/**
 * @file serial_debug.c
 * @brief Source file for the 'debug' serial printing utility.
 */

#include "serial_debug.h"

#define UART_PEER huart3
#define UART_TX_TIMEOUT pdMS_TO_TICKS(2000)

extern UART_HandleTypeDef huart3;
extern osMessageQueueId_t DebugQueueHandle;

/***
 * the serial_print_line function takes a raw string and sends it over UART,
 * with the addition of newline characters at the end.
 * the len parameter can optionally supply the length of string to print.
 * if len == 0 the length is considered unknown and updated using strlen.
 ***/
static void serial_print_line(const char *msg, uint16_t len)
{
	if (!SERIAL_DEBUG_ENABLED) return;

	static const uint8_t newline[2] = {'\r', '\n'};

	// a NULL message is valid as a request to just print a newline
	if (msg != NULL)
	{
		if (len == 0) len = strlen(msg);
		HAL_UART_Transmit(&UART_PEER, (uint8_t *)msg, len, UART_TX_TIMEOUT);
	}
	HAL_UART_Transmit(&UART_PEER, newline, 2, UART_TX_TIMEOUT);
}

/***
 * the serial_debug_initialize functions is used to initialize
 * variables and states used by the debug task, which are currently none.
 ***/
void serial_debug_initialize()
{
	if (!SERIAL_DEBUG_ENABLED) return;

	serial_print_line("Debug Task initialized.", 0);
}

/***
 * the serial_debug_loop functions periodically checks the debug queue
 * for new messages and prints them in order.
 ***/
void serial_debug_loop()
{
	if (!SERIAL_DEBUG_ENABLED) return;

	char debug_buff[256] = {0};

	for(;;)
	{
		if (osOK == osMessageQueueGet(DebugQueueHandle, debug_buff, 0, UART_TX_TIMEOUT))
		{
			serial_print_line(debug_buff, 0);
		}
	}
}

/***
 * enqueues the given string in the serial debug queue.
 * if the string is longer than the maximum allowed length,
 * it is truncated using a separate buffer.
 ***/
void serial_debug_enqueue(const char *msg)
{
	if (!SERIAL_DEBUG_ENABLED) return;

	size_t size = sizeof(msg);
	size_t len = strnlen(msg, size < SERIAL_DEBUG_MAX_LEN ? size : SERIAL_DEBUG_MAX_LEN);

	if (len > SERIAL_DEBUG_MAX_LEN)
	{
		char truncation_buff[SERIAL_DEBUG_MAX_LEN+1] = {0};
		snprintf(truncation_buff, SERIAL_DEBUG_MAX_LEN, msg);
		osMessageQueuePut(DebugQueueHandle, truncation_buff, 0, UART_TX_TIMEOUT);
		osMessageQueuePut(DebugQueueHandle, "[TRUNCATED]", 0, UART_TX_TIMEOUT);
	}
	else
	{
		osMessageQueuePut(DebugQueueHandle, msg, 0, UART_TX_TIMEOUT);
	}
}
