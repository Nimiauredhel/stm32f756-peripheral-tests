/**
 * @file client.h
 * @brief Header file for the test client module's networking functions.
 */

#ifndef CLIENT_H
#define CLIENT_H

/**
 * @brief Initializes a singular socket for the test client and binds it to the pre-determined client port.
 */
void client_init(void);
/**
 * @brief Releases resources acquired by the test client, if any. Currently just the one socket.
 */
void client_deinit(void);
/**
 * @brief Prepares a pairing probe packet in the outgoing packet buffer.
 */
void client_fill_pairing_packet(void);
/**
 * @brief Prepares a test message packet in the outgoing packet buffer.
 */
void client_fill_test_message_packet(TestPacketMsg_t msg, uint32_t test_id);
/**
 * @brief Prepares a test request packet in the outgoing packet buffer.
 */
void client_fill_test_request_packet(TestPacketMsg_t msg, uint16_t client_test_id, uint8_t test_selection, uint8_t iterations, uint8_t str_len, char *str_ptr);
/**
 * @brief Attempts to pair with a compatible testing server.
 */
void client_try_pairing(void);
/**
 * @brief Returns whether the client has already been paired with a testing server.
 */
bool client_is_paired(void);
/**
 * @brief Sends a portion of the outgoing packet buffer corresponding to the size of a test message packet.
 */
bool client_send_test_message_packet(void);
/**
 * @brief Sends a portion of the outgoing packet buffer corresponding to the total size of the contained test request packet.
 */
bool client_send_test_request_packet(void);
/**
 * @brief Copies the outgoing packet buffer to @ref latest_request_buffer for later reference,
 * and records the clock time into @ref latest_request_clock for later measurement.
 */
void client_save_test_request(void);
/**
 * @brief Listens for responses from a test server following the sending of a test request.
 */
void client_await_response(void);

#endif
