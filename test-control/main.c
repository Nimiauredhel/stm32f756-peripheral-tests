/**
 * @file main.c
 * @brief Entry point for the test client.
 */

#include "common.h"
#include "interface.h"

int main(void)
{
    initialize_signal_handler();
    interface_init();
    interface_loop();

    return 0;
}
