#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "networking_common.h"
#include "test_packet_def.h"
#include "interface.h"

void client_init(void);
void client_deinit(void);

void client_fill_pairing_packet(TestPacketMsg_t msg);
void client_fill_test_message_packet(TestPacketMsg_t msg, uint32_t test_id);
void client_fill_test_request_packet(TestPacketMsg_t msg, uint32_t test_id, uint8_t test_selection, uint8_t iterations, uint8_t str_len, char *str_ptr);

void client_try_pairing(void);
bool client_is_paired(void);

bool client_send_test_message_packet(void);
bool client_send_test_request_packet(void);

void client_await_response(uint8_t test_selection_byte);

#endif
