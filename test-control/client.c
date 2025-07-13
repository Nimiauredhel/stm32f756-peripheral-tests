#include "client.h"

#define SERVER_PORT (45678)

static int sockfd = 0;
static struct sockaddr_in server_addr;
static socklen_t server_addr_len = sizeof(server_addr);
static uint8_t tx_buffer[TEST_PACKET_MAX_SIZE_BYTES] = {0};
static uint8_t rx_buffer[TEST_PACKET_MAX_SIZE_BYTES] = {0};

void client_await_response(uint8_t test_selection_byte)
{
    bool results_received = false;

    while (!should_terminate && !results_received)
    {
        size_t received_bytes = recvfrom(sockfd, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*)&server_addr, &server_addr_len);

        if (received_bytes <= 0)
        {
            perror("Receiving failed");
        }
        else if (rx_buffer[0] == TEST_PACKET_START_BYTE_VALUE)
        {
            switch((TestPacketMsg_t)rx_buffer[TEST_PACKET_MSG_BYTE_OFFSET])
            {
            case TESTMSG_ACK:
                printf("Received acknowledgement from device.\n");
                break;
            case TESTMSG_RESULT:
                results_received = true;
                printf("Received results.\n");

                for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
                {
                    if (0x01 & (test_selection_byte >> (uint8_t)i))
                    {
                        printf("%s Test ", test_names[i]);

                        if (0x01 & (rx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] >> (uint8_t)i))
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
            case TESTMSG_NONE:
            case TESTMSG_NEWTEST:
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

bool client_send_packet(void)
{
    uint8_t packet_size = TEST_PACKET_EMPTY_SIZE_BYTES + tx_buffer[TEST_PACKET_STRING_LEN_OFFSET];
    int sent_bytes = sendto(sockfd, tx_buffer, packet_size, 0, (struct sockaddr*)&server_addr, server_addr_len);

    if (sent_bytes <= 0)
    {
        perror("sendto failed");
        return false;
    }

    printf("Sent test packet of size %u.\n", packet_size);

    return true;
}

void client_init(char *server_ip_str)
{
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (0 == inet_aton(server_ip_str, &server_addr.sin_addr))
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

void client_fill_packet(TestPacketMsg_t msg, uint32_t test_id, uint8_t test_selection, uint8_t iterations, uint8_t str_len, char *str_ptr)
{
    explicit_bzero(tx_buffer, sizeof(tx_buffer));
    tx_buffer[0] = TEST_PACKET_START_BYTE_VALUE;
    tx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] = msg;
    *(uint32_t *)(tx_buffer+TEST_PACKET_ID_BYTE_OFFSET) = test_id;
    tx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] = test_selection;
    tx_buffer[TEST_PACKET_ITERATIONS_BYTE_OFFSET] = iterations;
    tx_buffer[TEST_PACKET_STRING_LEN_OFFSET] = str_len;

    if (str_len > 0 && str_ptr != NULL)
    {
        strncpy((char *)(tx_buffer+TEST_PACKET_STRING_HEAD_OFFSET), str_ptr, str_len);
    }
    else
    {
        explicit_bzero(tx_buffer+TEST_PACKET_STRING_HEAD_OFFSET, TEST_PACKET_STR_MAX_LEN);
    }

    tx_buffer[TEST_PACKET_STRING_HEAD_OFFSET+str_len] = TEST_PACKET_END_BYTE_VALUE;
}
