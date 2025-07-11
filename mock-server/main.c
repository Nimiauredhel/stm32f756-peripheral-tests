#include "common.h"
#include "server.h"

int main(void)
{
    initialize_signal_handler();
    server_init();
    server_loop();
    server_deinit();

    return 0;
}
