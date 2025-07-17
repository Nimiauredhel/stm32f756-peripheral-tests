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

    switch (why_terminate)
    {
    case TERMR_UNKNOWN:
        printf("Termination reason unknown.\n");
        return 0;
    case TERMR_SIGNAL:
        printf("Terminated by signal.\n");
        return 0;
    case TERMR_ERROR:
        printf("Terminated following error.\n");
        return 1;
    }
}
