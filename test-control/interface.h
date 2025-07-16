/**
 * @file interface.h
 * @brief Header file for the test client module's user interface and flow control functions.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "common.h"
#include "client.h"

/**
 * @brief Initializes the user interface and control flow.
 * Currently does nothing but call @ref client_init().
 */
void interface_init(void);
/**
 * @brief Core interaction loop of the test client.
 * @details
 * The interface loop prompts the user for test request parameters,
 * and prints responses from the server if any.
 *
 * Repeats until terminated by user action (typically Ctrl-C).
 */
void interface_loop(void);

#endif
