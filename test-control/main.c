/**
 * @file main.c
 * @brief Entry point for the test client.
 */

#include "common.h"
#include "interface.h"
#include "db.h"

int main(void)
{
    initialize_signal_handler();
    interface_init();
    db_init();
    interface_loop();
    db_deinit();

    return 0;
}
