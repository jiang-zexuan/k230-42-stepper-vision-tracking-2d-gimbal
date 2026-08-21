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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "communication_watchdog.h"
#include "debounced_button.h"
#include "frame_sequence_tracker.h"
#include "motor_pulse_lab.h"
#include "motion_limits.h"
#include "periodic_task.h"
#include "relative_position_tracker.h"
#include "uart_rx_frame.h"
#include "uart_text_command.h"
#include "vision_frame_parser.h"
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
/* 启动后通过 USART1 输出，作为串口链路可用的第一条证据。 */
static uint8_t boot_message[] = "PanView P01 boot\r\n";

enum {
  /* 单位：ms；串口发送阻塞等待上限，仅用于当前诊断日志。 */
  BOOT_LOG_TIMEOUT_MS = 100U,
  /* 单位：ms；P03 接收结果日志的串口发送等待上限。 */
  UART_RX_LOG_TIMEOUT_MS = 100U
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

enum {
  /* 单位：ms；P02 轮询与消抖设计参数，仅适用于当前 KEY0 诊断任务。 */
  KEY_SCAN_PERIOD_MS = 10U,
  KEY_DEBOUNCE_MS = 20U,
  STATUS_PERIOD_MS = 100U,
  HEARTBEAT_PERIOD_MS = 500U,
  /* 单位：ms；P03 通信超时观察参数，不是最终云台安全阈值。 */
  COMMUNICATION_TIMEOUT_MS = 1000U,
  /* 单位：Hz；TIM4 配置为 16 MHz / (3199 + 1) / (9 + 1)，用于 P05 空载验收。 */
  MOTOR_STEP_FREQUENCY_HZ = 500U,
  /* 单位：无；TIM4 使用固定 Period=9、Pulse=5 时的 500 Hz 分频值。 */
  MOTOR_CONTINUOUS_PRESCALER = 3199U,
  /* 单位：deg；P05 固定运动的目标转角，基于当前 16 细分配置。 */
  MOTOR_FIXED_MOVE_ANGLE_DEGREES = 90U,
  /* 单位：脉冲；90 deg * 3200 pulses/rev / 360 deg/rev，X42S 当前为 16 细分。 */
  MOTOR_FIXED_MOVE_PULSES = 800U,
  /* 单位：脉冲/圈；X42S 当前 16 细分设置。 */
  MOTOR_PULSES_PER_REVOLUTION = 3200U
};

typedef struct
{
  uint16_t step_frequency_hz;
  uint16_t tim_prescaler;
} MotorPulseLabSpeedStage;

/* 300 Hz 无法被当前 16 MHz 时钟精确整除，实际为约 300.019 Hz。 */
static const MotorPulseLabSpeedStage motor_profile_speed_stages[] = {
    {100U, 15999U}, {200U, 7999U},  {300U, 5332U},
    {400U, 3999U},  {500U, 3199U},  {400U, 3999U},
    {300U, 5332U},  {200U, 7999U},  {100U, 15999U}};

/* KEY0 事件与心跳日志共用 USART1；心跳缓冲区避免逐字符发送。 */
static uint8_t key0_pressed_message[] = "KEY0 pressed\r\n";
static uint8_t key1_pressed_message[] = "KEY1 pressed\r\n";
static uint8_t key2_pressed_message[] = "KEY2 pressed\r\n";
static uint8_t key_up_pressed_message[] = "KEY_UP pressed\r\n";
static char heartbeat_message[96];
static char relative_position_message[128];

/* DMA 先写入此缓冲区；回调复制到 uart_rx_frame 后立即重启下一次接收。 */
static uint8_t uart_rx_dma_buffer[UART_RX_FRAME_CAPACITY];
static UartRxFrame uart_rx_frame;
static char uart_rx_log_message[128];
static uint8_t uart_pong_message[] = "PONG\r\n";
static uint8_t uart_unknown_command_message[] = "ERR unknown command\r\n";
static uint8_t motor_running_ccw_message[] =
    "MOTOR state=running dir=ccw step_hz=500\r\n";
static uint8_t motor_running_cw_message[] =
    "MOTOR state=running dir=cw step_hz=500\r\n";
static uint8_t motor_stopped_message[] = "MOTOR state=stopped\r\n";
static uint8_t motor_direction_ccw_message[] = "MOTOR direction=ccw\r\n";
static uint8_t motor_direction_cw_message[] = "MOTOR direction=cw\r\n";
static uint8_t motor_direction_ignored_message[] =
    "MOTOR direction change ignored: running\r\n";
static uint8_t motor_profile_move_ccw_message[] =
    "MOTOR state=profile_move dir=ccw target_deg=90 target_pulses=800\r\n";
static uint8_t motor_profile_move_cw_message[] =
    "MOTOR state=profile_move dir=cw target_deg=90 target_pulses=800\r\n";
static uint8_t motor_profile_move_ignored_message[] =
    "MOTOR profile move ignored: running\r\n";
static uint8_t motor_profile_move_position_invalid_message[] =
    "MOTOR profile move rejected: position_invalid; press KEY_UP to zero\r\n";
static uint8_t motor_profile_move_limit_message[] =
    "MOTOR profile move rejected: horizontal_limit\r\n";
static uint8_t motor_profile_move_invalid_argument_message[] =
    "MOTOR profile move rejected: invalid_limit_config\r\n";
static uint8_t motor_profile_move_complete_message[] =
    "MOTOR state=stopped reason=profile_complete target_pulses=800\r\n";
static uint8_t relative_position_zero_ignored_message[] =
    "POSITION manual zero ignored: motor running\r\n";
static VisionFrameParser vision_frame_parser;
static VisionFrame latest_vision_frame;
static bool latest_vision_frame_valid;
static CommunicationWatchdog communication_watchdog;
static FrameSequenceTracker frame_sequence_tracker;
static MotorPulseLab motor_pulse_lab;
static MotionLimits horizontal_motion_limits;
static RelativePositionTracker relative_position_tracker;
static volatile bool motor_profile_move_complete_pending;

static const char *CommunicationStateText(CommunicationState state)
{
  switch (state)
  {
    case COMMUNICATION_ACTIVE:
      return "active";
    case COMMUNICATION_TIMEOUT:
      return "timeout";
    case COMMUNICATION_WAITING:
    default:
      return "waiting";
  }
}

static const char *FrameSequenceResultText(FrameSequenceResult result)
{
  switch (result)
  {
    case FRAME_SEQUENCE_FIRST:
      return "first";
    case FRAME_SEQUENCE_IN_ORDER:
      return "in_order";
    case FRAME_SEQUENCE_GAP:
      return "gap";
    case FRAME_SEQUENCE_DUPLICATE:
      return "duplicate";
    case FRAME_SEQUENCE_OUT_OF_ORDER:
    default:
      return "out_of_order";
  }
}

static void StartUartRxDma(void)
{
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer,
                                   sizeof(uart_rx_dma_buffer)) != HAL_OK)
  {
    Error_Handler();
  }

  /* P03 以 IDLE 或缓冲区写满作为一段数据结束，不处理半满通知。 */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
}

/*
 * HAL 在 UART 空闲或 DMA 缓冲区写满时调用此函数。
 * 这里只做短时间的数据保存与 DMA 重启；字符串格式化和串口输出留给主循环。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart != &huart1)
  {
    return;
  }

  (void)UartRxFrame_Store(&uart_rx_frame, uart_rx_dma_buffer, size);
  StartUartRxDma();
}

/* P05 只把状态机结果映射到硬件动作，不在这里实现速度规划或视觉跟踪。 */
static void MotorPulseLab_SetTimerPrescaler(uint16_t prescaler)
{
  __HAL_TIM_SET_PRESCALER(&htim4, prescaler);
}

static void MotorPulseLab_StartHardware(MotorPulseLabDirection direction,
                                        uint16_t prescaler)
{
  GPIO_PinState dir_pin_state = direction == MOTOR_PULSE_LAB_DIRECTION_HIGH
                                    ? GPIO_PIN_SET
                                    : GPIO_PIN_RESET;

  MotorPulseLab_SetTimerPrescaler(prescaler);
  __HAL_TIM_SET_COUNTER(&htim4, 0U);
  if (HAL_TIM_GenerateEvent(&htim4, TIM_EVENTSOURCE_UPDATE) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
  HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin, dir_pin_state);
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);

  if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK)
  {
    HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
    Error_Handler();
  }
}

static void MotorPulseLab_StopHardware(void)
{
  if (HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Base_Stop_IT(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
}

static void MotorPulseLab_StartFixedHardware(MotorPulseLabDirection direction)
{
  uint8_t stage_index = MotorPulseLab_GetProfileStageIndex(&motor_pulse_lab);

  if (stage_index >=
      (sizeof(motor_profile_speed_stages) / sizeof(motor_profile_speed_stages[0])))
  {
    Error_Handler();
  }

  MotorPulseLab_StartHardware(
      direction, motor_profile_speed_stages[stage_index].tim_prescaler);

  if (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
  {
    MotorPulseLab_StopHardware();
    Error_Handler();
  }
}

static void MotorPulseLab_ApplyState(const MotorPulseLab *lab)
{
  uint8_t *message;
  uint16_t message_size;

  if (MotorPulseLab_GetState(lab) == MOTOR_PULSE_LAB_RUNNING)
  {
    MotorPulseLabDirection direction = MotorPulseLab_GetDirection(lab);

    MotorPulseLab_StartHardware(direction, MOTOR_CONTINUOUS_PRESCALER);
    message = direction == MOTOR_PULSE_LAB_DIRECTION_HIGH
                  ? motor_running_cw_message
                  : motor_running_ccw_message;
    message_size = direction == MOTOR_PULSE_LAB_DIRECTION_HIGH
                       ? sizeof(motor_running_cw_message) - 1U
                       : sizeof(motor_running_ccw_message) - 1U;
  }
  else
  {
    MotorPulseLab_StopHardware();
    message = motor_stopped_message;
    message_size = sizeof(motor_stopped_message) - 1U;
  }

  if (HAL_UART_Transmit(&huart1, message, message_size,
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
}

static void PublishRelativePosition(const char *reason)
{
  int position_length;

  if (RelativePositionTracker_IsValid(&relative_position_tracker))
  {
    int32_t position_pulses =
        RelativePositionTracker_GetPositionPulses(&relative_position_tracker);
    int32_t position_tenths_degrees = (position_pulses / 8) * 9;
    int32_t magnitude_tenths_degrees = position_tenths_degrees < 0
                                           ? -position_tenths_degrees
                                           : position_tenths_degrees;

    position_length = snprintf(
        relative_position_message, sizeof(relative_position_message),
        "POSITION theory=%c%ld.%lddeg pulses=%ld reason=%s\r\n",
        position_tenths_degrees < 0 ? '-' : '+',
        (long)(magnitude_tenths_degrees / 10),
        (long)(magnitude_tenths_degrees % 10), (long)position_pulses, reason);
  }
  else
  {
    position_length = snprintf(relative_position_message,
                               sizeof(relative_position_message),
                               "POSITION theory=invalid reason=%s\r\n", reason);
  }

  if ((position_length <= 0) ||
      (position_length >= (int)sizeof(relative_position_message)) ||
      (HAL_UART_Transmit(&huart1, (uint8_t *)relative_position_message,
                         (uint16_t)position_length,
                         BOOT_LOG_TIMEOUT_MS) != HAL_OK))
  {
    Error_Handler();
  }
}

static void PublishMotionLimitRejection(MotionLimitsResult result)
{
  uint8_t *message;
  uint16_t message_size;

  switch (result)
  {
    case MOTION_LIMITS_REJECT_POSITION_INVALID:
      message = motor_profile_move_position_invalid_message;
      message_size = sizeof(motor_profile_move_position_invalid_message) - 1U;
      break;
    case MOTION_LIMITS_REJECT_OUT_OF_RANGE:
      message = motor_profile_move_limit_message;
      message_size = sizeof(motor_profile_move_limit_message) - 1U;
      break;
    case MOTION_LIMITS_REJECT_INVALID_ARGUMENT:
    default:
      message = motor_profile_move_invalid_argument_message;
      message_size = sizeof(motor_profile_move_invalid_argument_message) - 1U;
      break;
  }

  if (HAL_UART_Transmit(&huart1, message, message_size,
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim4)
  {
    uint8_t stage_index_before =
        MotorPulseLab_GetProfileStageIndex(&motor_pulse_lab);

    if (MotorPulseLab_OnPulsePeriod(&motor_pulse_lab))
    {
      if (HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1) != HAL_OK)
      {
        Error_Handler();
      }

      if (HAL_TIM_Base_Stop_IT(&htim4) != HAL_OK)
      {
        Error_Handler();
      }

      HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
      motor_profile_move_complete_pending = true;
    }
    else if (MotorPulseLab_GetState(&motor_pulse_lab) ==
             MOTOR_PULSE_LAB_FIXED_MOVE)
    {
      uint8_t stage_index =
          MotorPulseLab_GetProfileStageIndex(&motor_pulse_lab);

      if (stage_index != stage_index_before)
      {
        if (stage_index >=
            (sizeof(motor_profile_speed_stages) /
             sizeof(motor_profile_speed_stages[0])))
        {
          Error_Handler();
        }

        /* 新分频在下一次 TIM4 更新事件生效，脉冲数量不受影响。 */
        MotorPulseLab_SetTimerPrescaler(
            motor_profile_speed_stages[stage_index].tim_prescaler);
      }
    }
  }
}
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  /* 三个任务对象只保存各自的时间状态，实际执行仍在同一个 while(1) 中。 */
  DebouncedButton key0_button;
  DebouncedButton key1_button;
  DebouncedButton key2_button;
  DebouncedButton key_up_button;
  PeriodicTask key_scan_task;
  PeriodicTask status_task;
  PeriodicTask heartbeat_task;
  uint32_t last_heartbeat_log_ms = HAL_GetTick();
  uint32_t key_scan_count = 0U;
  uint32_t status_task_count = 0U;
  uint32_t heartbeat_count = 0U;

  /* KEY0 为低有效：读到 RESET 表示已按下。 */
  DebouncedButton_Init(&key0_button,
                       HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET);
  DebouncedButton_Init(&key1_button,
                       HAL_GPIO_ReadPin(BOARD_KEY1_GPIO_Port, BOARD_KEY1_Pin) ==
                           GPIO_PIN_RESET);
  DebouncedButton_Init(&key2_button,
                       HAL_GPIO_ReadPin(BOARD_KEY2_GPIO_Port, BOARD_KEY2_Pin) ==
                           GPIO_PIN_RESET);
  DebouncedButton_Init(&key_up_button,
                       HAL_GPIO_ReadPin(BOARD_KEY_UP_GPIO_Port,
                                        BOARD_KEY_UP_Pin) == GPIO_PIN_SET);
  PeriodicTask_Init(&key_scan_task, KEY_SCAN_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&status_task, STATUS_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&heartbeat_task, HEARTBEAT_PERIOD_MS, HAL_GetTick());
  UartRxFrame_Init(&uart_rx_frame);
  VisionFrameParser_Init(&vision_frame_parser);
  CommunicationWatchdog_Init(&communication_watchdog, COMMUNICATION_TIMEOUT_MS);
  FrameSequenceTracker_Init(&frame_sequence_tracker);
  MotorPulseLab_Init(&motor_pulse_lab);
  horizontal_motion_limits = MotionLimits_HorizontalDefault();
  RelativePositionTracker_Init(&relative_position_tracker);
  MotorPulseLab_StopHardware();
  StartUartRxDma();
  /* 所有外设初始化完成后发送启动日志；失败则进入统一错误处理。 */
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

    /* USER CODE BEGIN 3 */
    uint32_t current_tick = HAL_GetTick();

    if (motor_profile_move_complete_pending)
    {
      motor_profile_move_complete_pending = false;
      if (HAL_UART_Transmit(&huart1, motor_profile_move_complete_message,
                            sizeof(motor_profile_move_complete_message) - 1U,
                            BOOT_LOG_TIMEOUT_MS) != HAL_OK)
      {
        Error_Handler();
      }

      /* 从输出轴正面看，已实测 DIR=Low 为逆时针；软件坐标约定逆时针为正。 */
      (void)RelativePositionTracker_ApplyCompletedPulseDelta(
          &relative_position_tracker,
          MotorPulseLab_GetDirection(&motor_pulse_lab) ==
                  MOTOR_PULSE_LAB_DIRECTION_LOW
              ? MOTOR_FIXED_MOVE_PULSES
              : -MOTOR_FIXED_MOVE_PULSES);
      PublishRelativePosition("profile_complete");
    }

    /* 10 ms：采样 KEY0/KEY1，并将抖动过滤与主循环其他工作解耦。 */
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

        /* 连续运动或中途停止没有可确认的完整脉冲增量，坐标立即失效。 */
        RelativePositionTracker_Invalidate(&relative_position_tracker);
        PublishRelativePosition("manual_control");
        (void)MotorPulseLab_Toggle(&motor_pulse_lab);
        MotorPulseLab_ApplyState(&motor_pulse_lab);
      }

      if (DebouncedButton_Update(&key1_button,
                                 HAL_GPIO_ReadPin(BOARD_KEY1_GPIO_Port,
                                                  BOARD_KEY1_Pin) == GPIO_PIN_RESET,
                                 current_tick, KEY_DEBOUNCE_MS))
      {
        uint8_t *direction_message;
        uint16_t direction_message_size;

        if (HAL_UART_Transmit(&huart1, key1_pressed_message,
                              sizeof(key1_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }

        if (MotorPulseLab_ToggleDirection(&motor_pulse_lab))
        {
          direction_message = MotorPulseLab_GetDirection(&motor_pulse_lab) ==
                                      MOTOR_PULSE_LAB_DIRECTION_HIGH
                                  ? motor_direction_cw_message
                                  : motor_direction_ccw_message;
          direction_message_size = MotorPulseLab_GetDirection(&motor_pulse_lab) ==
                                           MOTOR_PULSE_LAB_DIRECTION_HIGH
                                       ? sizeof(motor_direction_cw_message) - 1U
                                       : sizeof(motor_direction_ccw_message) - 1U;
          HAL_GPIO_WritePin(
              MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin,
              MotorPulseLab_GetDirection(&motor_pulse_lab) ==
                      MOTOR_PULSE_LAB_DIRECTION_HIGH
                  ? GPIO_PIN_SET
                  : GPIO_PIN_RESET);
        }
        else
        {
          direction_message = motor_direction_ignored_message;
          direction_message_size = sizeof(motor_direction_ignored_message) - 1U;
        }

        if (HAL_UART_Transmit(&huart1, direction_message, direction_message_size,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }
      }

      if (DebouncedButton_Update(&key2_button,
                                 HAL_GPIO_ReadPin(BOARD_KEY2_GPIO_Port,
                                                  BOARD_KEY2_Pin) == GPIO_PIN_RESET,
                                 current_tick, KEY_DEBOUNCE_MS))
      {
        MotionLimitsResult limit_result = MOTION_LIMITS_ACCEPTED;

        if (HAL_UART_Transmit(&huart1, key2_pressed_message,
                              sizeof(key2_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }

        if (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED)
        {
          if (HAL_UART_Transmit(&huart1, motor_profile_move_ignored_message,
                                sizeof(motor_profile_move_ignored_message) - 1U,
                                BOOT_LOG_TIMEOUT_MS) != HAL_OK)
          {
            Error_Handler();
          }
        }
        else
        {
          int32_t current_pulses =
              RelativePositionTracker_GetPositionPulses(
                  &relative_position_tracker);
          int32_t move_delta_pulses =
              MotorPulseLab_GetDirection(&motor_pulse_lab) ==
                      MOTOR_PULSE_LAB_DIRECTION_LOW
                  ? MOTOR_FIXED_MOVE_PULSES
                  : -MOTOR_FIXED_MOVE_PULSES;
          int32_t target_pulses = 0;

          limit_result = MotionLimits_CheckRelativeMove(
              &horizontal_motion_limits,
              RelativePositionTracker_IsValid(&relative_position_tracker),
              current_pulses, move_delta_pulses, &target_pulses);
          (void)target_pulses;

          if (limit_result != MOTION_LIMITS_ACCEPTED)
          {
            PublishMotionLimitRejection(limit_result);
          }
          else if (MotorPulseLab_StartProfileMove(&motor_pulse_lab))
          {
            MotorPulseLabDirection direction =
                MotorPulseLab_GetDirection(&motor_pulse_lab);
            uint8_t *fixed_move_message =
                direction == MOTOR_PULSE_LAB_DIRECTION_HIGH
                    ? motor_profile_move_cw_message
                    : motor_profile_move_ccw_message;
            uint16_t fixed_move_message_size =
                direction == MOTOR_PULSE_LAB_DIRECTION_HIGH
                    ? sizeof(motor_profile_move_cw_message) - 1U
                    : sizeof(motor_profile_move_ccw_message) - 1U;

            MotorPulseLab_StartFixedHardware(direction);
            if (HAL_UART_Transmit(&huart1, fixed_move_message,
                                  fixed_move_message_size,
                                  BOOT_LOG_TIMEOUT_MS) != HAL_OK)
            {
              Error_Handler();
            }
          }
          else
          {
            if (HAL_UART_Transmit(&huart1, motor_profile_move_ignored_message,
                                  sizeof(motor_profile_move_ignored_message) - 1U,
                                  BOOT_LOG_TIMEOUT_MS) != HAL_OK)
            {
              Error_Handler();
            }
          }
        }

      }

      if (DebouncedButton_Update(&key_up_button,
                                 HAL_GPIO_ReadPin(BOARD_KEY_UP_GPIO_Port,
                                                  BOARD_KEY_UP_Pin) == GPIO_PIN_SET,
                                 current_tick, KEY_DEBOUNCE_MS))
      {
        if (HAL_UART_Transmit(&huart1, key_up_pressed_message,
                              sizeof(key_up_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }

        if (MotorPulseLab_GetState(&motor_pulse_lab) == MOTOR_PULSE_LAB_STOPPED)
        {
          RelativePositionTracker_SetManualZero(&relative_position_tracker);
          PublishRelativePosition("manual_zero");
        }
        else if (HAL_UART_Transmit(&huart1, relative_position_zero_ignored_message,
                                   sizeof(relative_position_zero_ignored_message) - 1U,
                                   BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }
      }
    }

    /* 100 ms：预留的状态任务；当前只计数，用于验证其独立运行。 */
    if (PeriodicTask_IsDue(&status_task, current_tick))
    {
      status_task_count++;
    }

    /* 500 ms：输出心跳、实际间隔和其他任务计数，作为非阻塞调度证据。 */
    if (PeriodicTask_IsDue(&heartbeat_task, current_tick))
    {
      uint32_t heartbeat_interval_ms = current_tick - last_heartbeat_log_ms;
      int heartbeat_length;
      CommunicationState communication_state;

      last_heartbeat_log_ms = current_tick;
      communication_state = CommunicationWatchdog_GetState(&communication_watchdog,
                                                            current_tick);
      heartbeat_length = snprintf(
          heartbeat_message, sizeof(heartbeat_message),
          "t=%lu dt=%lu heartbeat=%lu scans=%lu status=%lu link=%s\r\n",
          (unsigned long)current_tick,
          (unsigned long)heartbeat_interval_ms,
          (unsigned long)++heartbeat_count,
          (unsigned long)key_scan_count,
          (unsigned long)status_task_count,
          CommunicationStateText(communication_state));

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

    /* 主循环按需识别文本命令或喂给二进制帧解析器。 */
    if (uart_rx_frame.pending)
    {
      UartTextCommand command = UartTextCommand_Parse(uart_rx_frame.data,
                                                       uart_rx_frame.size);
      uint8_t *response = NULL;
      uint16_t response_size = 0U;

      if (command == UART_TEXT_COMMAND_PING)
      {
        response = uart_pong_message;
        response_size = sizeof(uart_pong_message) - 1U;
      }
      else if (command == UART_TEXT_COMMAND_STATUS)
      {
        int uart_rx_log_length = snprintf(
            uart_rx_log_message, sizeof(uart_rx_log_message),
            "STATUS size=%u rx=%lu drop=%lu frame_ok=%lu frame_err=%lu last=%u miss=%lu dup=%lu old=%lu link=%s\r\n",
            (unsigned int)uart_rx_frame.size,
            (unsigned long)uart_rx_frame.received_count,
            (unsigned long)uart_rx_frame.dropped_count,
            (unsigned long)vision_frame_parser.valid_count,
            (unsigned long)vision_frame_parser.error_count,
            (unsigned int)(latest_vision_frame_valid
                               ? latest_vision_frame.sequence
                               : 0U),
            (unsigned long)frame_sequence_tracker.missing_count,
            (unsigned long)frame_sequence_tracker.duplicate_count,
            (unsigned long)frame_sequence_tracker.out_of_order_count,
            CommunicationStateText(CommunicationWatchdog_GetState(
                &communication_watchdog, HAL_GetTick())));

        if ((uart_rx_log_length <= 0) ||
            (uart_rx_log_length >= (int)sizeof(uart_rx_log_message)))
        {
          Error_Handler();
        }

        response = (uint8_t *)uart_rx_log_message;
        response_size = (uint16_t)uart_rx_log_length;
      }
      else
      {
        bool frame_accepted = false;
        uint32_t error_count_before = vision_frame_parser.error_count;
        uint16_t byte_index;
        VisionFrame received_vision_frame = {0};
        FrameSequenceResult last_sequence_result = FRAME_SEQUENCE_FIRST;

        for (byte_index = 0U; byte_index < uart_rx_frame.size; byte_index++)
        {
          if (VisionFrameParser_Feed(&vision_frame_parser,
                                     uart_rx_frame.data[byte_index],
                                     &received_vision_frame))
          {
            frame_accepted = true;
            last_sequence_result = FrameSequenceTracker_Observe(
                &frame_sequence_tracker, received_vision_frame.sequence);
            CommunicationWatchdog_OnValidFrame(&communication_watchdog,
                                                HAL_GetTick());

            /* 旧帧或重复帧不应倒退当前视觉目标，但仍证明链路没有超时。 */
            if ((last_sequence_result != FRAME_SEQUENCE_DUPLICATE) &&
                (last_sequence_result != FRAME_SEQUENCE_OUT_OF_ORDER))
            {
              latest_vision_frame = received_vision_frame;
              latest_vision_frame_valid = true;
            }
          }
        }

        if (frame_accepted)
        {
          int uart_rx_log_length = snprintf(
              uart_rx_log_message, sizeof(uart_rx_log_message),
              "FRAME OK seq=%u len=%u order=%s valid=%lu err=%lu miss=%lu\r\n",
              (unsigned int)received_vision_frame.sequence,
              (unsigned int)received_vision_frame.payload_size,
              FrameSequenceResultText(last_sequence_result),
              (unsigned long)vision_frame_parser.valid_count,
              (unsigned long)vision_frame_parser.error_count,
              (unsigned long)frame_sequence_tracker.missing_count);

          if ((uart_rx_log_length <= 0) ||
              (uart_rx_log_length >= (int)sizeof(uart_rx_log_message)))
          {
            Error_Handler();
          }

          response = (uint8_t *)uart_rx_log_message;
          response_size = (uint16_t)uart_rx_log_length;
        }
        else if (vision_frame_parser.error_count != error_count_before)
        {
          int uart_rx_log_length = snprintf(
              uart_rx_log_message, sizeof(uart_rx_log_message),
              "FRAME ERR valid=%lu err=%lu\r\n",
              (unsigned long)vision_frame_parser.valid_count,
              (unsigned long)vision_frame_parser.error_count);

          if ((uart_rx_log_length <= 0) ||
              (uart_rx_log_length >= (int)sizeof(uart_rx_log_message)))
          {
            Error_Handler();
          }

          response = (uint8_t *)uart_rx_log_message;
          response_size = (uint16_t)uart_rx_log_length;
        }
        else if (!VisionFrameParser_IsReceiving(&vision_frame_parser))
        {
          response = uart_unknown_command_message;
          response_size = sizeof(uart_unknown_command_message) - 1U;
        }
      }

      if ((response_size > 0U) &&
          (HAL_UART_Transmit(&huart1, response, response_size,
                             UART_RX_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }

      UartRxFrame_Release(&uart_rx_frame);
    }
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
