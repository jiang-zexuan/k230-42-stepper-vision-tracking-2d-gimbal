/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOARD_KEY2_Pin GPIO_PIN_2
#define BOARD_KEY2_GPIO_Port GPIOE
#define BOARD_KEY1_Pin GPIO_PIN_3
#define BOARD_KEY1_GPIO_Port GPIOE
#define BOARD_KEY0_Pin GPIO_PIN_4
#define BOARD_KEY0_GPIO_Port GPIOE
#define BOARD_STATUS_LED_Pin GPIO_PIN_9
#define BOARD_STATUS_LED_GPIO_Port GPIOF
#define BOARD_KEY_UP_Pin GPIO_PIN_0
#define BOARD_KEY_UP_GPIO_Port GPIOA
#define PITCH_EN_Pin GPIO_PIN_7
#define PITCH_EN_GPIO_Port GPIOC
#define PITCH_DIR_Pin GPIO_PIN_8
#define PITCH_DIR_GPIO_Port GPIOC
#define MOTOR_EN_Pin GPIO_PIN_15
#define MOTOR_EN_GPIO_Port GPIOG
#define MOTOR_DIR_Pin GPIO_PIN_7
#define MOTOR_DIR_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
