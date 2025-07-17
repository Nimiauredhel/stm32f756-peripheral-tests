#include "common.h"

const char test_names[NUM_POSSIBLE_TESTS][8] =
{
    "TIMER\0", "UART\0","SPI\0", "I2C\0", "ADC\0",
};

uint16_t next_test_id_client_half = 0;
TerminationReason_t why_terminate = TERMR_UNKNOWN;
bool should_terminate = false;

void initialize_signal_handler(void)
{
    should_terminate = false;

    struct sigaction action;
    bzero(&action, sizeof(action));
    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
}

void signal_handler(int signum)
{
    switch (signum)
    {
        case SIGINT:
        case SIGTERM:
        case SIGHUP:
            why_terminate = TERMR_SIGNAL;
            should_terminate = true;
            break;
    }
}

float seconds_since_clock(struct timespec start_clock)
{
    struct timespec now_clock;
    clock_gettime(CLOCK_MONOTONIC, &now_clock);
    float elapsed_float = (now_clock.tv_nsec - start_clock.tv_nsec) / 1000000000.0;
    elapsed_float += (now_clock.tv_sec - start_clock.tv_sec);
    return elapsed_float;
}

void datetime_str_nonalloc(char *buff, size_t maxlen)
{
    if (buff == NULL) return;

    time_t current_time = time(NULL);
    strftime(buff, maxlen, "%Y/%m/%d %H:%M:%S", localtime(&current_time));
}
