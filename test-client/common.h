/**
 * @file common.h
 * @brief Header file for common variables and functions used by different parts of the test client module.
 */

#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "test_packet_def.h"

typedef enum TerminationReason
{
    TERMR_UNKNOWN = 0,
    TERMR_SIGNAL = 1,
    TERMR_ERROR = 2,
} TerminationReason_t;

/**
 * @brief Names of the possible tests, for the user interface.
 */
extern const char test_names[NUM_POSSIBLE_TESTS][8];

/**
 * @brief Next 16-bit value to be used as the left half of a 32-bit Test ID.
 */
extern uint16_t next_test_id_client_half;

/**
 * @brief Global flag indicating reason for program termination.
 */
extern TerminationReason_t why_terminate;

/**
 * @brief Global flag set by OS termination signals
 * and polled by functions to allow graceful termination.
 */
extern bool should_terminate;

/**
 * @brief Hooks up OS signals to our custom handler.
 */
void initialize_signal_handler(void);

/**
 * @brief Handles selected OS signals.
 * @details
 * When termination signals are caught, this sets the should_terminate flag,
 * which signals running functions that they should attempt graceful termination.
 */
void signal_handler(int signum);

/**
 * @brief Returns the seconds elapsed since a given clock value.
 * Used for timing operations!
 */
float seconds_since_clock(struct timespec start_clock);

/**
 * @brief Gets the current date and time and writes the string into [buff].
 */
void datetime_str_nonalloc(char *buff, size_t maxlen);

#endif
