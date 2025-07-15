#include "client.h"

#define CLIENT_PORT (34567)
#define SERVER_PORT (45678)

static int sockfd = 0;
static struct sockaddr_in server_rx_addr;
static socklen_t server_rx_addr_len = sizeof(server_rx_addr);

static uint8_t client_tx_buffer[TEST_REQUEST_PACKET_MAX_SIZE_BYTES+1] = {0};
static uint8_t client_rx_buffer[TEST_REQUEST_PACKET_MIN_SIZE_BYTES+1] = {0};

static bool is_paired = false;

static bool client_send_packet(uint8_t *buffer, size_t length)
{
    ssize_t sent_bytes = sendto(sockfd, buffer, length, 0, (struct sockaddr*)&server_rx_addr, server_rx_addr_len);

    if (sent_bytes <= 0)
    {
        perror("sendto failed");
        return false;
    }

    printf("Sent packet of size %lu.\n", length);

    return true;
}

static bool client_send_pairing_packet(void)
{
    //inet_aton("192.168.1.255", &server_rx_addr.sin_addr);
    server_rx_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    server_rx_addr.sin_port = htons(SERVER_PORT);
    server_rx_addr.sin_family = AF_INET;

    printf("Sending a client probe.\n");
    return client_send_packet(client_tx_buffer, PAIRING_PACKET_SIZE_BYTES);
}

void client_try_pairing(void)
{
    static struct sockaddr_in new_server_addr = {0};
    static socklen_t new_server_addr_len = sizeof(new_server_addr);

    if (is_paired || should_terminate) return;

    client_fill_pairing_packet(TESTMSG_PAIRING_PROBE);

    printf("Awaiting a server beacon.\n");

    client_send_pairing_packet();

    while (!should_terminate && !is_paired)
    {
        ssize_t received_bytes = recvfrom(sockfd, client_rx_buffer, sizeof(client_rx_buffer)-1, 0, (struct sockaddr*)&new_server_addr, &new_server_addr_len);

        printf("Received packet of size %ld.\n", received_bytes);

        if (received_bytes <= 0)
        {
            int err = errno;
            if (err == ETIMEDOUT)
            {
                client_send_pairing_packet();
                continue;
            }
            perror("Receiving failed");
        }
        else if(new_server_addr.sin_port == CLIENT_PORT)
        {
            // most likely our broadcast
            continue;
        }
        else if (received_bytes == 3 && client_rx_buffer[0] == TEST_PACKET_START_BYTE_VALUE
                && client_rx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] == TESTMSG_PAIRING_BEACON
                && client_rx_buffer[TEST_PACKET_ID_BYTE_OFFSET] == TEST_PACKET_END_BYTE_VALUE)
        {
                server_rx_addr = new_server_addr;
                server_rx_addr.sin_port = htons(SERVER_PORT);
                server_rx_addr_len = new_server_addr_len;
                printf("Paired with server at IP %s !\n", inet_ntoa(server_rx_addr.sin_addr));
                is_paired = true;
        }
        else
        {
            printf("Received unexpected packet.\n");
            break;
        }
    }
}

bool client_is_paired(void)
{
    return is_paired;
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
        ssize_t received_bytes = recvfrom(sockfd, client_rx_buffer, sizeof(client_rx_buffer)-1, 0, (struct sockaddr*)&server_tx_addr, &server_tx_addr_len);

        if (received_bytes <= 0)
        {
            int err = errno;
            if (err == ETIMEDOUT) continue;
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

void client_init(void)
{
    static const struct timeval timeout =
    {
        .tv_sec = 4,
        .tv_usec = 0,
    };
    static const int one = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in bound_addr = {0};
    bound_addr.sin_addr.s_addr = INADDR_ANY;
    bound_addr.sin_port = htons(CLIENT_PORT);

    if (bind(sockfd, &bound_addr, sizeof(bound_addr)) < 0)
    {
        perror("Socket binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
    {
        perror("Setting socket timeout failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) < 0)
    {
        perror("Setting socket broadcast permission failed");
        close(sockfd);
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
    explicit_bzero(client_tx_buffer, TEST_REQUEST_PACKET_MAX_SIZE_BYTES);
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
