/**
 * @file common.c
 * @brief Source file for common variables and functions used by different parts of the test client module.
 */

#include "common.h"

#define PERSISTENCE_FILE_PATH ("persistence.dat")

const char test_names[NUM_POSSIBLE_TESTS][8] =
{
    "TIMER\0", "UART\0","SPI\0", "I2C\0", "ADC\0",
};

uint16_t last_test_id_client_half = 0;
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

void save_last_client_test_id(void)
{
    FILE *file;

    printf("Saving test ID %04x____ to file...\n", last_test_id_client_half);

    file = fopen(PERSISTENCE_FILE_PATH, "wb");

    if (file == NULL)
    {
        perror("Error opening file for writing");
        return;
    }

    size_t write_count = fwrite(&last_test_id_client_half, 2, 1, file);

    if (write_count == 1)
    {
        printf("Saved test ID %04x____ to file.\n", last_test_id_client_half);
    }
    else
    {
        perror("Error writing to file");
    }

    fclose(file);
}

void load_last_client_test_id(void)
{
    static const char defaulting_str[] = "Defaulting last test ID to 0x0000____.\n";

    FILE *file = NULL;
    int error_code = 0;
    size_t read_count = 0;
    uint16_t id = 0;

    printf("Loading last saved test ID...\n");

    file = fopen(PERSISTENCE_FILE_PATH, "rb");

    if (file == NULL)
    {
        error_code = errno;

        if (error_code == ENOENT)
        {
            printf("No saved data found.\n");
        }
        else
        {
            printf("Failed to open saved data. Error: %s\n", strerror(error_code));
        }

        printf(defaulting_str);
    }
    else
    {
        read_count = fread(&id, 2, 1, file);

        if (read_count == 1)
        {
            printf("Loaded last test ID: 0x%04X____.\n", id);
        }
        else
        {
            error_code = errno;
            printf("Failed to read saved data. Error: %s\n", strerror(error_code));
            printf(defaulting_str);
            id = 0;
        }

        fclose(file);
    }

    last_test_id_client_half = id;
}
