/*
 * peripheral_tests.c
 *
 *  Created on: Jul 7, 2025
 *      Author: mickey
 */
#include "peripheral_tests.h"

static bool test_uart(const char *test_string, const uint8_t len);
static bool test_timer(const char *test_string, const uint8_t len);
static bool test_spi(const char *test_string, const uint8_t len);
static bool test_i2c(const char *test_string, const uint8_t len);
static bool test_adc(const char *test_string, const uint8_t len);

volatile uint8_t test_string_len = 0;
char test_string_buff[TEST_STRING_MAX_LEN] = {0};

TestDefinition_t test_defs[NUM_POSSIBLE_TESTS] =
{
	{ .name = "Timer\0", .state = TESTSTATE_READY, .func = test_timer, },
	{ .name = "UART\0", .state = TESTSTATE_READY, .func = test_uart, },
	{ .name = "SPI\0", .state = TESTSTATE_READY, .func = test_spi, },
	{ .name = "I2C\0", .state = TESTSTATE_READY, .func = test_i2c, },
	{ .name = "ADC\0", .state = TESTSTATE_READY, .func = test_adc, },
};

static bool test_timer(const char *test_string, const uint8_t len)
{
	return false;
}

static bool test_uart(const char *test_string, const uint8_t len)
{
	char uart_test_rx_buff[TEST_STRING_MAX_LEN] = {0};

	HAL_UART_Receive_DMA(&huart6, (uint8_t *)uart_test_rx_buff, len);
	HAL_UART_Transmit(&huart2, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);

	return (0 == strcmp(test_string, uart_test_rx_buff));
}

static bool test_spi(const char *test_string, const uint8_t len)
{
	char spi_rx_buff[TEST_STRING_MAX_LEN] = {0};

	HAL_SPI_Receive_DMA(&hspi5, (uint8_t *)spi_rx_buff, len);
	HAL_SPI_Transmit(&hspi3, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);

	return (0 == strcmp(test_string, spi_rx_buff));
}

static bool test_i2c(const char *test_string, const uint8_t len)
{
	char i2c_rx_buff[TEST_STRING_MAX_LEN] = {0};

	HAL_I2C_Slave_Receive_DMA(&hi2c1, (uint8_t *)i2c_rx_buff, len);
	HAL_I2C_Master_Transmit(&hi2c2, hi2c1.Init.OwnAddress1, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);

	return (0 == strcmp(test_string, i2c_rx_buff));
}

static bool test_adc(const char *test_string, const uint8_t len)
{
	return false;
}

void test_task_loop(TestDefinition_t *def)
{
	for(;;)
	{
		vTaskDelay(pdMS_TO_TICKS(100));

		if (def->state == TESTSTATE_PENDING)
		{
			def->state = TESTSTATE_BUSY;

			def->state = def->func(test_string_buff, test_string_len)
			? TESTSTATE_SUCCESS : TESTSTATE_FAILURE;
		}
	}
}
