#ifndef INTERFACE_H
#define INTERFACE_H

#include "common.h"
#include "test_packet_def.h"
#include "client.h"

extern char test_names[NUM_POSSIBLE_TESTS][8];

void interface_loop(void);

#endif
