#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "networking_common.h"
#include "test_packet_def.h"
#include "interface.h"

void client_init(char *server_ip_str);
void client_deinit(void);
void client_fill_packet(TestPacketMsg_t msg, uint32_t test_id, uint8_t test_selection, uint8_t str_len, char *str_ptr);
bool client_send_packet(void);
void client_await_response(uint8_t test_selection_byte);

#endif
