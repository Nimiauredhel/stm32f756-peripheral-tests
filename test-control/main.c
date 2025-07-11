#include "common.h"
#include "interface.h"

int main(void)
{
    initialize_signal_handler();
    interface_init();
    interface_loop();

    return 0;
}
