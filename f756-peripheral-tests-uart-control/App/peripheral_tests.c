/*
 * peripheral_tests.c
 *
 *  Created on: Jul 7, 2025
 *      Author: mickey
 */
#include "peripheral_tests.h"
#include <math.h>

static bool test_uart(const char *test_string, const uint8_t len);
static bool test_timer(const char *test_string, const uint8_t len);
static bool test_spi(const char *test_string, const uint8_t len);
static bool test_i2c(const char *test_string, const uint8_t len);
static bool test_adc(const char *test_string, const uint8_t len);

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef htim1;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi5;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;

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
	static const uint8_t reps = 8;

	for (int i = 0; i < reps; i++)
	{
		uint32_t duty_val = htim1.Instance->ARR / pow(2, i+1);
		htim1.Instance->CCR3 = duty_val;
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
		HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_2);
		vTaskDelay(pdMS_TO_TICKS(50));
		HAL_TIM_IC_Stop(&htim1, TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
		uint32_t ic_result = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_2);

		/*
		char ic_buff[64] = {0};
		snprintf(ic_buff, 64, "Expected: %lu, IC value: %lu", duty_val, ic_result);
		serial_print_line(ic_buff, 0);
		*/

		uint32_t margin = ic_result > duty_val ? ic_result - duty_val
				: duty_val - ic_result;

		if (margin > 10) return false;
	}

	return true;
}

static bool test_uart(const char *test_string, const uint8_t len)
{
	static char uart_test_rx_buff_1[TEST_STRING_MAX_LEN] = {0};
	static char uart_test_rx_buff_2[TEST_STRING_MAX_LEN] = {0};

	bzero(uart_test_rx_buff_1, sizeof(uart_test_rx_buff_1));
	bzero(uart_test_rx_buff_2, sizeof(uart_test_rx_buff_2));

	HAL_UART_Receive_DMA(&huart6, (uint8_t *)uart_test_rx_buff_1, len);
	HAL_UART_Transmit(&huart2, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	if (0 != strcmp(test_string, uart_test_rx_buff_1)) return false;

	HAL_UART_Receive_DMA(&huart2, (uint8_t *)uart_test_rx_buff_2, len);
	HAL_UART_Transmit(&huart6, (uint8_t *)uart_test_rx_buff_1, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	return (0 == strcmp(test_string, uart_test_rx_buff_2));
}

static bool test_spi(const char *test_string, const uint8_t len)
{
	static char spi_rx_buff_1[TEST_STRING_MAX_LEN] = {0};
	static char spi_rx_buff_2[TEST_STRING_MAX_LEN] = {0};
	static char spi_rx_buff_dummy[TEST_STRING_MAX_LEN] = {0};
	static char spi_tx_buff_dummy[TEST_STRING_MAX_LEN] = {0};

	bzero(spi_rx_buff_1, sizeof(spi_rx_buff_1));
	bzero(spi_rx_buff_2, sizeof(spi_rx_buff_2));

	HAL_SPI_Receive_DMA(&hspi5, (uint8_t *)spi_rx_buff_1, len);
	HAL_SPI_Transmit(&hspi3, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	if (0 != strcmp(test_string, spi_rx_buff_1)) return false;

	HAL_SPI_TransmitReceive_DMA(&hspi5, (uint8_t *)spi_rx_buff_1, (uint8_t *)spi_rx_buff_dummy, len);
	HAL_SPI_TransmitReceive(&hspi3, (uint8_t *)spi_tx_buff_dummy, (uint8_t *)spi_rx_buff_2, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	return (0 == strcmp(test_string, spi_rx_buff_2));
}

static bool test_i2c(const char *test_string, const uint8_t len)
{
	static char i2c_rx_buff_1[TEST_STRING_MAX_LEN] = {0};
	static char i2c_rx_buff_2[TEST_STRING_MAX_LEN] = {0};

	bzero(i2c_rx_buff_1, sizeof(i2c_rx_buff_1));
	bzero(i2c_rx_buff_2, sizeof(i2c_rx_buff_2));

	HAL_I2C_Slave_Receive_DMA(&hi2c1, (uint8_t *)i2c_rx_buff_1, len);
	HAL_I2C_Master_Transmit(&hi2c2, hi2c1.Init.OwnAddress1, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	if (0 != strcmp(test_string, i2c_rx_buff_1)) return false;

	HAL_I2C_Slave_Transmit_DMA(&hi2c1, (uint8_t *)i2c_rx_buff_1, len);
	HAL_I2C_Master_Receive(&hi2c2, hi2c1.Init.OwnAddress1, (uint8_t *)i2c_rx_buff_2, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	return (0 == strcmp(test_string, i2c_rx_buff_2));
}

static bool test_adc(const char *test_string, const uint8_t len)
{
	static const uint32_t adc_min_val = 4000;
	static const uint32_t adc_max_val = 4095;

	uint32_t adc_val = 0;

	HAL_ADC_Start_DMA(&hadc1, &adc_val, 1);
	HAL_ADC_PollForConversion(&hadc1, TEST_TIMEOUT_TICKS);
	HAL_ADC_Stop_DMA(&hadc1);

	return (adc_val >= adc_min_val && adc_val <= adc_max_val);
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
