#include "interface.h"

char test_names[NUM_POSSIBLE_TESTS][8] =
{
    "TIMER\0", "UART\0","SPI\0", "I2C\0", "ADC\0",
};

void interface_init(void)
{
    char server_ip_str[24] = {0};
    printf("Please input server IP.\n");
    fgets(server_ip_str, 24, stdin);

    if (strlen(server_ip_str) < 7)
    {
        printf("Defaulting to localhost.\n");
        sprintf(server_ip_str, "127.0.0.1");
    }

    client_init(server_ip_str);
}

void interface_loop(void)
{
    static char user_input_buffer[TEST_PACKET_STR_MAX_LEN] = {0};

    while(!should_terminate)
    {
        printf("Please input a test string.\n");
        explicit_bzero(user_input_buffer, sizeof(user_input_buffer));
        fgets(user_input_buffer, sizeof(user_input_buffer), stdin);
        printf("Given input: %s\n", user_input_buffer);

        bool selection_valid = false;
        char test_selection_input[4] = {0};
        uint8_t test_selection_byte = 0;

        while (!selection_valid)
        {
            if (should_terminate) break;

            printf("\nSelect a test combination [TIMER|UART|SPI|I2C|ADC]:\n");
            fgets(test_selection_input, 4, stdin);
            test_selection_byte = (uint8_t)atoi(test_selection_input);

            selection_valid = test_selection_byte > 0 && test_selection_byte <= 31;
        }

        if (should_terminate)
        {
            break;
        }

        client_fill_packet(TESTMSG_NEWTEST, 1234, test_selection_byte, strlen(user_input_buffer), user_input_buffer);

        if(client_send_packet())
        {
            printf("Sent test request.\n");
            client_await_response(test_selection_byte);
        }
        else
        {
            printf("Failed to send test request.\n");
        }
    }

    printf("\nTerminated by user.\n");
    client_deinit();
}
