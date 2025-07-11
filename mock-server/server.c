#include "server.h"

#define SERVER_PORT (45678)

static int sockfd = 0;
static struct sockaddr_in server_addr;
static socklen_t server_addr_len = sizeof(server_addr);
static uint8_t tx_buffer[TEST_PACKET_SIZE_BYTES] = {0};

static bool server_send_packet(void)
{
    int sent_bytes = sendto(sockfd, tx_buffer, sizeof(tx_buffer), 0, (struct sockaddr*)&server_addr, server_addr_len);

    if (sent_bytes <= 0)
    {
        perror("sendto failed");
        return false;
    }

    return true;
}

static void server_fill_packet(TestPacketMsg_t msg, uint32_t test_id, uint8_t test_selection, uint8_t str_len, char *str_ptr)
{
    explicit_bzero(tx_buffer, sizeof(tx_buffer));
    tx_buffer[0] = TEST_PACKET_START_BYTE_VALUE;
    tx_buffer[TEST_PACKET_MSG_BYTE_OFFSET] = msg;
    tx_buffer[TEST_PACKET_ID_BYTE_OFFSET] = test_id;
    tx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET] = test_selection;
    tx_buffer[TEST_PACKET_STRING_LEN_OFFSET] = str_len;

    if (str_len > 0 && str_ptr != NULL)
    {
        strncpy((char *)(tx_buffer+TEST_PACKET_STRING_HEAD_OFFSET), str_ptr, str_len);
    }
    else
    {
        explicit_bzero(tx_buffer+TEST_PACKET_STRING_HEAD_OFFSET, TEST_PACKET_STR_MAX_LEN);
    }

    tx_buffer[TEST_PACKET_SIZE_BYTES-1] = TEST_PACKET_END_BYTE_VALUE;
}

static void server_listen(void)
{
    static struct sockaddr_in client_addr = {0};
    static socklen_t client_addr_len = sizeof(server_addr);
    static uint8_t rx_buffer[TEST_PACKET_SIZE_BYTES] = {0};

    printf("Listening...\n");

    explicit_bzero(rx_buffer, sizeof(rx_buffer));
    size_t received_bytes = recvfrom(sockfd, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*)&server_addr, &server_addr_len);

    if (received_bytes <= 0)
    {
        perror("Receiving failed");
    }
    else if (rx_buffer[0] == TEST_PACKET_START_BYTE_VALUE && rx_buffer[TEST_PACKET_SIZE_BYTES -1] == TEST_PACKET_END_BYTE_VALUE)
    {
        switch((TestPacketMsg_t)rx_buffer[TEST_PACKET_MSG_BYTE_OFFSET])
        {
        case TESTMSG_NEWTEST:
            printf("Received test request.\n");
            server_fill_packet(TESTMSG_ACK, rx_buffer[TEST_PACKET_ID_BYTE_OFFSET], rx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET], 0, NULL);
            server_send_packet();
            sleep(3);
            server_fill_packet(TESTMSG_RESULT, rx_buffer[TEST_PACKET_ID_BYTE_OFFSET], rx_buffer[TEST_PACKET_SELECTION_BYTE_OFFSET], 0, NULL);
            server_send_packet();
            break;
        case TESTMSG_NONE:
        case TESTMSG_ACK:
        case TESTMSG_RESULT:
        default:
            printf("Received unexpected packet.\n");
            break;
        }
    }
    else
    {
        printf("Received invalid packet.\n");
    }
}

void server_init(void)
{
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd, (struct sockaddr*)&server_addr, server_addr_len)  < 0)
    {
        perror("Socket binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

}

void server_loop(void)
{
    while(!should_terminate)
    {
        server_listen();
    }
}

void server_deinit(void)
{
    if (sockfd > 0) close(sockfd);
}
