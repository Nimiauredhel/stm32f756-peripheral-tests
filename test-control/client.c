#include "client.h"

#define SERVER_PORT (45678)

static int sockfd = 0;
static struct sockaddr_in server_rx_addr;
static socklen_t server_rx_addr_len = sizeof(server_rx_addr);

static uint8_t client_tx_buffer[TEST_REQUEST_PACKET_MAX_SIZE_BYTES] = {0};
static uint8_t client_rx_buffer[TEST_MSG_PACKET_SIZE_BYTES] = {0};

static bool client_send_packet(uint8_t *buffer, size_t length)
{
    int sent_bytes = sendto(sockfd, buffer, length, 0, (struct sockaddr*)&server_rx_addr, server_rx_addr_len);

    if (sent_bytes <= 0)
    {
        perror("sendto failed");
        return false;
    }

    printf("Sent packet of size %lu.\n", length);

    return true;
}

bool client_send_pairing_packet(void)
{
    return client_send_packet(client_tx_buffer, PAIRING_PACKET_SIZE_BYTES);
}

bool client_send_test_message_packet(void)
{
    return client_send_packet(client_tx_buffer, TEST_MSG_PACKET_SIZE_BYTES);
}

bool client_send_test_request_packet(void)
{
    return client_send_packet(client_tx_buffer, TEST_REQUEST_PACKET_MIN_SIZE_BYTES + client_tx_buffer[TEST_PACKET_STRING_LEN_OFFSET]);
}

void client_await_response(uint8_t test_selection_byte)
{
    struct sockaddr_in server_tx_addr = server_rx_addr;
    socklen_t server_tx_addr_len = sizeof(server_tx_addr);
    bool results_received = false;

    while (!should_terminate && !results_received)
    {
        size_t received_bytes = recvfrom(sockfd, client_rx_buffer, sizeof(client_rx_buffer), 0, (struct sockaddr*)&server_tx_addr, &server_tx_addr_len);

        if (received_bytes <= 0)
        {
            perror("Receiving failed");
        }
        else if (client_rx_buffer[0] == TEST_PACKET_START_BYTE_VALUE)
        {
            switch((TestPacketMsg_t)client_rx_buffer[TEST_PACKET_MSG_BYTE_OFFSET])
            {
            case TESTMSG_TEST_NEW_ACK:
                printf("Device acknowledged test request, ID %u .\n", *(uint32_t *)(client_rx_buffer+TEST_PACKET_ID_BYTE_OFFSET));
                break;
            case TESTMSG_TEST_START_ACK:
                printf("Device acknowledged test ID %u started.\n", *(uint32_t *)(client_rx_buffer+TEST_PACKET_ID_BYTE_OFFSET));
                break;
            case TESTMSG_TEST_OVER_RESULTS:
                results_received = true;
                printf("Received test results for test ID %u.\n", *(uint32_t *)(client_rx_buffer+TEST_PACKET_ID_BYTE_OFFSET));

                for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
                {
                    if (0x01 & (test_selection_byte >> (uint8_t)i))
                    {
                        printf("%s Test ", test_names[i]);

                        if (0x01 & (client_rx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] >> (uint8_t)i))
                        {
                            printf("Passed.\n");
                        }
                        else
                        {
                            printf("Failed.\n");
                        }
                    }
                }

                break;
            case TESTMSG_FLAG_CLIENT:
            case TESTMSG_FLAG_SERVER:
            case TESTMSG_TEST_NEW_REQUEST:
            case TESTMSG_TEST_START_REQUEST:
            case TESTMSG_TEST_OVER_ACK:
            case TESTMSG_PAIRING_PROBE:
            case TESTMSG_PAIRING_BEACON:
            default:
                printf("Received unexpected response packet.\n");
                break;
            }
        }
        else
        {
            printf("Received invalid packet.\n");
        }
    }
}

void client_init(char *server_ip_str)
{
    bzero(&server_rx_addr, sizeof(server_rx_addr));
    server_rx_addr.sin_family = AF_INET;
    server_rx_addr.sin_port = htons(SERVER_PORT);

    if (0 == inet_aton(server_ip_str, &server_rx_addr.sin_addr))
    {
        printf("Failed parsing IP.\n");
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
}

void client_deinit(void)
{
    if (sockfd > 0) close(sockfd);
}

void client_fill_pairing_packet(TestPacketMsg_t msg)
{
    explicit_bzero(client_tx_buffer, TEST_MSG_PACKET_SIZE_BYTES);
    client_tx_buffer[0] = TEST_PACKET_START_BYTE_VALUE;
    client_tx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] = msg;
    client_tx_buffer[TEST_PACKET_ID_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
}

void client_fill_test_message_packet(TestPacketMsg_t msg, uint32_t test_id)
{
    explicit_bzero(client_tx_buffer, TEST_REQUEST_PACKET_MAX_SIZE_BYTES);
    client_tx_buffer[0] = TEST_PACKET_START_BYTE_VALUE;
    client_tx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] = msg;
    *(uint32_t *)(client_tx_buffer+TEST_PACKET_ID_BYTE_OFFSET) = test_id;
    client_tx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] = 0;
    client_tx_buffer[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
}

void client_fill_test_request_packet(TestPacketMsg_t msg, uint32_t test_id, uint8_t test_selection, uint8_t iterations, uint8_t str_len, char *str_ptr)
{
    explicit_bzero(client_tx_buffer, TEST_REQUEST_PACKET_MAX_SIZE_BYTES + str_len);
    client_tx_buffer[0] = TEST_PACKET_START_BYTE_VALUE;
    client_tx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] = msg;
    *(uint32_t *)(client_tx_buffer+TEST_PACKET_ID_BYTE_OFFSET) = test_id;
    client_tx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] = test_selection;
    client_tx_buffer[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = iterations;

    if (str_len > 0 && str_ptr != NULL)
    {
        client_tx_buffer[TEST_PACKET_STRING_LEN_OFFSET] = str_len;
        strncpy((char *)(client_tx_buffer+TEST_PACKET_STRING_HEAD_OFFSET), str_ptr, str_len);
        client_tx_buffer[TEST_PACKET_STRING_HEAD_OFFSET+str_len] = TEST_PACKET_END_BYTE_VALUE;
    }
    else
    {
        client_tx_buffer[TEST_PACKET_STRING_LEN_OFFSET] = 0;
        explicit_bzero(client_tx_buffer+TEST_PACKET_STRING_HEAD_OFFSET, TEST_PACKET_STR_MAX_LEN);
        client_tx_buffer[TEST_PACKET_STRING_HEAD_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
    }
}
