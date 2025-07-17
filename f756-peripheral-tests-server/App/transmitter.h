/*
 * transmitter.h
 *
 *  Created on: Jul 14, 2025
 *      Author: User
 */

/**
 * @file transmitter.h
 * @brief Header file for the 'transmitter' (ethernet out) task functionality.
 */

#ifndef TRANSMITTER_H_
#define TRANSMITTER_H_

void transmitter_task_init(void);
void transmitter_task_loop(void);

#endif /* TRANSMITTER_H_ */
