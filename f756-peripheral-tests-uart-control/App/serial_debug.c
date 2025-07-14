/*
 * uart_io.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include <serial_debug.h>

#define UART_PEER huart3
#define UART_TX_TIMEOUT pdMS_TO_TICKS(2000)

extern UART_HandleTypeDef huart3;
extern osMessageQueueId_t DebugQueueHandle;

static void serial_print_line(const char *msg, uint16_t len)
{
	static const uint8_t newline[2] = {'\r', '\n'};

	// a NULL message is valid as a request to just print a newline
	if (msg != NULL)
	{
		if (len == 0) len = strlen(msg);
		HAL_UART_Transmit(&UART_PEER, (uint8_t *)msg, len, UART_TX_TIMEOUT);
	}
	HAL_UART_Transmit(&UART_PEER, newline, 2, UART_TX_TIMEOUT);
}

void serial_debug_initialize()
{
}

void serial_debug_loop()
{
	char debug_buff[256] = {0};

	for(;;)
	{
		if (osOK == osMessageQueueGet(DebugQueueHandle, debug_buff, 0, UART_TX_TIMEOUT))
		{
			serial_print_line(debug_buff, 0);
		}
	}
}

void serial_debug_enqueue(const char *msg)
{
	osMessageQueuePut(DebugQueueHandle, msg, 0, UART_TX_TIMEOUT);
}
