/**
 * @file db.h
 * @brief Header file for the test client module's database functions.
 */

#ifndef DB_H
#define DB_H

#include "common.h"

void db_init(void);
void db_deinit(void);
void db_append_request(uint8_t *request_buffer);
void db_append_results(uint8_t *results_buffer, uint8_t *request_buffer, float duration_secs);

#endif
