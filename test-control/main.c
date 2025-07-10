#include "common.h"
#include "networking_common.h"
#include "test_packet_def.h"

#define SERVER_PORT (45678)
#define NUM_POSSIBLE_TESTS (5)

static int sockfd = 0;
static struct sockaddr_in server_addr;
static socklen_t server_addr_len = sizeof(server_addr);
static char user_input_buffer[TEST_PACKET_STR_MAX_LEN] = {0};
static uint8_t tx_buffer[TEST_PACKET_SIZE_BYTES] = {0};
static uint8_t rx_buffer[TEST_PACKET_SIZE_BYTES] = {0};

static char test_names[NUM_POSSIBLE_TESTS][8] =
{
    "TIMER\0", "UART\0","SPI\0", "I2C\0", "ADC\0",
};

static void listen_for_responses(uint8_t test_selection_byte)
{
    bool results_received = false;

    while (!should_terminate && !results_received)
    {
        size_t received_bytes = recvfrom(sockfd, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*)&server_addr, &server_addr_len);

        if (received_bytes <= 0)
        {
            perror("Receiving failed");
        }
        else if (rx_buffer[0] == TEST_PACKET_START_BYTE_VALUE && rx_buffer[TEST_PACKET_SIZE_BYTES -1] == TEST_PACKET_END_BYTE_VALUE)
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

static bool send_test_packet(void)
{
    int sent_bytes = sendto(sockfd, tx_buffer, sizeof(tx_buffer), 0, (struct sockaddr*)&server_addr, server_addr_len);

    if (sent_bytes <= 0)
    {
        perror("Sending failed");
        return false;
    }

    return true;
}

static void init_udp(void)
{
    char server_ip_str[16] = {0};

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    printf("Please input server IP.\n");
    fgets(server_ip_str, 8, stdin);

    if (0 == inet_aton(server_ip_str, &server_addr.sin_addr))
    {
        printf("Failed parsing IP.\n");
        exit(EXIT_FAILURE);
    }

    if (should_terminate) return;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

}

static void init_packet(void)
{
    explicit_bzero(tx_buffer, sizeof(tx_buffer));
    tx_buffer[0] = TEST_PACKET_START_BYTE_VALUE;
    tx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_NEWTEST;
    tx_buffer[TEST_PACKET_SIZE_BYTES-1] = TEST_PACKET_END_BYTE_VALUE;
}

int main(void)
{
    initialize_signal_handler();
    init_udp();

    while(!should_terminate)
    {
        init_packet();

        printf("Please input a test string.\n");
        explicit_bzero(user_input_buffer, sizeof(user_input_buffer));
        fgets(user_input_buffer, sizeof(user_input_buffer), stdin);
        printf("Given input: %s\n", user_input_buffer);

        bool selection_valid = false;
        char test_selection_input[2] = {0};
        uint8_t test_selection_byte = 0;

        while (!selection_valid)
        {
            if (should_terminate) goto terminate;

            printf("\nSelect a test combination [TIMER|UART|SPI|I2C|ADC]:\n");
            fgets(test_selection_input, 2, stdin);
            test_selection_byte = atoi(test_selection_input);

            selection_valid = test_selection_byte > 0 && test_selection_byte <= 7;
        }

        tx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] = test_selection_byte;
        tx_buffer[TEST_PACKET_STRING_LEN_OFFSET] = strlen(user_input_buffer);

        snprintf((char *)(tx_buffer + TEST_PACKET_STRING_HEAD_OFFSET), TEST_PACKET_STR_MAX_LEN, "%s", user_input_buffer);

        if(send_test_packet())
        {
            printf("Sent Test Request.\n");
            listen_for_responses(test_selection_byte);
        }
    }

terminate:
    close(sockfd);

    printf("\nTerminated by user.\n");

    return EXIT_SUCCESS;
}
