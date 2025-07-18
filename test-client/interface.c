#include "common.h"
#include "client.h"
#include "interface.h"

void interface_init(void)
{
    load_last_client_test_id();
    client_init();
}

void interface_loop(void)
{
    static bool selection_valid = false;
    static char short_input_buff[4] = {0};
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
        explicit_bzero(short_input_buff, sizeof(short_input_buff));

        test_str_len = 0;
        test_selection_byte = 0;
        test_iterations_byte = 0;

        printf("\nPlease input a test string (or Ctrl-c to quit).\nInput: ");
        fflush(stdout);
        fgets(test_str_buff, sizeof(test_str_buff), stdin);

        test_str_len = strlen(test_str_buff);

        while (test_str_buff[test_str_len-1] == '\r'
            || test_str_buff[test_str_len-1] == '\n'
            || test_str_buff[test_str_len-1] == ' ')
        {
            test_str_buff[test_str_len-1] = '\0';
            test_str_len -= 1;
        }

        printf("Given input: [%s]\n", test_str_buff);


        for (uint8_t i = 0; i < NUM_POSSIBLE_TESTS; i++)
        {
            if (should_terminate) break;
            selection_valid = false;

            while (!selection_valid && !should_terminate)
            {
                printf("Test %s? (y/n): ", test_names[i]);
                fflush(stdout);
                fgets(short_input_buff, sizeof(short_input_buff), stdin);

                switch(short_input_buff[0])
                {
                    case 'y':
                    case 'Y':
                        test_selection_byte |= (uint8_t)1 << (uint8_t)i;
                        // intentional fallthrough, thx
                    case 'n':
                    case 'N':
                        selection_valid = true;
                        break;
                    default:
                        break;
                }

                printf("\n");
            }
        }

        if (test_selection_byte == 0)
        {
            printf("No tests....? \n");
            continue;
        }

        selection_valid = false;

        while (!selection_valid)
        {
            if (should_terminate) break;

            printf("Select number of test iterations (1-255): ");
            fflush(stdout);
            fgets(short_input_buff, sizeof(short_input_buff), stdin);
            numeric_input_int = atoi(short_input_buff);

            selection_valid = numeric_input_int > 0 && numeric_input_int <= UINT8_MAX;
            printf("\n");
        }

        test_iterations_byte = (uint8_t)numeric_input_int;

        if (should_terminate)
        {
            break;
        }

        last_test_id_client_half = (last_test_id_client_half == UINT16_MAX) ? 1 : last_test_id_client_half + 1;
        client_fill_test_request_packet(TESTMSG_TEST_NEW_REQUEST, (last_test_id_client_half << 16), test_selection_byte, test_iterations_byte, test_str_len, test_str_buff);
        save_last_client_test_id();

        if(client_send_test_request_packet())
        {
            printf("Sent test request.\n");
            client_save_test_request();
            client_await_response();
        }
        else
        {
            printf("Failed to send test request.\n");
        }
    }

    client_deinit();
}
