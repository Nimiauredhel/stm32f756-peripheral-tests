#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "networking_common.h"
#include "test_packet_def.h"

void server_init(void);
void server_loop(void);
void server_deinit(void);

#endif
