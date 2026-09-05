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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* 本区域用途：添加本 freertos.c 需要使用的头文件。 */

#include "panview_task_heartbeat.h"
#include "panview_messages.h"
#include "panview_message_bus.h"
#include "panview_uart_rx.h"
#include "panview_text_line.h"
#include "panview_pv04_parser.h"
#include "panview_motion.h"
#include "panview_stepper.h"
#include "panview_safety.h"
#include "panview_safety_test.h"
#include "ili9341.h"
#include "panview_ui.h"
#include "ft6336g.h"
#include "audio_player.h"
#include "panview_telemetry.h"
#include "tim.h"
#include "iwdg.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* 栈溢出时由 FreeRTOS 钩子写入，调试器可直接查看。 */
volatile uint32_t panview_stack_overflow_marker = 0U;
volatile const char *panview_stack_overflow_task_name = 0;

/* MotionTask 写入、UiTask 读取的最新视觉显示数据。 */
volatile uint8_t panview_ui_target_present = 0U;
volatile int16_t panview_ui_error_x_px = 0;
volatile int16_t panview_ui_error_y_px = 0;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* 本区域用途：定义本文件使用的类型、枚举和结构体。 */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* 本区域用途：定义宏、周期、容量和其他编译期常量。 */

/* 架构心跳任务每次运行之间的等待时间，单位：毫秒。 */
#define ARCHITECTURE_HEARTBEAT_PERIOD_MS (500U)

/* 安全任务的计划周期，单位：RTOS tick。当前 10 tick 等于 10 ms。 */
#define SAFETY_TASK_PERIOD_TICKS (10U)

/* 视觉接收任务的骨架运行周期，单位：RTOS tick。 */
#define VISION_RX_TASK_PERIOD_TICKS (10U)

/* 运动任务的骨架运行周期，单位：RTOS tick。 */
#define MOTION_TASK_PERIOD_TICKS (10U)
#define CONTROL_TASK_TIMEOUT_MS (200U)

/* 步进执行任务的骨架运行周期，单位：RTOS tick。 */
#define STEPPER_TASK_PERIOD_TICKS (10U)

/* 输入任务的骨架运行周期，单位：RTOS tick。当前 20 tick 等于 20 ms。 */
#define INPUT_TASK_PERIOD_TICKS (20U)

/* 应用控制任务的骨架运行周期，单位：RTOS tick。 */
#define APP_CONTROL_TASK_PERIOD_TICKS (20U)

/* UI 任务的骨架运行周期，单位：RTOS tick。当前 100 tick 等于 100 ms。 */
#define UI_TASK_PERIOD_TICKS (100U)

/* 音频指示任务的骨架运行周期，单位：RTOS tick。 */
#define AUDIO_TASK_PERIOD_TICKS (100U)

/* 遥测任务的骨架运行周期，单位：RTOS tick。当前 500 tick 等于 500 ms。 */
#define TELEMETRY_TASK_PERIOD_TICKS (500U)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* 本区域用途：定义本文件专用的宏函数。当前任务暂时不需要新增内容。 */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* 本区域用途：定义本文件使用的全局或静态状态变量。 */

/* 记录最近一次检查到的 FreeRTOS 剩余堆空间，单位：字节。 */
static volatile size_t telemetry_free_heap_bytes;

/* 系统事件标志组的句柄，用于保存后续的故障和紧急状态。 */
static osEventFlagsId_t SystemEventFlagsHandle;

/* 系统事件标志组的创建属性。 */
static const osEventFlagsAttr_t SystemEventFlags_attributes = {
  .name = "SystemEventFlags"
};

/* USER CODE END Variables */
/* Definitions for ArchitectureHea */
osThreadId_t ArchitectureHeaHandle;
const osThreadAttr_t ArchitectureHea_attributes = {
  .name = "ArchitectureHea",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SafetyTask */
osThreadId_t SafetyTaskHandle;
const osThreadAttr_t SafetyTask_attributes = {
  .name = "SafetyTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for StepperTask */
osThreadId_t StepperTaskHandle;
const osThreadAttr_t StepperTask_attributes = {
  .name = "StepperTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for VisionRxTask */
osThreadId_t VisionRxTaskHandle;
const osThreadAttr_t VisionRxTask_attributes = {
  .name = "VisionRxTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for MotionTask */
osThreadId_t MotionTaskHandle;
const osThreadAttr_t MotionTask_attributes = {
  .name = "MotionTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for InputTask */
osThreadId_t InputTaskHandle;
const osThreadAttr_t InputTask_attributes = {
  .name = "InputTask",
  .stack_size = 320 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for AppControlTask */
osThreadId_t AppControlTaskHandle;
const osThreadAttr_t AppControlTask_attributes = {
  .name = "AppControlTask",
  .stack_size = 320 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UiTask */
osThreadId_t UiTaskHandle;
const osThreadAttr_t UiTask_attributes = {
  .name = "UiTask",
  .stack_size = 768 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for AudioTask */
osThreadId_t AudioTaskHandle;
const osThreadAttr_t AudioTask_attributes = {
  .name = "AudioTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for TelemetryTask */
osThreadId_t TelemetryTaskHandle;
const osThreadAttr_t TelemetryTask_attributes = {
  .name = "TelemetryTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* 本区域用途：声明本文件中由后文实现的静态函数。 */
static void ArchitectureHeartbeat_RunOnce(void);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartSafetyTask(void *argument);
void StartStepperTask(void *argument);
void StartVisionRxTask(void *argument);
void StartMotionTask(void *argument);
void StartInputTask(void *argument);
void StartAppControlTask(void *argument);
void StartUiTask(void *argument);
void StartAudioTask(void *argument);
void StartTelemetryTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* 本区域用途：放置 FreeRTOS 对象创建前的一次性初始化逻辑。 */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* 本区域用途：创建互斥锁，用于保护不可并发访问的共享资源。 */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* 本区域用途：创建二值信号量或计数信号量，用于同步事件或资源计数。 */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* 本区域用途：创建或启动 FreeRTOS 软件定时器。 */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* 本区域用途：创建任务间消息队列，定义消息容量和消息大小。 */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ArchitectureHea */
  ArchitectureHeaHandle = osThreadNew(StartDefaultTask, NULL, &ArchitectureHea_attributes);

  /* creation of SafetyTask */
  SafetyTaskHandle = osThreadNew(StartSafetyTask, NULL, &SafetyTask_attributes);

  /* creation of StepperTask */
  StepperTaskHandle = osThreadNew(StartStepperTask, NULL, &StepperTask_attributes);

  /* creation of VisionRxTask */
  VisionRxTaskHandle = osThreadNew(StartVisionRxTask, NULL, &VisionRxTask_attributes);

  /* creation of MotionTask */
  MotionTaskHandle = osThreadNew(StartMotionTask, NULL, &MotionTask_attributes);

  /* creation of InputTask */
  InputTaskHandle = osThreadNew(StartInputTask, NULL, &InputTask_attributes);

  /* creation of AppControlTask */
  AppControlTaskHandle = osThreadNew(StartAppControlTask, NULL, &AppControlTask_attributes);

  /* creation of UiTask */
  UiTaskHandle = osThreadNew(StartUiTask, NULL, &UiTask_attributes);

  /* creation of AudioTask */
  AudioTaskHandle = osThreadNew(StartAudioTask, NULL, &AudioTask_attributes);

  /* creation of TelemetryTask */
  TelemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &TelemetryTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* 本区域用途：创建 CubeMX 未生成的额外任务，或补充任务创建后的检查。 */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* 本区域用途：创建事件标志组，或设置 RTOS 启动阶段的事件状态。 */

  /* 创建系统事件标志组，后续用于保存各类故障和紧急状态。 */
  SystemEventFlagsHandle = osEventFlagsNew(&SystemEventFlags_attributes);

  /* 初始化消息总线；本步暂不由任务发送或读取视觉结果。 */
  (void)PanView_MessageBus_Init();
  /* 初始化并启动 K230 串口接收。 */
  PanView_UartRx_Init();
  (void)PanView_UartRx_Start();

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
  /* 本区域用途：编写该任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  for(;;)
  {
    /* 执行一次架构心跳动作。 */
    ArchitectureHeartbeat_RunOnce();

    /* 等待一个完整周期后再运行下一次。 */
    osDelay(ARCHITECTURE_HEARTBEAT_PERIOD_MS);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartSafetyTask */
/**
* @brief Function implementing the SafetyTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSafetyTask */
void StartSafetyTask(void *argument)
{
  /* USER CODE BEGIN StartSafetyTask */
  /* 本区域用途：编写安全任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮安全任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();
  PanViewSafetyStatus safety_status;
  const volatile PanViewHeartbeatRecord *motion_heartbeat;
  PanViewStepperStatus safety_stepper_status;
  PanView_Safety_Init();

  /* Infinite loop */
  for(;;)
  {
#if 1
    motion_heartbeat = PanView_TaskHeartbeat_Get(PANVIEW_HEARTBEAT_MOTION);
    if ((motion_heartbeat != 0) && (motion_heartbeat->count > 0U) &&
        ((uint32_t)(HAL_GetTick() - motion_heartbeat->last_tick) >
         CONTROL_TASK_TIMEOUT_MS))
    {
      PanView_Safety_RaiseFault(PANVIEW_SAFETY_CONTROL_TASK_STALE,
                                HAL_GetTick());
    }

#endif

#if 1
    PanView_Stepper_GetStatus(&safety_stepper_status);
    if (((safety_stepper_status.pan_position_steps >= 800) &&
         (safety_stepper_status.pan_applied_speed_steps_per_second > 0)) ||
        ((safety_stepper_status.pan_position_steps <= -800) &&
         (safety_stepper_status.pan_applied_speed_steps_per_second < 0)) ||
        ((safety_stepper_status.pitch_position_steps >= 800) &&
         (safety_stepper_status.pitch_applied_speed_steps_per_second > 0)) ||
        ((safety_stepper_status.pitch_position_steps <= -800) &&
         (safety_stepper_status.pitch_applied_speed_steps_per_second < 0)))
    {
      PanView_Safety_RaiseFault(PANVIEW_SAFETY_SOFTWARE_LIMIT,
                                HAL_GetTick());
    }
#endif

    /* 统一安全停机路径：发生锁存故障时立即停止步进执行。 */
#if 1
    PanView_Safety_GetStatus(&safety_status);
    if (safety_status.latched != 0U)
    {
      PanView_Stepper_SafetyStop();
    }
#endif

    /* SafetyTask 正常运行时周期性喂狗；若任务失活，IWDG 将复位芯片。 */
    (void)HAL_IWDG_Refresh(&hiwdg);

    /* 执行一次安全任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_SAFETY,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持任务按固定节拍运行。 */
    next_wake_tick += SAFETY_TASK_PERIOD_TICKS;

    /* 挂起当前任务，直到到达下一轮的计划时刻。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartSafetyTask */
}

/* USER CODE BEGIN Header_StartStepperTask */
/**
* @brief Function implementing the StepperTask thread.
* @param argument: Not used
* @retval None吗，
*/
/* USER CODE END Header_StartStepperTask */
void StartStepperTask(void *argument)
{
  /* USER CODE BEGIN StartStepperTask */
  (void)argument;

  uint32_t next_wake_tick = osKernelGetTickCount();
  uint32_t last_stepper_log_tick = 0U;
  MotionCommand latest_command;
  PanViewStepperStatus stepper_status;
  GPIO_PinState previous_key_up_state = GPIO_PIN_RESET;
  uint32_t last_key_up_tick = HAL_GetTick() - 200U;

  (void)memset(&latest_command, 0, sizeof(latest_command));
  PanView_Stepper_Init();

  for(;;)
  {
    GPIO_PinState key_up_state =
        HAL_GPIO_ReadPin(BOARD_KEY_UP_GPIO_Port, BOARD_KEY_UP_Pin);

    /*
     * KEY_UP 只报告一次有效按下：第一次确认零点并开始，再按则暂停。
     * 200 ms 防抖可避免一次机械按键抖动被误认为连续两次切换。
     */
    if ((key_up_state == GPIO_PIN_SET) &&
        (previous_key_up_state == GPIO_PIN_RESET) &&
        ((uint32_t)(HAL_GetTick() - last_key_up_tick) >= 200U))
    {
      PanView_Stepper_ToggleSystemRun();
      last_key_up_tick = HAL_GetTick();
    }
    previous_key_up_state = key_up_state;

    /* StepperTask 只传递命令，速度渐变和硬件执行由步进模块负责。 */
    (void)PanView_MessageBus_ReadMotionCommand(&latest_command);
    PanView_Stepper_Execute(&latest_command);
    PanView_Stepper_GetStatus(&stepper_status);

    if (0 && ((uint32_t)(HAL_GetTick() - last_stepper_log_tick) >= 500U))
    {
      char stepper_log[128];
      int32_t pan_angle_deg =
          (stepper_status.pan_position_steps * 90) / 800;
      int32_t pitch_angle_deg =
          (stepper_status.pitch_position_steps * 90) / 800;
      int stepper_log_length = snprintf(
          stepper_log, sizeof(stepper_log),
          "T07 run=%u v=%u pt=%ld pa=%ld qt=%ld qa=%ld pr=%u qr=%u x=%ld y=%ld "
          "pan_deg=%ld pitch_deg=%ld\r\n",
          stepper_status.system_running,
          latest_command.valid,
          (long)latest_command.pan_speed_steps_per_second,
          (long)stepper_status.pan_applied_speed_steps_per_second,
          (long)latest_command.pitch_speed_steps_per_second,
          (long)stepper_status.pitch_applied_speed_steps_per_second,
          stepper_status.pan_running,
          stepper_status.pitch_running,
          (long)stepper_status.pan_position_steps,
          (long)stepper_status.pitch_position_steps,
          (long)pan_angle_deg,
          (long)pitch_angle_deg);

      last_stepper_log_tick = HAL_GetTick();
      if ((stepper_log_length > 0) &&
          (stepper_log_length < (int)sizeof(stepper_log)))
      {
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)stepper_log,
                                (uint16_t)stepper_log_length, 20U);
      }
    }

    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_STEPPER, HAL_GetTick());
    next_wake_tick += STEPPER_TASK_PERIOD_TICKS;
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartStepperTask */
}

/* USER CODE BEGIN Header_StartVisionRxTask */
/**
* @brief Function implementing the VisionRxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartVisionRxTask */
void StartVisionRxTask(void *argument)
{
  /* USER CODE BEGIN StartVisionRxTask */
  /* 本区域用途：编写视觉接收任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮视觉接收任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();
  uint8_t received_data[PANVIEW_UART_RX_BUFFER_SIZE];
  PanViewTextLineAccumulator line_accumulator;
  char line[PANVIEW_TEXT_LINE_CAPACITY];
  VisionResult vision_result;
  uint32_t last_log_tick = 0U;

  PanView_TextLine_Init(&line_accumulator);

  /* Infinite loop */
  for(;;)
  {
    /* 暂时只消费串口通知，真正的文本解析放到后续步骤。 */
    (void)osThreadFlagsWait(PANVIEW_UART_RX_FLAG_DATA_READY, osFlagsWaitAny, 0U);

    /* 把环形缓冲区中当前积压的字节全部取走。 */
    uint16_t received_length;

    while ((received_length = PanView_UartRx_Read(
                received_data, sizeof(received_data))) > 0U)
    {
      uint16_t i;

      for (i = 0U; i < received_length; i++)
      {
        if (PanView_TextLine_FeedByte(&line_accumulator, received_data[i],
                                      line, sizeof(line)) != 0U)
        {
          if (PanView_Pv04_Parse(line, &vision_result) != 0U)
          {
            vision_result.received_tick_ms = HAL_GetTick();
            (void)PanView_MessageBus_PublishVisionResult(&vision_result);
            uint32_t now = HAL_GetTick();
            if (0 && ((uint32_t)(now - last_log_tick) >= 500U))
            {
              char uart_log[160];
              int log_length;
              PanViewUartRxBuffer *uart_rx = PanView_UartRx_GetBuffer();

              last_log_tick = now;
              log_length = snprintf(
                  uart_log, sizeof(uart_log),
                  "PV05_OK t_ms=%lu target=%u count=%u cx=%d cy=%d rx_chunks=%lu rx_bytes=%lu\r\n",
                  (unsigned long)vision_result.source_timestamp_ms,
                  vision_result.target_present,
                  vision_result.target_count,
                  (int)vision_result.center_x_px,
                  (int)vision_result.center_y_px,
                  (unsigned long)uart_rx->chunk_count,
                  (unsigned long)uart_rx->received_byte_count);

              if ((log_length > 0) && (log_length < (int)sizeof(uart_log)))
              {
                (void)HAL_UART_Transmit(&huart1, (uint8_t *)uart_log,
                                        (uint16_t)log_length, 20U);
              }
            }
          }
        }
      }
    }

    /* 记录 VisionRxTask 历史最低剩余栈空间，单位是 Words。 */
    PanView_TaskHeartbeat_UpdateStack(
        PANVIEW_HEARTBEAT_VISION_RX,
        (uint32_t)uxTaskGetStackHighWaterMark(NULL));

    /* 执行一次视觉接收任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_VISION_RX,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持任务按固定节拍运行。 */
    next_wake_tick += VISION_RX_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入 UART 后会改为等待收包通知。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartVisionRxTask */
}

/* USER CODE BEGIN Header_StartMotionTask */
/**
* @brief Function implementing the MotionTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMotionTask */
void StartMotionTask(void *argument)
{
  /* USER CODE BEGIN StartMotionTask */
  (void)argument;

  uint32_t next_wake_tick = osKernelGetTickCount();
  VisionResult latest_vision;
  MotionCommand motion_command;
  int16_t error_x_px;
  int16_t error_y_px;
  /* 诊断文本较长，使用任务独占的静态缓冲区，避免额外占用任务栈。 */
  static char vision_log[192];

  (void)memset(&latest_vision, 0, sizeof(latest_vision));
  (void)memset(&motion_command, 0, sizeof(motion_command));

  for(;;)
  {
    /*
     * 干净调参基线：
     * 只读取最新视觉结果，再由运动模块生成纯 P 命令。
     * 此处不再叠加坐标限幅、目标防抖、锁定滞回和二次速度斜率。
     */
    int vision_read_result =
        PanView_MessageBus_ReadVisionResult(&latest_vision);
    PanView_Motion_CalculateError(&latest_vision, &error_x_px, &error_y_px);
    PanView_Motion_CreateCommand(&latest_vision, HAL_GetTick(),
                                 &motion_command);
    panview_ui_target_present = latest_vision.target_present;
    panview_ui_error_x_px = error_x_px;
    panview_ui_error_y_px = error_y_px;
    (void)PanView_MessageBus_PublishMotionCommand(&motion_command);

    /*
     * 只在真正读到一份新视觉结果时打印，避免重复打印旧数据。
     * 同一行同时保留输入坐标、计算误差和输出命令，便于判断突变
     * 最先发生在视觉输入、运动计算还是电机执行之前。
     */
    /* 暂时关闭 VISION_NEW 串口诊断输出，保留后续排查代码。 */
    if (0 && (vision_read_result == 0))
    {
      int vision_log_length = snprintf(
          vision_log, sizeof(vision_log),
          "VISION_NEW src=%lu rx=%lu target=%u count=%u cx=%d cy=%d "
          "err_x=%d err_y=%d pan=%ld pitch=%ld\r\n",
          (unsigned long)latest_vision.source_timestamp_ms,
          (unsigned long)latest_vision.received_tick_ms,
          latest_vision.target_present,
          latest_vision.target_count,
          (int)latest_vision.center_x_px,
          (int)latest_vision.center_y_px,
          error_x_px,
          error_y_px,
          (long)motion_command.pan_speed_steps_per_second,
          (long)motion_command.pitch_speed_steps_per_second);

      if ((vision_log_length > 0) &&
          (vision_log_length < (int)sizeof(vision_log)))
      {
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)vision_log,
                                (uint16_t)vision_log_length, 20U);
      }
    }

    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_MOTION, HAL_GetTick());
    next_wake_tick += MOTION_TASK_PERIOD_TICKS;
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartMotionTask */
}

/* USER CODE BEGIN Header_StartInputTask */
/**
* @brief Function implementing the InputTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInputTask */
void StartInputTask(void *argument)
{
  /* USER CODE BEGIN StartInputTask */
  /* 本区域用途：编写输入任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮输入任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();
  GPIO_PinState previous_key0_state = GPIO_PIN_SET;
  Ft6336gPoint previous_touch = {0};
  /* 触摸芯片只在任务启动时初始化一次。 */
  const bool touch_polling_enabled = FT6336G_Init();

  /* Infinite loop */
  for(;;)
  {
    /* KEY0 使用上拉输入，按下时为低电平；下降沿注入一次软件急停。 */
    GPIO_PinState key0_state =
        HAL_GPIO_ReadPin(BOARD_KEY0_GPIO_Port, BOARD_KEY0_Pin);
    if ((key0_state == GPIO_PIN_RESET) &&
        (previous_key0_state == GPIO_PIN_SET))
    {
      PanView_SafetyTest_InjectEmergencyStop(HAL_GetTick());
    }
    previous_key0_state = key0_state;

    /* 触摸屏只在“未按下 -> 按下”时发布一次 START/STOP 事件。 */
    Ft6336gPoint touch_point;
    if (touch_polling_enabled && FT6336G_ReadPoint(&touch_point))
    {
      const bool touch_pressed = (touch_point.touch_count > 0U);
      const bool was_pressed = (previous_touch.touch_count > 0U);
      if (touch_pressed && !was_pressed)
      {
        /* 旧工程显示旋转 180 度，FT6336G 原始坐标也要反向映射到 240x320。 */
        const uint16_t screen_x = (touch_point.x <= 239U)
                                      ? (uint16_t)(239U - touch_point.x)
                                      : 0U;
        const uint16_t screen_y = (touch_point.y <= 319U)
                                      ? (uint16_t)(319U - touch_point.y)
                                      : 0U;
        const UiEvent ui_event = PanViewUi_HandleTouch(
            screen_x, screen_y, true);
        InputEvent input_event;
        input_event.occurred_tick_ms = HAL_GetTick();
        if (ui_event == UI_EVENT_START)
        {
          input_event.type = PANVIEW_INPUT_EVENT_START_REQUEST;
          (void)PanView_MessageBus_PublishInputEvent(&input_event);
        }
        else if (ui_event == UI_EVENT_STOP)
        {
          input_event.type = PANVIEW_INPUT_EVENT_STOP_REQUEST;
          (void)PanView_MessageBus_PublishInputEvent(&input_event);
        }
      }
      previous_touch = touch_point;
    }

    /* 执行一次输入任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_INPUT,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持输入扫描按固定节拍运行。 */
    next_wake_tick += INPUT_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入按键和触摸后再扩展采样逻辑。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartInputTask */
}

/* USER CODE BEGIN Header_StartAppControlTask */
/**
* @brief Function implementing the AppControlTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAppControlTask */
void StartAppControlTask(void *argument)
{
  /* USER CODE BEGIN StartAppControlTask */
  /* 本区域用途：编写应用控制任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮应用控制任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();

  /* Infinite loop */
  for(;;)
  {
    /* 执行一次应用控制任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_APP_CONTROL,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持应用控制按固定节拍运行。 */
    next_wake_tick += APP_CONTROL_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入输入事件队列后再改为等待事件。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartAppControlTask */
}

/* USER CODE BEGIN Header_StartUiTask */
/**
* @brief Function implementing the UiTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUiTask */
void StartUiTask(void *argument)
{
  /* USER CODE BEGIN StartUiTask */
  /* 本区域用途：编写 UI 任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮 UI 任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();
  PanViewUiData ui_data;
  PanViewVisualState previous_visual_state = PANVIEW_VISUAL_STATE_SEARCH;
  (void)memset(&ui_data, 0, sizeof(ui_data));
  ui_data.state = UI_STATE_SEARCH;
  ui_data.tft_ready = true;
  PanViewUi_Init(UI_LANGUAGE_EN);

  /* Infinite loop */
  for(;;)
  {
    /* 应用控制任务统一决定系统是否启动或停止。 */
    InputEvent input_event;
    while (PanView_MessageBus_ReadInputEvent(&input_event) == 0)
    {
      if (input_event.type == PANVIEW_INPUT_EVENT_START_REQUEST)
      {
        PanView_Stepper_ToggleSystemRun();
      }
      else if (input_event.type == PANVIEW_INPUT_EVENT_STOP_REQUEST)
      {
        PanView_Stepper_SafetyStop();
      }
    }

    /* T10-5：读取步进模块只读状态，更新 UI 的运行状态和位置数据。 */
    PanViewStepperStatus ui_stepper_status;
    PanView_Stepper_GetStatus(&ui_stepper_status);
    ui_data.running = (ui_stepper_status.system_running != 0U);
    ui_data.target_present = (panview_ui_target_present != 0U);
    ui_data.error_x = panview_ui_error_x_px;
    ui_data.error_y = panview_ui_error_y_px;
    const bool limit_fault = (ui_stepper_status.pan_limit_active != 0U) ||
                             (ui_stepper_status.pitch_limit_active != 0U);
    const bool target_hit = ui_data.target_present &&
                            (ui_data.error_x <= PANVIEW_MOTION_LOCK_ENTER_PX) &&
                            (ui_data.error_x >= -PANVIEW_MOTION_LOCK_ENTER_PX) &&
                            (ui_data.error_y <= PANVIEW_MOTION_LOCK_ENTER_PX) &&
                            (ui_data.error_y >= -PANVIEW_MOTION_LOCK_ENTER_PX);
    ui_data.state = limit_fault ? UI_STATE_FAULT :
                    (target_hit ? UI_STATE_LOCKED :
                     (ui_data.target_present ? UI_STATE_TRACKING : UI_STATE_SEARCH));
    if ((ui_data.state == UI_STATE_LOCKED) &&
        (previous_visual_state != PANVIEW_VISUAL_STATE_LOCKED))
    {
      IndicatorEvent hit_event = { PANVIEW_INDICATOR_EVENT_TARGET_LOCKED, HAL_GetTick() };
      (void)PanView_MessageBus_PublishIndicatorEvent(&hit_event);
    }
    previous_visual_state = (PanViewVisualState)ui_data.state;
    ui_data.pan_position = ui_stepper_status.pan_position_steps;
    ui_data.pitch_position = ui_stepper_status.pitch_position_steps;
    ui_data.pan_speed = ui_stepper_status.pan_applied_speed_steps_per_second;
    ui_data.pitch_speed = ui_stepper_status.pitch_applied_speed_steps_per_second;
    /* 由 UI 模块按 500 ms 周期只刷新运行页的动态区域。 */
    PanViewUi_Refresh(&ui_data, HAL_GetTick());

    /* 执行一次 UI 任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_UI,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持 UI 刷新按较低频率运行。 */
    next_wake_tick += UI_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入 TFT 后再扩展显示逻辑。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartUiTask */
}

/* USER CODE BEGIN Header_StartAudioTask */
/**
* @brief Function implementing the AudioTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAudioTask */
void StartAudioTask(void *argument)
{
  /* USER CODE BEGIN StartAudioTask */
  /* 本区域用途：编写音频指示任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮音频指示任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();
  AudioPlayer_Init();
  HAL_GPIO_WritePin(HIT_INDICATOR_GPIO_Port, HIT_INDICATOR_Pin, GPIO_PIN_RESET);

  /* Infinite loop */
  for(;;)
  {
    IndicatorEvent indicator_event;
    while (PanView_MessageBus_ReadIndicatorEvent(&indicator_event) == 0)
    {
      if (indicator_event.type == PANVIEW_INDICATOR_EVENT_TARGET_LOCKED)
      {
        if (AudioPlayer_Play(PANVIEW_AUDIO_HIT))
        {
          /* 音频开始播放时点亮 PF10 命中指示灯。 */
          HAL_GPIO_WritePin(HIT_INDICATOR_GPIO_Port, HIT_INDICATOR_Pin,
                           GPIO_PIN_SET);
        }
      }
    }

    /* DMA 播放结束后，播放器会清除 busy 标志，此时同步熄灭指示灯。 */
    if (!AudioPlayer_IsBusy())
    {
      HAL_GPIO_WritePin(HIT_INDICATOR_GPIO_Port, HIT_INDICATOR_Pin,
                       GPIO_PIN_RESET);
    }
    /* 执行一次音频指示任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_AUDIO,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持低优先级任务按固定节拍运行。 */
    next_wake_tick += AUDIO_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入音效事件后再改为等待事件。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartAudioTask */
}

/* USER CODE BEGIN Header_StartTelemetryTask */
/**
* @brief Function implementing the TelemetryTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTelemetryTask */
void StartTelemetryTask(void *argument)
{
  /* USER CODE BEGIN StartTelemetryTask */
  /* 本区域用途：编写遥测任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮遥测任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();
  uint8_t uart_log_divider = 0U;
  char uart_log[512];

  /* Infinite loop */
  for(;;)
  {
    /* 读取当前 FreeRTOS 尚未使用的堆空间，供调试器观察。 */
    PanViewTelemetrySnapshot telemetry;
    telemetry_free_heap_bytes = xPortGetFreeHeapSize();

    /* 由心跳模块统一读取所有任务的栈余量和 RTOS 状态。 */
    PanView_TaskHeartbeat_CollectRuntimeInfo();

    /* 执行一次遥测任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_TELEMETRY,
                                 HAL_GetTick());

    /* TelemetryTask 每 500 ms 运行一次，两次循环输出一条 T04 状态。 */
    uart_log_divider++;
    if (uart_log_divider >= 2U)
    {
      int uart_log_length;

      uart_log_divider = 0U;
      PanView_Telemetry_Collect(&telemetry);
      uart_log_length = PanView_Telemetry_Format(&telemetry, uart_log, sizeof(uart_log));

      /* 日志从 USART1 输出，最长等待 20 ms，避免长期阻塞遥测任务。 */
      if ((uart_log_length > 0) && (uart_log_length < (int)sizeof(uart_log)))
      {
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)uart_log,
                                (uint16_t)uart_log_length, 20U);
      }
    }

    /* 计算下一轮的计划时刻，保持低优先级遥测按较低频率运行。 */
    next_wake_tick += TELEMETRY_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入日志和统计后再扩展发送逻辑。 */
    osDelayUntil(next_wake_tick);
  }
  /* USER CODE END StartTelemetryTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* 本区域用途：实现本文件中已声明的静态辅助函数和应用级 RTOS 辅助逻辑。 */
static void ArchitectureHeartbeat_RunOnce(void)
{
  /* 读取当前系统运行时间，单位：毫秒。 */
  uint32_t current_tick = HAL_GetTick();

  /* 将本任务的运行记录交给统一心跳模块保存。 */
  PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_ARCHITECTURE,
                               current_tick);

  /* 翻转状态灯，表示任务仍在正常运行。 */
  HAL_GPIO_TogglePin(BOARD_STATUS_LED_GPIO_Port,
                     BOARD_STATUS_LED_Pin);
}

/*
 * FreeRTOS 栈溢出回调。
 * configCHECK_FOR_STACK_OVERFLOW=2 时，检测到任务栈溢出会进入这里。
 * 调试器中查看 marker 和 task_name，即可知道是否溢出以及哪个任务溢出。
 */
void vApplicationStackOverflowHook(TaskHandle_t task_handle,
                                   char *task_name)
{
  (void)task_handle;
  panview_stack_overflow_marker = 0x53544B4FU; /* ASCII: STKO */
  panview_stack_overflow_task_name = task_name;
  __disable_irq();

  while (1)
  {
    /* 故意停在这里，方便调试器定位现场。 */
  }
}

/* USER CODE END Application */

