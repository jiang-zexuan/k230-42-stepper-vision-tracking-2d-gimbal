/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "debounced_button.h"
#include "periodic_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t boot_message[] = "PanView P01 boot\r\n";

enum {
  BOOT_LOG_TIMEOUT_MS = 100U
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

enum {
  KEY_SCAN_PERIOD_MS = 10U,
  KEY_DEBOUNCE_MS = 20U,
  STATUS_PERIOD_MS = 100U,
  HEARTBEAT_PERIOD_MS = 500U
};
/* 单位：ms；P02 轮询与消抖设计参数，仅适用于当前 KEY0 诊断任务。 */

static uint8_t key0_pressed_message[] = "KEY0 pressed\r\n";
static char heartbeat_message[96];
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  DebouncedButton key0_button;
  PeriodicTask key_scan_task;
  PeriodicTask status_task;
  PeriodicTask heartbeat_task;
  uint32_t last_heartbeat_log_ms = HAL_GetTick();
  uint32_t key_scan_count = 0U;
  uint32_t status_task_count = 0U;
  uint32_t heartbeat_count = 0U;

  DebouncedButton_Init(&key0_button,
                       HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET);
  PeriodicTask_Init(&key_scan_task, KEY_SCAN_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&status_task, STATUS_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&heartbeat_task, HEARTBEAT_PERIOD_MS, HAL_GetTick());
  if (HAL_UART_Transmit(&huart1, boot_message,
                        sizeof(boot_message) - 1U,
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  uint32_t current_tick = HAL_GetTick();
	  if (PeriodicTask_IsDue(&key_scan_task, current_tick))
	  {
	    bool key0_pressed;

	    key_scan_count++;
	    key0_pressed = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET;

	    if (DebouncedButton_Update(&key0_button, key0_pressed, current_tick,
	                               KEY_DEBOUNCE_MS))
	    {
	      HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);

	      if (HAL_UART_Transmit(&huart1, key0_pressed_message,
	                            sizeof(key0_pressed_message) - 1U,
	                            BOOT_LOG_TIMEOUT_MS) != HAL_OK)
	      {
	        Error_Handler();
	      }
	    }
	  }
	  if (PeriodicTask_IsDue(&status_task, current_tick))
	  {
	    status_task_count++;
	  }
	  if (PeriodicTask_IsDue(&heartbeat_task, current_tick))
	   {
	     uint32_t heartbeat_interval_ms = current_tick - last_heartbeat_log_ms;

	     last_heartbeat_log_ms = current_tick;
     int heartbeat_length = snprintf(
	         heartbeat_message, sizeof(heartbeat_message),
	         "t=%lu dt=%lu heartbeat=%lu scans=%lu status=%lu\r\n",
	         (unsigned long)current_tick,
	         (unsigned long)heartbeat_interval_ms,
	         (unsigned long)++heartbeat_count,
	         (unsigned long)key_scan_count,
	         (unsigned long)status_task_count);

	     if ((heartbeat_length <= 0) ||
	         (heartbeat_length >= (int)sizeof(heartbeat_message)) ||
	         (HAL_UART_Transmit(&huart1,
	                            (uint8_t *)heartbeat_message,
	                            (uint16_t)heartbeat_length,
	                            BOOT_LOG_TIMEOUT_MS) != HAL_OK))
	     {
	       Error_Handler();
	     }
	   }
    /* USER CODE BEGIN 3 */
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
