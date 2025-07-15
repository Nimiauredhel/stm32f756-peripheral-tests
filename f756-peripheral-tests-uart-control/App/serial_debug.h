/*
 * uart_io.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef SERIAL_DEBUG_H_
#define SERIAL_DEBUG_H_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os2.h"

#include "main.h"

#define SERIAL_DEBUG_ENABLED (1)
#define SERIAL_DEBUG_MAX_LEN 150

void serial_debug_initialize();
void serial_debug_loop();
void serial_debug_enqueue(const char *msg);

#endif /* SERIAL_DEBUG_H_ */
