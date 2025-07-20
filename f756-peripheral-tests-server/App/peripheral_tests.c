/*
 * peripheral_tests.c
 *
 *  Created on: Jul 7, 2025
 *      Author: mickey
 */

/**
 * @file peripheral_tests.c
 * @brief Source file in which the actual peripheral tests are implemented.
 */

#include <math.h>

#include "main.h"
#include "peripheral_tests.h"

/**
 * @brief The UART peripheral test implementation.
 * @param [in] *test_string The string used by the current peripheral test.
 * @param [in] len The length of the string used by the current peripheral test.
 */
static bool test_uart(const char *test_string, const uint8_t len);
/**
 * @brief The Timer peripheral test implementation.
 * @details
 * This function tests the Timer 1 Input Capture and PWM Generation capabilities,
 * using channel 2 and channel 3, respectively.
 * A series of PWM signals with different duty cycles are generated on channel 3,
 * and Channel 2 is expected to measure those duty cycle values within a reasonable tolerance.
 * @param [in] *test_string The string used by the current peripheral test.
 * @param [in] len The length of the string used by the current peripheral test.
 */
static bool test_timer(const char *test_string, const uint8_t len);
/**
 * @brief The SPI peripheral test implementation.
 * @param [in] *test_string The string used by the current peripheral test.
 * @param [in] len The length of the string used by the current peripheral test.
 */
static bool test_spi(const char *test_string, const uint8_t len);
/**
 * @brief The I2C peripheral test implementation.
 * @param [in] *test_string The string used by the current peripheral test.
 * @param [in] len The length of the string used by the current peripheral test.
 */
static bool test_i2c(const char *test_string, const uint8_t len);
/**
 * @brief The ADC peripheral test implementation.
 * @param [in] *test_string The string used by the current peripheral test.
 * @param [in] len The length of the string used by the current peripheral test.
 */
static bool test_adc(const char *test_string, const uint8_t len);

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef htim1;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi5;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;
extern CRC_HandleTypeDef hcrc;

volatile uint8_t test_string_len = 0;
char test_string_buff[TEST_PACKET_STR_MAX_LEN] = {0};
uint32_t test_string_crc = 0;

TestData_t test_defs[NUM_POSSIBLE_TESTS] =
{
	{ .name = "Timer\0", .state = TESTSTATE_READY, .func = test_timer, },
	{ .name = "UART\0", .state = TESTSTATE_READY, .func = test_uart, },
	{ .name = "SPI\0", .state = TESTSTATE_READY, .func = test_spi, },
	{ .name = "I2C\0", .state = TESTSTATE_READY, .func = test_i2c, },
	{ .name = "ADC\0", .state = TESTSTATE_READY, .func = test_adc, },
};

static bool test_timer(const char *test_string, const uint8_t len)
{
	static const uint32_t capture_error_tolerance = 10;
	static const uint16_t capture_delay_ticks = pdMS_TO_TICKS(50);
	static const uint8_t duty_variation_count = 8;

	uint32_t generated_duty_cycle;
	uint32_t captured_duty_cycle;
	uint32_t capture_error_amount;

	for (int i = 0; i < duty_variation_count; i++)
	{
		generated_duty_cycle = htim1.Instance->ARR / pow(2, i+1);
		htim1.Instance->CCR3 = generated_duty_cycle;
		if (HAL_OK != HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3)
			|| HAL_OK != HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_2))
		{
			HAL_TIM_IC_Stop(&htim1, TIM_CHANNEL_2);
			HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
			return false;
		}
		vTaskDelay(capture_delay_ticks);
		HAL_TIM_IC_Stop(&htim1, TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
		captured_duty_cycle = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_2);

		capture_error_amount = captured_duty_cycle > generated_duty_cycle
				? captured_duty_cycle - generated_duty_cycle
				: generated_duty_cycle - captured_duty_cycle;

		if (capture_error_amount > capture_error_tolerance) return false;
	}

	return true;
}

static bool test_uart(const char *test_string, const uint8_t len)
{
	static char uart_test_rx_buff_1[TEST_PACKET_STR_MAX_LEN] = {0};
	static char uart_test_rx_buff_2[TEST_PACKET_STR_MAX_LEN] = {0};

	bzero(uart_test_rx_buff_1, sizeof(uart_test_rx_buff_1));
	bzero(uart_test_rx_buff_2, sizeof(uart_test_rx_buff_2));

	HAL_UART_Receive_DMA(&huart6, (uint8_t *)uart_test_rx_buff_1, len);
	HAL_UART_Transmit(&huart2, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	if (HAL_CRC_Calculate(&hcrc, uart_test_rx_buff_1, len)
		!= test_string_crc) return false;

	HAL_UART_Receive_DMA(&huart2, (uint8_t *)uart_test_rx_buff_2, len);
	HAL_UART_Transmit(&huart6, (uint8_t *)uart_test_rx_buff_1, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	return (HAL_CRC_Calculate(&hcrc, uart_test_rx_buff_2, len)
			== test_string_crc);
}

static bool test_spi(const char *test_string, const uint8_t len)
{
	static char spi_rx_buff_1[TEST_PACKET_STR_MAX_LEN] = {0};
	static char spi_rx_buff_2[TEST_PACKET_STR_MAX_LEN] = {0};
	static char spi_rx_buff_dummy[TEST_PACKET_STR_MAX_LEN] = {0};
	static char spi_tx_buff_dummy[TEST_PACKET_STR_MAX_LEN] = {0};

	bzero(spi_rx_buff_1, sizeof(spi_rx_buff_1));
	bzero(spi_rx_buff_2, sizeof(spi_rx_buff_2));

	HAL_SPI_Receive_DMA(&hspi5, (uint8_t *)spi_rx_buff_1, len);
	HAL_SPI_Transmit(&hspi3, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	if (HAL_CRC_Calculate(&hcrc, spi_rx_buff_1, len)
			!= test_string_crc) return false;

	HAL_SPI_TransmitReceive_DMA(&hspi5, (uint8_t *)spi_rx_buff_1, (uint8_t *)spi_rx_buff_dummy, len);
	HAL_SPI_TransmitReceive(&hspi3, (uint8_t *)spi_tx_buff_dummy, (uint8_t *)spi_rx_buff_2, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	return (HAL_CRC_Calculate(&hcrc, spi_rx_buff_2, len)
				== test_string_crc);
}

static bool test_i2c(const char *test_string, const uint8_t len)
{
	static char i2c_rx_buff_1[TEST_PACKET_STR_MAX_LEN] = {0};
	static char i2c_rx_buff_2[TEST_PACKET_STR_MAX_LEN] = {0};

	bzero(i2c_rx_buff_1, sizeof(i2c_rx_buff_1));
	bzero(i2c_rx_buff_2, sizeof(i2c_rx_buff_2));

	HAL_I2C_Slave_Receive_DMA(&hi2c1, (uint8_t *)i2c_rx_buff_1, len);
	HAL_I2C_Master_Transmit(&hi2c2, hi2c1.Init.OwnAddress1, (uint8_t *)test_string, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	if (HAL_CRC_Calculate(&hcrc, i2c_rx_buff_1, len)
				!= test_string_crc) return false;

	HAL_I2C_Slave_Transmit_DMA(&hi2c1, (uint8_t *)i2c_rx_buff_1, len);
	HAL_I2C_Master_Receive(&hi2c2, hi2c1.Init.OwnAddress1, (uint8_t *)i2c_rx_buff_2, len, TEST_TIMEOUT_TICKS);
	vTaskDelay(TEST_GAP_TICKS);

	return (HAL_CRC_Calculate(&hcrc, i2c_rx_buff_2, len)
					== test_string_crc);
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

void test_task_loop(TestData_t *def)
{
	for(;;)
	{
		vTaskDelay(pdMS_TO_TICKS(100));

		if (def->state == TESTSTATE_PENDING)
		{
			def->state = TESTSTATE_BUSY;

			bool passed = true;

			while(passed == true && def->iterations > 0)
			{
				vTaskDelay(pdMS_TO_TICKS(1));
				passed = def->func(test_string_buff, test_string_len);
				def->iterations--;
			}

			def->iterations = 0;
			def->state = passed ? TESTSTATE_SUCCESS : TESTSTATE_FAILURE;
		}
	}
}
