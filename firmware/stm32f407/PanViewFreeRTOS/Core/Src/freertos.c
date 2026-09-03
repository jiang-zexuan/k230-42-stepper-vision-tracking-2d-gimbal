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
#include "usart.h"
#include <stdio.h>

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
  .stack_size = 512 * 4,
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

  /* 将任务句柄交给心跳模块，后续由心跳模块统一采集资源信息。 */
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_ARCHITECTURE,
                                     ArchitectureHeaHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_SAFETY,
                                     SafetyTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_STEPPER,
                                     StepperTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_VISION_RX,
                                     VisionRxTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_MOTION,
                                     MotionTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_INPUT,
                                     InputTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_APP_CONTROL,
                                     AppControlTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_UI,
                                     UiTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_AUDIO,
                                     AudioTaskHandle);
  PanView_TaskHeartbeat_RegisterTask(PANVIEW_HEARTBEAT_TELEMETRY,
                                     TelemetryTaskHandle);

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

  /* Infinite loop */
  for(;;)
  {
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
* @retval None
*/
/* USER CODE END Header_StartStepperTask */
void StartStepperTask(void *argument)
{
  /* USER CODE BEGIN StartStepperTask */
  /* 本区域用途：编写步进执行任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮步进执行任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();

  /* Infinite loop */
  for(;;)
  {
    /* 执行一次步进执行任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_STEPPER,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持任务按固定节拍运行。 */
    next_wake_tick += STEPPER_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入运动命令后再扩展执行逻辑。 */
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

  /* Infinite loop */
  for(;;)
  {
    /* 暂时只消费串口通知，真正的文本解析放到后续步骤。 */
    (void)osThreadFlagsWait(PANVIEW_UART_RX_FLAG_DATA_READY, osFlagsWaitAny, 0U);

    /* 把环形缓冲区中当前积压的字节全部取走。 */
    while (PanView_UartRx_Read(received_data, sizeof(received_data)) > 0U)
    {
      /* T04 只验证传输链路，T05 再解析这些字节。 */
    }

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
  /* 本区域用途：编写运动任务的初始化动作和永久循环逻辑。 */
  /* 当前任务没有使用传入参数，显式标记可以避免编译器警告。 */
  (void)argument;

  /* 保存下一轮运动任务计划开始运行的 RTOS tick。 */
  uint32_t next_wake_tick = osKernelGetTickCount();

  /* Infinite loop */
  for(;;)
  {
    /* 执行一次运动任务的最小心跳动作。 */
    PanView_TaskHeartbeat_Update(PANVIEW_HEARTBEAT_MOTION,
                                 HAL_GetTick());

    /* 计算下一轮的计划时刻，保持运动计算按固定节拍运行。 */
    next_wake_tick += MOTION_TASK_PERIOD_TICKS;

    /* 暂时按固定周期运行，后续接入视觉数据后继续保持固定节拍。 */
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

  /* Infinite loop */
  for(;;)
  {
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

  /* Infinite loop */
  for(;;)
  {
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

  /* Infinite loop */
  for(;;)
  {
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
  char uart_log[160];

  /* Infinite loop */
  for(;;)
  {
    /* 读取当前 FreeRTOS 尚未使用的堆空间，供调试器观察。 */
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
      PanViewUartRxBuffer *uart_rx = PanView_UartRx_GetBuffer();
      int uart_log_length;

      uart_log_divider = 0U;
      uart_log_length = snprintf(uart_log, sizeof(uart_log),
                                 "T04 chunks=%lu bytes=%lu used=%u dropped=%lu uart_err=%lu len=%u arm=%lu/%lu start=%lu\r\n",
                                 (unsigned long)uart_rx->chunk_count,
                                 (unsigned long)uart_rx->received_byte_count,
                                 (unsigned int)PanView_RingBuffer_GetUsed(&uart_rx->ring_buffer),
                                 (unsigned long)uart_rx->dropped_byte_count,
                                 (unsigned long)uart_rx->error_count,
                                 (unsigned int)uart_rx->last_length,
                                 (unsigned long)uart_rx->start_success_count,
                                 (unsigned long)uart_rx->start_attempt_count,
                                 (unsigned long)uart_rx->last_start_status);

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

/* USER CODE END Application */

