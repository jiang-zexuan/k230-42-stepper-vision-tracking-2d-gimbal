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
typedef struct
{
  uint8_t target_present;
  int16_t error_x;
  int16_t error_y;
  uint16_t center_x;
  uint16_t center_y;
  uint32_t tick;
} VisionTargetMessage;

typedef struct
{
  uint16_t screen_x;
  uint16_t screen_y;
} PanViewTouchMessage;

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
void PanView_AppStep(void);
void PanView_InputStep(uint32_t current_tick);
void PanView_ApplyUiTouch(uint16_t screen_x, uint16_t screen_y);
void PanView_UiRefreshStep(uint32_t current_tick);
void PanView_TelemetryStep(uint32_t current_tick);
void PanView_SafetyStep(uint32_t current_tick);
HAL_StatusTypeDef PanView_Uart1Transmit(UART_HandleTypeDef *huart,
                                        uint8_t *data,
                                        uint16_t size,
                                        uint32_t timeout);
void PanView_RtosPublishVision(const VisionTargetMessage *message);
void PanView_RtosPublishTouch(uint16_t screen_x, uint16_t screen_y);
void PanView_RtosProcessUiEvents(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOARD_KEY2_Pin GPIO_PIN_2
#define BOARD_KEY2_GPIO_Port GPIOE
#define BOARD_KEY1_Pin GPIO_PIN_3
#define BOARD_KEY1_GPIO_Port GPIOE
#define BOARD_KEY0_Pin GPIO_PIN_4
#define BOARD_KEY0_GPIO_Port GPIOE
#define CTP_RST_Pin GPIO_PIN_5
#define CTP_RST_GPIO_Port GPIOE
#define CTP_INT_Pin GPIO_PIN_6
#define CTP_INT_GPIO_Port GPIOE
#define TFT_LCD_CS_Pin GPIO_PIN_13
#define TFT_LCD_CS_GPIO_Port GPIOC
#define BOARD_STATUS_LED_Pin GPIO_PIN_9
#define BOARD_STATUS_LED_GPIO_Port GPIOF
#define BOARD_KEY_UP_Pin GPIO_PIN_0
#define BOARD_KEY_UP_GPIO_Port GPIOA
#define TFT_LCD_DC_Pin GPIO_PIN_5
#define TFT_LCD_DC_GPIO_Port GPIOA
#define TFT_LCD_RST_Pin GPIO_PIN_6
#define TFT_LCD_RST_GPIO_Port GPIOA
#define TFT_LCD_BL_Pin GPIO_PIN_15
#define TFT_LCD_BL_GPIO_Port GPIOB
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
