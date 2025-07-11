/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static bool lwip_initialized = false;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ListenerTask */
osThreadId_t ListenerTaskHandle;
uint32_t ListenerTaskBuffer[ 128 ];
osStaticThreadDef_t ListenerTaskControlBlock;
const osThreadAttr_t ListenerTask_attributes = {
  .name = "ListenerTask",
  .cb_mem = &ListenerTaskControlBlock,
  .cb_size = sizeof(ListenerTaskControlBlock),
  .stack_mem = &ListenerTaskBuffer[0],
  .stack_size = sizeof(ListenerTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for UARTTestTask */
osThreadId_t UARTTestTaskHandle;
uint32_t UARTTestTaskBuffer[ 128 ];
osStaticThreadDef_t UARTTestTaskControlBlock;
const osThreadAttr_t UARTTestTask_attributes = {
  .name = "UARTTestTask",
  .cb_mem = &UARTTestTaskControlBlock,
  .cb_size = sizeof(UARTTestTaskControlBlock),
  .stack_mem = &UARTTestTaskBuffer[0],
  .stack_size = sizeof(UARTTestTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for I2CTestTask */
osThreadId_t I2CTestTaskHandle;
uint32_t I2CTestTaskBuffer[ 128 ];
osStaticThreadDef_t I2CTestTaskControlBlock;
const osThreadAttr_t I2CTestTask_attributes = {
  .name = "I2CTestTask",
  .cb_mem = &I2CTestTaskControlBlock,
  .cb_size = sizeof(I2CTestTaskControlBlock),
  .stack_mem = &I2CTestTaskBuffer[0],
  .stack_size = sizeof(I2CTestTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for SPITestTask */
osThreadId_t SPITestTaskHandle;
uint32_t SPITestTaskBuffer[ 128 ];
osStaticThreadDef_t SPITestTaskControlBlock;
const osThreadAttr_t SPITestTask_attributes = {
  .name = "SPITestTask",
  .cb_mem = &SPITestTaskControlBlock,
  .cb_size = sizeof(SPITestTaskControlBlock),
  .stack_mem = &SPITestTaskBuffer[0],
  .stack_size = sizeof(SPITestTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TimerTestTask */
osThreadId_t TimerTestTaskHandle;
uint32_t TimerTestTaskBuffer[ 128 ];
osStaticThreadDef_t TimerTestTaskControlBlock;
const osThreadAttr_t TimerTestTask_attributes = {
  .name = "TimerTestTask",
  .cb_mem = &TimerTestTaskControlBlock,
  .cb_size = sizeof(TimerTestTaskControlBlock),
  .stack_mem = &TimerTestTaskBuffer[0],
  .stack_size = sizeof(TimerTestTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ADCTestTask */
osThreadId_t ADCTestTaskHandle;
uint32_t ADCTestTaskBuffer[ 128 ];
osStaticThreadDef_t ADCTestTaskControlBlock;
const osThreadAttr_t ADCTestTask_attributes = {
  .name = "ADCTestTask",
  .cb_mem = &ADCTestTaskControlBlock,
  .cb_size = sizeof(ADCTestTaskControlBlock),
  .stack_mem = &ADCTestTaskBuffer[0],
  .stack_size = sizeof(ADCTestTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartListenerTask(void *argument);
void StartUARTTestTask(void *argument);
void StartI2CTestTask(void *argument);
void StartSPITestTask(void *argument);
void StartTimerTestTask(void *argument);
void StartADCTestTask(void *argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ListenerTask */
  ListenerTaskHandle = osThreadNew(StartListenerTask, NULL, &ListenerTask_attributes);

  /* creation of UARTTestTask */
  UARTTestTaskHandle = osThreadNew(StartUARTTestTask, NULL, &UARTTestTask_attributes);

  /* creation of I2CTestTask */
  I2CTestTaskHandle = osThreadNew(StartI2CTestTask, NULL, &I2CTestTask_attributes);

  /* creation of SPITestTask */
  SPITestTaskHandle = osThreadNew(StartSPITestTask, NULL, &SPITestTask_attributes);

  /* creation of TimerTestTask */
  TimerTestTaskHandle = osThreadNew(StartTimerTestTask, NULL, &TimerTestTask_attributes);

  /* creation of ADCTestTask */
  ADCTestTaskHandle = osThreadNew(StartADCTestTask, NULL, &ADCTestTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  lwip_initialized = true;
  /* Infinite loop */
  for(;;)
  {
	  vTaskDelay(pdMS_TO_TICKS(1000));
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartListenerTask */
/**
* @brief Function implementing the ListenerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartListenerTask */
void StartListenerTask(void *argument)
{
  /* USER CODE BEGIN StartListenerTask */
  while (!lwip_initialized) vTaskDelay(pdMS_TO_TICKS(100));
  test_listener_task_init();
  /* Infinite loop */
  for(;;)
  {
	  test_listener_task_loop();
  }
  /* USER CODE END StartListenerTask */
}

/* USER CODE BEGIN Header_StartUARTTestTask */
/**
* @brief Function implementing the UARTTestTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUARTTestTask */
void StartUARTTestTask(void *argument)
{
  /* USER CODE BEGIN StartUARTTestTask */
  /* Infinite loop */
  for(;;)
  {
	  test_task_loop(&test_defs[TESTIDX_UART]);
  }
  /* USER CODE END StartUARTTestTask */
}

/* USER CODE BEGIN Header_StartI2CTestTask */
/**
* @brief Function implementing the I2CTestTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartI2CTestTask */
void StartI2CTestTask(void *argument)
{
  /* USER CODE BEGIN StartI2CTestTask */
  /* Infinite loop */
  for(;;)
  {
	  test_task_loop(&test_defs[TESTIDX_I2C]);
  }
  /* USER CODE END StartI2CTestTask */
}

/* USER CODE BEGIN Header_StartSPITestTask */
/**
* @brief Function implementing the SPITestTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSPITestTask */
void StartSPITestTask(void *argument)
{
  /* USER CODE BEGIN StartSPITestTask */
  /* Infinite loop */
  for(;;)
  {
	  test_task_loop(&test_defs[TESTIDX_SPI]);
  }
  /* USER CODE END StartSPITestTask */
}

/* USER CODE BEGIN Header_StartTimerTestTask */
/**
* @brief Function implementing the TimerTestTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTimerTestTask */
void StartTimerTestTask(void *argument)
{
  /* USER CODE BEGIN StartTimerTestTask */
  /* Infinite loop */
  for(;;)
  {
	  test_task_loop(&test_defs[TESTIDX_TIMER]);
  }
  /* USER CODE END StartTimerTestTask */
}

/* USER CODE BEGIN Header_StartADCTestTask */
/**
* @brief Function implementing the ADCTestTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartADCTestTask */
void StartADCTestTask(void *argument)
{
  /* USER CODE BEGIN StartADCTestTask */
  /* Infinite loop */
  for(;;)
  {
	  test_task_loop(&test_defs[TESTIDX_ADC]);
  }
  /* USER CODE END StartADCTestTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

