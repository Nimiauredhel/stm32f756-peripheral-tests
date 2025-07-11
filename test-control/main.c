#include "common.h"
#include "client.h"
#include "interface.h"

int main(void)
{
    initialize_signal_handler();
    client_init();
    interface_loop();
    client_deinit();

    return 0;
}
