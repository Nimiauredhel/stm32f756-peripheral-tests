#include "interface.h"

char test_names[NUM_POSSIBLE_TESTS][8] =
{
    "TIMER\0", "UART\0","SPI\0", "I2C\0", "ADC\0",
};

void interface_init(void)
{
    /*
    char server_ip_str[24] = {0};
    printf("Please input server IP.\n");
    fgets(server_ip_str, sizeof(server_ip_str), stdin);

    if (strlen(server_ip_str) < 7)
    {
        printf("Defaulting to localhost.\n");
        sprintf(server_ip_str, "127.0.0.1");
    }
    */

    client_init();
}

void interface_loop(void)
{
    static bool selection_valid = false;
    static char numeric_input_buff[4] = {0};
    static char test_str_buff[TEST_PACKET_STR_MAX_LEN] = {0};
    static int numeric_input_int = 0;
    static uint8_t test_str_len = 0;
    static uint8_t test_selection_byte = 0;
    static uint8_t test_iterations_byte = 0;

    while(!should_terminate)
    {
        while (!client_is_paired() && !should_terminate)
        {
            client_try_pairing();
        }

        if (should_terminate) break;

        // resetting all values asked from user
        explicit_bzero(test_str_buff, sizeof(test_str_buff));
        explicit_bzero(numeric_input_buff, sizeof(numeric_input_buff));

        test_str_len = 0;
        test_selection_byte = 0;
        test_iterations_byte = 0;

        printf("Please input a test string.\n");
        fgets(test_str_buff, sizeof(test_str_buff), stdin);

        test_str_len = strlen(test_str_buff);

        while (test_str_buff[test_str_len-1] == '\r'
            || test_str_buff[test_str_len-1] == '\n'
            || test_str_buff[test_str_len-1] == ' ')
        {
            test_str_buff[test_str_len-1] = '\0';
            test_str_len -= 1;
        }

        printf("Given input:\n %s\n\n", test_str_buff);

        selection_valid = false;

        while (!selection_valid)
        {
            if (should_terminate) break;

            printf("Select a test combination [TIMER|UART|SPI|I2C|ADC]:\n");
            fgets(numeric_input_buff, sizeof(numeric_input_buff), stdin);
            numeric_input_int = atoi(numeric_input_buff);

            selection_valid = numeric_input_int > 0 && numeric_input_int <= 31;
        }

        test_selection_byte = (uint8_t)numeric_input_int;

        if (should_terminate)
        {
            break;
        }

        selection_valid = false;

        while (!selection_valid)
        {
            if (should_terminate) break;

            printf("Select number of test iterations (max 255).\n");
            fgets(numeric_input_buff, sizeof(numeric_input_buff), stdin);
            numeric_input_int = atoi(numeric_input_buff);

            selection_valid = numeric_input_int > 0 && numeric_input_int <= UINT8_MAX;
        }

        test_iterations_byte = (uint8_t)numeric_input_int;

        if (should_terminate)
        {
            break;
        }

        client_fill_test_request_packet(TESTMSG_TEST_NEW_REQUEST, (next_test_id_client_half << 16), test_selection_byte, test_iterations_byte, test_str_len, test_str_buff);
        next_test_id_client_half = (next_test_id_client_half == UINT16_MAX) ? 0 : next_test_id_client_half + 1;

        if(client_send_test_request_packet())
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
