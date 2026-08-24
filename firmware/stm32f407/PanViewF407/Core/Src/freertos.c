/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "iwdg.h"
#include "usart.h"
#include <stdio.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
/* USER CODE BEGIN Variables */
osThreadId_t panViewTaskHandle;
const osThreadAttr_t panViewTask_attributes = {
  .name = "PanViewTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t visionQueueTaskHandle;
const osThreadAttr_t visionQueueTask_attributes = {
  .name = "VisionQueueTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
osMessageQueueId_t visionTargetQueueHandle;
const osMessageQueueAttr_t visionTargetQueue_attributes = {
  .name = "VisionTargetQueue",
};
osThreadId_t telemetryTaskHandle;
const osThreadAttr_t telemetryTask_attributes = {
  .name = "TelemetryTask",
  /* 遥测只组装少量日志，先控制栈占用，避免接近 RTOS heap 上限。 */
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
osThreadId_t statusTaskHandle;
const osThreadAttr_t statusTask_attributes = {
  .name = "StatusTask",
  /* TFT 绘制调用链较深，实测 256 words 只剩 23 words，扩大到 512 words。 */
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = {
  .name = "InputTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
osThreadId_t safetyTaskHandle;
const osThreadAttr_t safetyTask_attributes = {
  .name = "SafetyTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
osMessageQueueId_t uiTouchQueueHandle;
const osMessageQueueAttr_t uiTouchQueue_attributes = {
  .name = "UiTouchQueue",
};
static volatile uint32_t vision_queue_received_count;
static volatile uint32_t vision_queue_dropped_count;
static volatile uint32_t pan_view_step_count;
static osMutexId_t usart1TxMutexHandle;
static const osMutexAttr_t usart1TxMutex_attributes = {
  .name = "Usart1TxMutex",
};
static bool safety_watchdog_started;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for P11HeartbeatTas */
osThreadId_t P11HeartbeatTasHandle;
const osThreadAttr_t P11HeartbeatTas_attributes = {
  .name = "P11HeartbeatTas",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void PanViewTask(void *argument);
void VisionQueueTask(void *argument);
void TelemetryTask(void *argument);
void StatusTask(void *argument);
void InputTask(void *argument);
void SafetyTask(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);

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
  usart1TxMutexHandle = osMutexNew(&usart1TxMutex_attributes);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  visionTargetQueueHandle = osMessageQueueNew(
      4U, sizeof(VisionTargetMessage), &visionTargetQueue_attributes);
  uiTouchQueueHandle = osMessageQueueNew(
      4U, sizeof(PanViewTouchMessage), &uiTouchQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of P11HeartbeatTas */
  P11HeartbeatTasHandle = osThreadNew(StartTask02, NULL, &P11HeartbeatTas_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  panViewTaskHandle = osThreadNew(PanViewTask, NULL, &panViewTask_attributes);
  visionQueueTaskHandle = osThreadNew(VisionQueueTask, NULL,
                                      &visionQueueTask_attributes);
  telemetryTaskHandle = osThreadNew(TelemetryTask, NULL,
                                    &telemetryTask_attributes);
  statusTaskHandle = osThreadNew(StatusTask, NULL, &statusTask_attributes);
  inputTaskHandle = osThreadNew(InputTask, NULL, &inputTask_attributes);
  safetyTaskHandle = osThreadNew(SafetyTask, NULL, &safetyTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* 任务创建失败时不要静默进入半启动状态；这条日志用于区分堆不足、
   * 调度器未启动和任务运行异常。 */
  {
    char message[160];
    int length = snprintf(
        message, sizeof(message),
        "RTOS create pan=%u vision=%u telemetry=%u status=%u input=%u safety=%u queue=%u touch_queue=%u heap=%u\r\n",
        panViewTaskHandle != NULL, visionQueueTaskHandle != NULL,
        telemetryTaskHandle != NULL, statusTaskHandle != NULL,
        inputTaskHandle != NULL, safetyTaskHandle != NULL,
        visionTargetQueueHandle != NULL, uiTouchQueueHandle != NULL,
        (unsigned int)xPortGetFreeHeapSize());
    if ((length > 0) && (length < (int)sizeof(message)))
    {
      (void)HAL_UART_Transmit(&huart1, (uint8_t *)message,
                              (uint16_t)length, 100U);
    }
  }

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
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the P11HeartbeatTas thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  uint32_t heartbeat = 0U;
  uint32_t previous_pan_view_step_count = 0U;
  char message[360];

  (void)argument;
  /* USER CODE BEGIN StartTask02 */
  for(;;)
  {
    uint32_t current_pan_view_step_count = pan_view_step_count;
    uint32_t pan_view_steps_last_second =
        current_pan_view_step_count - previous_pan_view_step_count;
    UBaseType_t pan_view_stack_free_words =
        uxTaskGetStackHighWaterMark((TaskHandle_t)panViewTaskHandle);
    UBaseType_t vision_queue_stack_free_words =
        uxTaskGetStackHighWaterMark((TaskHandle_t)visionQueueTaskHandle);
    UBaseType_t heartbeat_stack_free_words =
        uxTaskGetStackHighWaterMark((TaskHandle_t)P11HeartbeatTasHandle);
    UBaseType_t telemetry_stack_free_words =
        telemetryTaskHandle == NULL
            ? 0U
            : uxTaskGetStackHighWaterMark((TaskHandle_t)telemetryTaskHandle);
    UBaseType_t status_stack_free_words =
        statusTaskHandle == NULL
            ? 0U
            : uxTaskGetStackHighWaterMark((TaskHandle_t)statusTaskHandle);
    UBaseType_t input_stack_free_words =
        inputTaskHandle == NULL
            ? 0U
            : uxTaskGetStackHighWaterMark((TaskHandle_t)inputTaskHandle);
    UBaseType_t safety_stack_free_words =
        safetyTaskHandle == NULL
            ? 0U
            : uxTaskGetStackHighWaterMark((TaskHandle_t)safetyTaskHandle);

    int length = snprintf(message, sizeof(message),
                          "RTOS heartbeat=%lu pan_steps=%lu vision_rx=%lu "
                          "vision_drop=%lu stack_words={pan:%lu vision:%lu "
                          "telemetry:%lu status:%lu input:%lu safety:%lu heartbeat:%lu} "
                          "state={pan:%u vision:%u telemetry:%u status:%u input:%u "
                          "safety:%u heartbeat:%u}\r\n",
                          (unsigned long)heartbeat++,
                          (unsigned long)pan_view_steps_last_second,
                          (unsigned long)vision_queue_received_count,
                          (unsigned long)vision_queue_dropped_count,
                          (unsigned long)pan_view_stack_free_words,
                          (unsigned long)vision_queue_stack_free_words,
                          (unsigned long)telemetry_stack_free_words,
                          (unsigned long)status_stack_free_words,
                          (unsigned long)input_stack_free_words,
                          (unsigned long)safety_stack_free_words,
                          (unsigned long)heartbeat_stack_free_words,
                          (unsigned int)osThreadGetState(panViewTaskHandle),
                          (unsigned int)osThreadGetState(visionQueueTaskHandle),
                          (unsigned int)(telemetryTaskHandle == NULL
                                             ? 0U
                                             : osThreadGetState(telemetryTaskHandle)),
                          (unsigned int)(statusTaskHandle == NULL
                                             ? 0U
                                             : osThreadGetState(statusTaskHandle)),
                          (unsigned int)(inputTaskHandle == NULL
                                             ? 0U
                                             : osThreadGetState(inputTaskHandle)),
                          (unsigned int)(safetyTaskHandle == NULL
                                             ? 0U
                                             : osThreadGetState(safetyTaskHandle)),
                          (unsigned int)osThreadGetState(P11HeartbeatTasHandle));
    if ((length > 0) && (length < (int)sizeof(message)))
    {
      (void)PanView_Uart1Transmit(&huart1, (uint8_t *)message,
                              (uint16_t)length, 100U);
    }
    previous_pan_view_step_count = current_pan_view_step_count;
    osDelay(1000U);
  }
  /* USER CODE END StartTask02 */
}

HAL_StatusTypeDef PanView_Uart1Transmit(UART_HandleTypeDef *huart,
                                        uint8_t *data,
                                        uint16_t size,
                                        uint32_t timeout)
{
  osStatus_t lock_status;
  HAL_StatusTypeDef transmit_status;

  if ((huart != &huart1) ||
      (osKernelGetState() != osKernelRunning) ||
      (usart1TxMutexHandle == NULL))
  {
    return HAL_UART_Transmit(huart, data, size, timeout);
  }

  lock_status = osMutexAcquire(usart1TxMutexHandle, timeout);
  if (lock_status != osOK)
  {
    return HAL_TIMEOUT;
  }

  transmit_status = HAL_UART_Transmit(huart, data, size, timeout);
  (void)osMutexRelease(usart1TxMutexHandle);
  return transmit_status;
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void PanViewTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    PanView_AppStep();
    pan_view_step_count++;
    osDelay(1U);
  }
}

void VisionQueueTask(void *argument)
{
  VisionTargetMessage message;

  (void)argument;
  for (;;)
  {
    if ((visionTargetQueueHandle != NULL) &&
        (osMessageQueueGet(visionTargetQueueHandle, &message, NULL,
                           osWaitForever) == osOK))
    {
      vision_queue_received_count++;
    }
  }
}

void TelemetryTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    PanView_TelemetryStep(HAL_GetTick());
    osDelay(10U);
  }
}

void StatusTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    PanView_UiRefreshStep(HAL_GetTick());
    osDelay(20U);
  }
}

void InputTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    PanView_InputStep(HAL_GetTick());
    osDelay(10U);
  }
}

void SafetyTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    /* 仅执行无日志的安全检查；发生故障时直接停 STEP、释放 EN。 */
    PanView_SafetyStep(HAL_GetTick());
    /* 只有 SafetyTask 自己持续运行，才允许启动并刷新硬件看门狗。 */
    if (!safety_watchdog_started)
    {
      MX_IWDG_Init();
      safety_watchdog_started = true;
    }
    else
    {
      (void)HAL_IWDG_Refresh(&hiwdg);
    }
    osDelay(10U);
  }
}

void PanView_RtosPublishVision(const VisionTargetMessage *message)
{
  if ((visionTargetQueueHandle == NULL) || (message == NULL))
  {
    return;
  }

  if (osMessageQueuePut(visionTargetQueueHandle, message, 0U, 0U) != osOK)
  {
    /* 队列只保留最近结果：丢弃旧帧，再尝试放入当前帧。 */
    VisionTargetMessage discarded;
    (void)osMessageQueueGet(visionTargetQueueHandle, &discarded, NULL, 0U);
    vision_queue_dropped_count++;
    (void)osMessageQueuePut(visionTargetQueueHandle, message, 0U, 0U);
  }
}

void PanView_RtosPublishTouch(uint16_t screen_x, uint16_t screen_y)
{
  PanViewTouchMessage message = {screen_x, screen_y};

  if (uiTouchQueueHandle != NULL)
  {
    (void)osMessageQueuePut(uiTouchQueueHandle, &message, 0U, 0U);
  }
}

void PanView_RtosProcessUiEvents(void)
{
  PanViewTouchMessage message;

  while ((uiTouchQueueHandle != NULL) &&
         (osMessageQueueGet(uiTouchQueueHandle, &message, NULL, 0U) == osOK))
  {
    PanView_ApplyUiTouch(message.screen_x, message.screen_y);
  }
}

/* USER CODE END Application */

