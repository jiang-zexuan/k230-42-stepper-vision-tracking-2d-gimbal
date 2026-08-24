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
#include "cmsis_os.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "communication_watchdog.h"
#include "debounced_button.h"
#include "frame_sequence_tracker.h"
#include "motor_pulse_lab.h"
#include "motion_limits.h"
#include "periodic_task.h"
#include "relative_position_tracker.h"
#include "uart_rx_frame.h"
#include "uart_text_line_accumulator.h"
#include "uart_text_command.h"
#include "vision_frame_parser.h"
#include "vision_error.h"
#include "vision_text_result_parser.h"
#include "visual_track_controller.h"
#include "ili9341.h"
#include "ft6336g.h"
#include "es8388.h"
#include "audio_player.h"
#include "panview_ui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* P11 第一阶段只验证调度器；保持 0 时不启动 RTOS，避免原裸机主循环失效。 */
#define PANVIEW_P11_RTOS_DEMO 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* 启动后通过 USART1 输出，作为串口链路可用的第一条证据。 */
static uint8_t boot_message[] = "PanView P10 boot\r\n";
static uint8_t motor_ttl_probe_request[] = {0x00U, 0x15U, 0x6BU};
static uint8_t motor_ttl_option_request[] = {0x01U, 0x1AU, 0x6BU};
static uint8_t motor_ttl_system_status_request[] = {0x01U, 0x43U, 0x7AU, 0x6BU};
/* PanView_AppStep 在裸机主循环和 RTOS 任务之间共用的运行时状态。 */
static DebouncedButton key0_button;
static DebouncedButton key1_button;
static DebouncedButton key2_button;
static DebouncedButton key_up_button;
static PeriodicTask key_scan_task;
static PeriodicTask status_task;
static PeriodicTask motor_feedback_task;
static PeriodicTask heartbeat_task;
static PeriodicTask k230_uart_log_task;
static PeriodicTask touch_poll_task;
static Ft6336gPoint touch_point;
static uint8_t touch_last_count;
static uint16_t touch_last_x;
static uint16_t touch_last_y;
static char touch_runtime_message[96];
static uint32_t last_heartbeat_log_ms;
static uint32_t key_scan_count;
static uint32_t status_task_count;
static uint32_t heartbeat_count;

enum {
  /* 单位：ms；串口发送阻塞等待上限，仅用于当前诊断日志。 */
  BOOT_LOG_TIMEOUT_MS = 100U,
  /* 单位：ms；P03 接收结果日志的串口发送等待上限。 */
  UART_RX_LOG_TIMEOUT_MS = 100U
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
typedef enum
{
  PANVIEW_SAFETY_CLEAR = 0,
  PANVIEW_SAFETY_VISION_TIMEOUT,
  PANVIEW_SAFETY_LIMIT,
  PANVIEW_SAFETY_CONTROL_STALE
} PanViewSafetyFault;

static volatile PanViewSafetyFault safety_fault_latched = PANVIEW_SAFETY_CLEAR;

void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
static bool VisualState_AtLimitFault(void);
static void TouchUi_StopAllMotion(void);
static void PublishRelativePosition(const char *reason);
static void Pitch_EnableHold(void);
static const char *PanView_SafetyFaultText(PanViewSafetyFault fault);
static void PanView_SafetyStop(PanViewSafetyFault fault);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

enum {
  /* 单位：ms；P02 轮询与消抖设计参数，仅适用于当前 KEY0 诊断任务。 */
  KEY_SCAN_PERIOD_MS = 10U,
  KEY_DEBOUNCE_MS = 20U,
  STATUS_PERIOD_MS = 100U,
  /* 单位：ms；运动期间读取一次 X42S 状态，观察真实速度和跟随误差。 */
  MOTOR_FEEDBACK_PERIOD_MS = 100U,
  HEARTBEAT_PERIOD_MS = 5000U,
  /* 单位：ms；P10 触摸底层验证先采用轮询，暂不引入 EXTI 并发。 */
  TOUCH_POLL_PERIOD_MS = 50U,
  /* 单位：ms；P08 只汇总 K230 接收统计，避免逐帧刷屏。 */
  K230_UART_LOG_PERIOD_MS = 1000U,
  /* 单位：ms；视觉结果超过此时间未更新就停止水平 STEP。 */
  VISUAL_TARGET_TIMEOUT_MS = 300U,
  /* 单位：ms；控制任务超过此时间没有打心跳就执行独立安全停机。 */
  SAFETY_CONTROL_STALE_TIMEOUT_MS = 200U,
  /* 单位：ms；目标误差同时落入死区并持续此时间，才进入 LOCKED。 */
  VISUAL_LOCK_HOLD_MS = 500U,
  /* 单位：ms；命中音效最短间隔，避免锁定状态抖动造成重复播放。 */
  VISUAL_AUDIO_COOLDOWN_MS = 500U,
  /* 单位：ms；仅局部覆盖变化字段后的 TFT 遥测刷新周期。 */
  VISUAL_STATUS_REFRESH_MS = 500U,
  /* 单位：ms；P03 通信超时观察参数，不是最终云台安全阈值。 */
  COMMUNICATION_TIMEOUT_MS = 1000U,
  /* 单位：Hz；TIM4 配置为 16 MHz / (3199 + 1) / (9 + 1)，用于 P05 空载验收。 */
  MOTOR_STEP_FREQUENCY_HZ = 500U,
  /* 单位：无；TIM4 使用固定 Period=9、Pulse=5 时的 500 Hz 分频值。 */
  MOTOR_CONTINUOUS_PRESCALER = 3199U,
  /* MSP2834 触摸/显示逻辑坐标范围：X=0..239，Y=0..319。 */
  TOUCH_SCREEN_MAX_X = 239U,
  TOUCH_SCREEN_MAX_Y = 319U,
  TOUCH_SCREEN_MID_X = 120U,
  TOUCH_SCREEN_MID_Y = 160U,
  /* 单位：deg；P05 固定运动的目标转角，基于当前 16 细分配置。 */
  MOTOR_FIXED_MOVE_ANGLE_DEGREES = 90U,
  /* 单位：脉冲；90 deg * 3200 pulses/rev / 360 deg/rev，X42S 当前为 16 细分。 */
  MOTOR_FIXED_MOVE_PULSES = 800U,
  /* 单位：脉冲/圈；X42S 当前 16 细分设置。 */
  MOTOR_PULSES_PER_REVOLUTION = 3200U
};

enum {
  /* P07 第一阶段只验证俯仰链路，100 脉冲约 11.25 度（16 细分）。 */
  PITCH_TEST_PULSES = 100U,
  PITCH_TEST_PRESCALER = 3199U,
  /* 当前 MStep=16：3200 脉冲/圈，±45°对应 ±400 脉冲。 */
  PITCH_LIMIT_PULSES = 400U,
  DUAL_TEST_PULSES = 100U
};

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
static uint8_t k230_uart_rx_dma_buffer[UART_RX_FRAME_CAPACITY];
static UartRxFrame k230_uart_rx_frame;
static UartTextLineAccumulator k230_uart_line_accumulator;
static char uart_rx_log_message[128];
static char k230_uart_log_message[320];
static uint16_t k230_uart_last_frame_size;
static VisionTextResult latest_k230_result;
static VisionError latest_k230_error;
/* P10 到位稳定调参：放宽中心死区，限制速度变化率，抑制到位后的机械抖动。 */
static VisualTrackControllerConfig visual_track_config =
    {30, 3.0f, 0.02f, 0.15f, 200.0f, 800, 5000};
static VisualTrackControllerState pan_track_state;
static VisualTrackControllerState pitch_track_state;
static int32_t latest_pan_speed_target;
static int32_t latest_pitch_speed_target;
static bool latest_k230_result_valid;
static uint32_t k230_text_valid_count;
static uint32_t k230_text_error_count;
/* 触摸 MODE 按钮控制的视觉跟踪总开关，默认保持 P09 行为。 */
static bool visual_tracking_enabled = true;
static uint8_t uart_pong_message[] = "PONG\r\n";
static uint8_t uart_unknown_command_message[] = "ERR unknown command\r\n";
static uint8_t pitch_up_message[] = "PITCH state=running dir=up pulses=100\r\n";
static uint8_t pitch_down_message[] = "PITCH state=running dir=down pulses=100\r\n";
static uint8_t pitch_stop_message[] = "PITCH state=stopped\r\n";
static uint8_t pitch_busy_message[] = "PITCH command ignored: running\r\n";
static uint8_t pitch_not_zeroed_message[] =
    "PITCH command rejected: press KEY_UP at mechanical center\r\n";
static uint8_t pitch_zero_message[] =
    "PITCH zero=accepted position=0 limit=plus_or_minus_45deg\r\n";
static uint8_t pitch_limit_message[] =
    "PITCH command rejected: soft_limit_plus_or_minus_45deg\r\n";
static uint8_t pitch_rezero_message[] =
    "PITCH stop interrupted move: press KEY_UP to re-zero\r\n";
static uint8_t dual_test_message[] =
    "DUAL state=running pan_pulses=100 pitch_pulses=100\r\n";
static uint8_t dual_test_neg_message[] =
    "DUAL state=running dir=negative pan_pulses=100 pitch_pulses=100\r\n";
static uint8_t dual_test_rejected_message[] =
    "DUAL command rejected: zero_or_limit_or_busy\r\n";
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
static volatile uint16_t pitch_remaining_pulses;
static volatile bool pitch_running;
static volatile bool pitch_zeroed;
static volatile bool control_takeover_active;
static volatile int32_t pitch_position_pulses;
static volatile int32_t pitch_active_delta_pulses;
static volatile bool dual_test_running;
static volatile uint16_t dual_test_remaining_pulses;
static volatile int32_t dual_test_direction_sign;
static volatile bool visual_pan_running;
static volatile int32_t visual_pan_direction_sign;
static volatile bool visual_pitch_running;
static volatile int32_t visual_pitch_direction_sign;
static uint32_t last_k230_valid_tick;
static volatile uint32_t last_pan_view_step_tick;

typedef enum
{
  VISUAL_STATE_SEARCH = 0,
  VISUAL_STATE_TRACKING,
  VISUAL_STATE_LOCKED,
  VISUAL_STATE_LOST,
  VISUAL_STATE_FAULT
} VisualState;

static VisualState visual_state = VISUAL_STATE_SEARCH;
static bool visual_state_initialized;
static uint32_t visual_lock_candidate_tick;
static uint32_t visual_audio_last_tick;
static bool visual_audio_played;
static bool tft_ready;
static bool touch_ready;
static uint16_t ui_touch_raw_x;
static uint16_t ui_touch_raw_y;
static uint16_t ui_touch_screen_x;
static uint16_t ui_touch_screen_y;
static uint8_t ui_touch_count;
/* P11 临时故障定位：只在视觉状态变化时标记 TFT 重绘前后。 */
static int32_t ui_trace_last_state = -1;

static PanViewUiData MainUiData(void)
{
  PanViewUiData data;
  (void)memset(&data, 0, sizeof(data));
  switch (visual_state)
  {
    case VISUAL_STATE_TRACKING: data.state = UI_STATE_TRACKING; break;
    case VISUAL_STATE_LOCKED: data.state = UI_STATE_LOCKED; break;
    case VISUAL_STATE_LOST: data.state = UI_STATE_LOST; break;
    case VISUAL_STATE_FAULT: data.state = UI_STATE_FAULT; break;
    case VISUAL_STATE_SEARCH:
    default: data.state = UI_STATE_SEARCH; break;
  }
  data.running = control_takeover_active;
  data.target_present = latest_k230_result_valid && latest_k230_result.target_present;
  data.error_x = latest_k230_result_valid ? latest_k230_error.error_x : 0;
  data.error_y = latest_k230_result_valid ? latest_k230_error.error_y : 0;
  data.pan_position = RelativePositionTracker_GetPositionPulses(&relative_position_tracker);
  data.pitch_position = pitch_position_pulses;
  data.pan_speed = latest_k230_result_valid ? latest_pan_speed_target : 0;
  data.pitch_speed = latest_k230_result_valid ? latest_pitch_speed_target : 0;
  data.rx_frames = k230_uart_rx_frame.received_count;
  data.rx_drops = k230_uart_rx_frame.dropped_count;
  data.parse_errors = k230_text_error_count;
  data.last_frame_age_ms = last_k230_valid_tick == 0U ? 0U : HAL_GetTick() - last_k230_valid_tick;
  data.position_valid = RelativePositionTracker_IsValid(&relative_position_tracker);
  data.pitch_zeroed = pitch_zeroed;
  data.tft_ready = tft_ready;
  data.touch_ready = touch_ready;
  data.touch_raw_x = ui_touch_raw_x;
  data.touch_raw_y = ui_touch_raw_y;
  data.touch_screen_x = ui_touch_screen_x;
  data.touch_screen_y = ui_touch_screen_y;
  data.touch_count = ui_touch_count;
  data.pan_limit = VisualState_AtLimitFault();
  data.pitch_limit = (pitch_position_pulses >= (int32_t)PITCH_LIMIT_PULSES) ||
                     (pitch_position_pulses <= -(int32_t)PITCH_LIMIT_PULSES);
  return data;
}

static void ExecuteManualZeroStart(void)
{
  if ((MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED) ||
      pitch_running || dual_test_running || visual_pan_running || visual_pitch_running)
  {
    return;
  }
  RelativePositionTracker_SetManualZero(&relative_position_tracker);
  pitch_zeroed = true;
  pitch_position_pulses = 0;
  pitch_active_delta_pulses = 0;
  control_takeover_active = true;
  visual_tracking_enabled = true;
  safety_fault_latched = PANVIEW_SAFETY_CLEAR;
  last_k230_valid_tick = HAL_GetTick();
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);
  Pitch_EnableHold();
  PublishRelativePosition("manual_zero");
  (void)PanView_Uart1Transmit(&huart1, pitch_zero_message,
                          sizeof(pitch_zero_message) - 1U, BOOT_LOG_TIMEOUT_MS);
}

static void MainUi_HandleEvent(UiEvent event)
{
  switch (event)
  {
    case UI_EVENT_START_STOP:
      if (control_takeover_active)
      {
        TouchUi_StopAllMotion();
        visual_tracking_enabled = false;
      }
      else
      {
        ExecuteManualZeroStart();
      }
      break;
    case UI_EVENT_OPEN_SETTINGS:
      TouchUi_StopAllMotion(); visual_tracking_enabled = false;
      PanViewUi_Navigate(UI_PAGE_SETTINGS);
      break;
    case UI_EVENT_LANGUAGE_ZH: PanViewUi_SetLanguage(UI_LANGUAGE_ZH); break;
    case UI_EVENT_LANGUAGE_EN: PanViewUi_SetLanguage(UI_LANGUAGE_EN); break;
    case UI_EVENT_THEME_BLACK_GOLD: PanViewUi_SetTheme(UI_THEME_BLACK_GOLD); break;
    case UI_EVENT_THEME_NEON: PanViewUi_SetTheme(UI_THEME_NEON); break;
    case UI_EVENT_THEME_MONO: PanViewUi_SetTheme(UI_THEME_MONO); break;
    case UI_EVENT_THEME_FLUORESCENT_GREEN:
      PanViewUi_SetTheme(UI_THEME_FLUORESCENT_GREEN);
      break;
    case UI_EVENT_OPEN_DIAGNOSTIC:
      TouchUi_StopAllMotion(); visual_tracking_enabled = false;
      PanViewUi_Navigate(UI_PAGE_DIAGNOSTIC);
      break;
    case UI_EVENT_BACK:
      TouchUi_StopAllMotion(); visual_tracking_enabled = false;
      PanViewUi_Navigate(UI_PAGE_RUN);
      break;
    default: break;
  }
}

void PanView_ApplyUiTouch(uint16_t screen_x, uint16_t screen_y)
{
  MainUi_HandleEvent(PanViewUi_HandleTouch(screen_x, screen_y, true));
}

/*
 * 只读探测 X42S TTL 链路。依据电机手册 5.6.30：发送 00 15 6B，
 * 返回 Addr 15 Addr 6B。探测不会产生运动，也不会修改参数。
 */
static bool MotorTtlProbe(void)
{
  uint8_t response[4] = {0U};
  char message[96];
  int message_length;
  HAL_StatusTypeDef receive_status = HAL_ERROR;

  if (HAL_UART_Transmit(&huart3, motor_ttl_probe_request,
                        sizeof(motor_ttl_probe_request),
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    message_length = snprintf(message, sizeof(message),
                              "MOTOR TTL probe=tx_error\r\n");
  }
  else
  {
    receive_status = HAL_UART_Receive(&huart3, response, sizeof(response),
                                      BOOT_LOG_TIMEOUT_MS);
    if ((receive_status == HAL_OK) && (response[1] == 0x15U) &&
        (response[3] == 0x6BU))
    {
      message_length = snprintf(message, sizeof(message),
                                "MOTOR TTL probe=ok id=%u raw=%02X %02X %02X %02X\r\n",
                                (unsigned int)response[2], response[0],
                                response[1], response[2], response[3]);
    }
    else
    {
      message_length = snprintf(message, sizeof(message),
                                "MOTOR TTL probe=timeout_or_bad_frame status=%d raw=%02X %02X %02X %02X\r\n",
                                (int)receive_status, response[0], response[1],
                                response[2], response[3]);
    }
  }

  if ((message_length <= 0) ||
      (message_length >= (int)sizeof(message)) ||
      (PanView_Uart1Transmit(&huart1, (uint8_t *)message, (uint16_t)message_length,
                         BOOT_LOG_TIMEOUT_MS) != HAL_OK))
  {
    Error_Handler();
  }

  return (message_length > 0) && (receive_status == HAL_OK) &&
         (response[1] == 0x15U) && (response[3] == 0x6BU);
}

/*
 * 读取 X42S 选项状态。依据电机手册 5.6.4：01 1A 6B，返回 5 字节。
 * 返回的 flags 用于确定后续实时位置数据的单位，整个操作仍然只读。
 */
static void MotorTtlReadOption(void)
{
  uint8_t response[5] = {0U};
  char message[128];
  int message_length;
  HAL_StatusTypeDef receive_status = HAL_TIMEOUT;

  if (HAL_UART_Transmit(&huart3, motor_ttl_option_request,
                        sizeof(motor_ttl_option_request),
                        BOOT_LOG_TIMEOUT_MS) == HAL_OK)
  {
    receive_status = HAL_UART_Receive(&huart3, response, sizeof(response),
                                      BOOT_LOG_TIMEOUT_MS);
  }

  if ((receive_status == HAL_OK) && (response[0] == 0x01U) &&
      (response[1] == 0x1AU) && (response[4] == 0x6BU))
  {
    uint16_t flags = ((uint16_t)response[2] << 8U) | response[3];
    const char *motor_type = (flags & (1U << 0)) ? "0.9deg" : "1.8deg";
    const char *firmware = (flags & (1U << 1)) ? "Emm" : "X";
    const char *control_mode = (flags & (1U << 2)) ? "closed" : "open";
    const char *positive_direction = (flags & (1U << 4)) ? "CCW" : "CW";

    message_length = snprintf(
        message, sizeof(message),
        "MOTOR CFG option=ok flags=%04X motor=%s fw=%s mode=%s dir=%s\r\n",
        (unsigned int)flags, motor_type, firmware, control_mode,
        positive_direction);
  }
  else
  {
    message_length = snprintf(
        message, sizeof(message),
        "MOTOR CFG option=timeout_or_bad_frame status=%d raw=%02X %02X %02X %02X %02X\r\n",
        (int)receive_status, response[0], response[1], response[2],
        response[3], response[4]);
  }

  if ((message_length <= 0) ||
      (message_length >= (int)sizeof(message)) ||
      (PanView_Uart1Transmit(&huart1, (uint8_t *)message, (uint16_t)message_length,
                         BOOT_LOG_TIMEOUT_MS) != HAL_OK))
  {
    Error_Handler();
  }
}

/*
 * 读取实时位置和位置误差。依据电机手册 5.5.13/5.5.14，两个返回帧都为：
 * Addr、功能码、符号、4 字节数据、校验码。当前已确认是 Emm 固件，
 * 每 65536 个计数对应一圈 360 度；位置值可跨圈累积。这里暂时只做诊断，
 * 不参与控制。
 */
static void MotorTtlReadPositionAndError(void)
{
  uint8_t requests[2][3] = {
      {0x01U, 0x36U, 0x6BU},
      {0x01U, 0x37U, 0x6BU}};
  uint8_t responses[2][8] = {{0U}};
  uint32_t raw_values[2] = {0U, 0U};
  uint32_t angle_hundredths[2] = {0U, 0U};
  char message[160];
  int message_length;
  uint8_t index;

  for (index = 0U; index < 2U; index++)
  {
    if (HAL_UART_Transmit(&huart3, requests[index], sizeof(requests[index]),
                          BOOT_LOG_TIMEOUT_MS) != HAL_OK ||
        HAL_UART_Receive(&huart3, responses[index], sizeof(responses[index]),
                         BOOT_LOG_TIMEOUT_MS) != HAL_OK)
    {
      message_length = snprintf(
          message, sizeof(message),
          "MOTOR FB read=%s failed raw=%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
          index == 0U ? "position" : "error", responses[index][0],
          responses[index][1], responses[index][2], responses[index][3],
          responses[index][4], responses[index][5], responses[index][6],
          responses[index][7]);

      if ((message_length <= 0) ||
          (PanView_Uart1Transmit(&huart1, (uint8_t *)message,
                             (uint16_t)message_length,
                             BOOT_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }
      return;
    }

    if ((responses[index][0] != 0x01U) ||
        (responses[index][1] != (index == 0U ? 0x36U : 0x37U)) ||
        (responses[index][7] != 0x6BU))
    {
      message_length = snprintf(
          message, sizeof(message),
          "MOTOR FB read=%s bad_frame raw=%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
          index == 0U ? "position" : "error", responses[index][0],
          responses[index][1], responses[index][2], responses[index][3],
          responses[index][4], responses[index][5], responses[index][6],
          responses[index][7]);

      if ((message_length <= 0) ||
          (PanView_Uart1Transmit(&huart1, (uint8_t *)message,
                             (uint16_t)message_length,
                             BOOT_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }
      return;
    }

    raw_values[index] = ((uint32_t)responses[index][3] << 24U) |
                        ((uint32_t)responses[index][4] << 16U) |
                        ((uint32_t)responses[index][5] << 8U) |
                        responses[index][6];
    angle_hundredths[index] = (uint32_t)(
        ((uint64_t)raw_values[index] * 36000ULL) / 65536ULL);
  }

  message_length = snprintf(
      message, sizeof(message),
      "MOTOR FB position=%c%lu.%02u deg raw=%08lX error=%c%lu.%02u deg raw=%08lX\r\n",
      responses[0][2] == 0U ? '+' : '-',
      (unsigned long)(angle_hundredths[0] / 100U),
      (unsigned int)(angle_hundredths[0] % 100U), (unsigned long)raw_values[0],
      responses[1][2] == 0U ? '+' : '-',
      (unsigned long)(angle_hundredths[1] / 100U),
      (unsigned int)(angle_hundredths[1] % 100U), (unsigned long)raw_values[1]);

  if ((message_length <= 0) ||
      (PanView_Uart1Transmit(&huart1, (uint8_t *)message, (uint16_t)message_length,
                         BOOT_LOG_TIMEOUT_MS) != HAL_OK))
  {
    Error_Handler();
  }
}

/*
 * 读取 X42S Emm 系统状态。依据手册 5.8.2，回包固定 31 字节：
 * 电压/电流单位为 mV/mA，位置相关计数每 65536 对应 360 度，
 * 转速单位为 RPM。该函数只读，不参与运动控制。
 */
static void MotorTtlReadSystemStatus(void)
{
  uint8_t response[31] = {0U};
  uint16_t bus_voltage_mv;
  uint16_t phase_current_ma;
  uint16_t encoder_value;
  uint16_t speed_rpm;
  uint32_t target_position;
  uint32_t realtime_position;
  uint32_t position_error;
  uint32_t target_angle_hundredths;
  uint32_t realtime_angle_hundredths;
  uint32_t error_angle_hundredths;
  const char *calibration_text;
  const char *encoder_text;
  const char *homing_text;
  const char *protection_text;
  const char *reached_text;
  const char *enabled_text;
  const char *stall_text;
  const char *limit_left_text;
  const char *limit_right_text;
  char message[320];
  int message_length;
  HAL_StatusTypeDef receive_status = HAL_TIMEOUT;

  if (HAL_UART_Transmit(&huart3, motor_ttl_system_status_request,
                        sizeof(motor_ttl_system_status_request),
                        BOOT_LOG_TIMEOUT_MS) == HAL_OK)
  {
    receive_status = HAL_UART_Receive(&huart3, response, sizeof(response),
                                      BOOT_LOG_TIMEOUT_MS);
  }

  if ((receive_status != HAL_OK) || (response[0] != 0x01U) ||
      (response[1] != 0x43U) || (response[2] != 0x1FU) ||
      (response[3] != 0x09U) || (response[30] != 0x6BU))
  {
    message_length = snprintf(
        message, sizeof(message),
        "MOTOR SYS read=failed status=%d raw=%02X %02X %02X %02X ... %02X\r\n",
        (int)receive_status, response[0], response[1], response[2],
        response[3], response[30]);
  }
  else
  {
    bus_voltage_mv = ((uint16_t)response[4] << 8U) | response[5];
    phase_current_ma = ((uint16_t)response[6] << 8U) | response[7];
    encoder_value = ((uint16_t)response[8] << 8U) | response[9];
    target_position = ((uint32_t)response[11] << 24U) |
                      ((uint32_t)response[12] << 16U) |
                      ((uint32_t)response[13] << 8U) | response[14];
    speed_rpm = ((uint16_t)response[16] << 8U) | response[17];
    realtime_position = ((uint32_t)response[19] << 24U) |
                        ((uint32_t)response[20] << 16U) |
                        ((uint32_t)response[21] << 8U) | response[22];
    position_error = ((uint32_t)response[24] << 24U) |
                     ((uint32_t)response[25] << 16U) |
                     ((uint32_t)response[26] << 8U) | response[27];
    target_angle_hundredths = (uint32_t)(((uint64_t)target_position * 36000ULL) /
                                         65536ULL);
    realtime_angle_hundredths = (uint32_t)(((uint64_t)realtime_position * 36000ULL) /
                                           65536ULL);
    error_angle_hundredths = (uint32_t)(((uint64_t)position_error * 36000ULL) /
                                        65536ULL);

    calibration_text = (response[28] & 0x02U) != 0U ? "ready" : "not_ready";
    encoder_text = (response[28] & 0x01U) != 0U ? "ready" : "error";
    if ((response[28] & 0x0CU) == 0x04U)
    {
      homing_text = "running";
    }
    else if ((response[28] & 0x0CU) == 0x08U)
    {
      homing_text = "failed";
    }
    else
    {
      homing_text = "idle";
    }
    protection_text = (response[28] & 0x30U) == 0U ? "clear" : "triggered";
    reached_text = (response[29] & 0x02U) != 0U ? "yes" : "no";
    enabled_text = (response[29] & 0x01U) != 0U ? "yes" : "no";
    stall_text = (response[29] & 0x0CU) == 0U ? "clear" : "triggered";
    limit_left_text = (response[29] & 0x10U) != 0U ? "high" : "low";
    limit_right_text = (response[29] & 0x20U) != 0U ? "high" : "low";

    message_length = snprintf(
        message, sizeof(message),
        "MOTOR SYS bus=%umV current=%umA encoder=%u target=%c%lu.%02udeg "
        "speed=%c%uRPM position=%c%lu.%02udeg error=%c%lu.%02udeg "
        "home=0x%02X(cal=%s,enc=%s,homing=%s,protect=%s) "
        "state=0x%02X(reached=%s,enabled=%s,stall=%s,left=%s,right=%s)\r\n",
        (unsigned int)bus_voltage_mv, (unsigned int)phase_current_ma,
        (unsigned int)encoder_value, response[10] == 0U ? '+' : '-',
        (unsigned long)(target_angle_hundredths / 100U),
        (unsigned int)(target_angle_hundredths % 100U),
        response[15] == 0U ? '+' : '-', (unsigned int)speed_rpm,
        response[18] == 0U ? '+' : '-',
        (unsigned long)(realtime_angle_hundredths / 100U),
        (unsigned int)(realtime_angle_hundredths % 100U),
        response[23] == 0U ? '+' : '-',
        (unsigned long)(error_angle_hundredths / 100U),
        (unsigned int)(error_angle_hundredths % 100U),
        response[28], calibration_text, encoder_text, homing_text,
        protection_text, response[29], reached_text, enabled_text, stall_text,
        limit_left_text, limit_right_text);
  }

  if ((message_length <= 0) ||
      (message_length >= (int)sizeof(message)) ||
      (PanView_Uart1Transmit(&huart1, (uint8_t *)message, (uint16_t)message_length,
                         BOOT_LOG_TIMEOUT_MS) != HAL_OK))
  {
    Error_Handler();
  }
}

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

static void StartK230UartRxDma(void)
{
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart2, k230_uart_rx_dma_buffer,
                                   sizeof(k230_uart_rx_dma_buffer)) != HAL_OK)
  {
    Error_Handler();
  }

  /* K230 由 IDLE 标记一段发送结束，不需要半满中断。 */
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
}

/*
 * HAL 在 UART 空闲或 DMA 缓冲区写满时调用此函数。
 * 这里只做短时间的数据保存与 DMA 重启；字符串格式化和串口输出留给主循环。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart == &huart1)
  {
    (void)UartRxFrame_Store(&uart_rx_frame, uart_rx_dma_buffer, size);
    StartUartRxDma();
  }
  else if (huart == &huart2)
  {
    (void)UartTextLineAccumulator_Consume(
        &k230_uart_line_accumulator, &k230_uart_rx_frame,
        k230_uart_rx_dma_buffer, size);
    StartK230UartRxDma();
  }
}

/* P05 只把状态机结果映射到硬件动作，不在这里实现速度规划或视觉跟踪。 */
static void MotorPulseLab_SetTimerPrescaler(uint16_t prescaler)
{
  __HAL_TIM_SET_PRESCALER(&htim4, prescaler);
}

static void Pitch_StopPulses(void)
{
  (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
  (void)HAL_TIM_Base_Stop_IT(&htim3);
  pitch_running = false;
  pitch_remaining_pulses = 0U;
}

static void Pitch_DisableHardware(void)
{
  Pitch_StopPulses();
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_RESET);
}

static void Pitch_EnableHold(void)
{
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_SET);
}

static bool Pitch_StartTestMove(GPIO_PinState direction,
                                int32_t delta_pulses)
{
  if (pitch_running || dual_test_running || visual_pitch_running)
  {
    return false;
  }

  __HAL_TIM_SET_PRESCALER(&htim3, PITCH_TEST_PRESCALER);
  __HAL_TIM_SET_COUNTER(&htim3, 0U);
  HAL_GPIO_WritePin(PITCH_DIR_GPIO_Port, PITCH_DIR_Pin, direction);
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_SET);
  pitch_active_delta_pulses = delta_pulses;
  pitch_remaining_pulses = PITCH_TEST_PULSES;
  pitch_running = true;
  if ((HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK) ||
      (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK))
  {
    Pitch_DisableHardware();
    Error_Handler();
  }
  return true;
}

static void Dual_StopHardware(void)
{
  (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
  (void)HAL_TIM_Base_Stop_IT(&htim3);
  (void)HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
  (void)HAL_TIM_Base_Stop_IT(&htim4);
  dual_test_running = false;
  dual_test_remaining_pulses = 0U;
}

static bool Dual_StartTestMove(int32_t direction_sign)
{
  int32_t pan_delta_pulses = direction_sign * (int32_t)DUAL_TEST_PULSES;
  int32_t pitch_delta_pulses = direction_sign * (int32_t)DUAL_TEST_PULSES;
  int32_t pan_target_pulses = 0;

  if (dual_test_running || pitch_running || visual_pan_running ||
      visual_pitch_running ||
      (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED) ||
      !pitch_zeroed || !control_takeover_active)
  {
    return false;
  }

  if (MotionLimits_CheckRelativeMove(
          &horizontal_motion_limits,
          RelativePositionTracker_IsValid(&relative_position_tracker),
          RelativePositionTracker_GetPositionPulses(&relative_position_tracker),
          pan_delta_pulses, &pan_target_pulses) != MOTION_LIMITS_ACCEPTED)
  {
    return false;
  }

  if ((pitch_position_pulses + pitch_delta_pulses) >
          (int32_t)PITCH_LIMIT_PULSES ||
      (pitch_position_pulses + pitch_delta_pulses) <
          -(int32_t)PITCH_LIMIT_PULSES)
  {
    return false;
  }

  HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin,
                    direction_sign > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(PITCH_DIR_GPIO_Port, PITCH_DIR_Pin,
                    direction_sign > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  __HAL_TIM_SET_PRESCALER(&htim4, MOTOR_CONTINUOUS_PRESCALER);
  __HAL_TIM_SET_PRESCALER(&htim3, PITCH_TEST_PRESCALER);
  __HAL_TIM_SET_COUNTER(&htim4, 0U);
  __HAL_TIM_SET_COUNTER(&htim3, 0U);
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_SET);
  dual_test_remaining_pulses = DUAL_TEST_PULSES;
  dual_test_direction_sign = direction_sign;
  dual_test_running = true;

  if ((HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK) ||
      (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK) ||
      (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK) ||
      (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK))
  {
    Dual_StopHardware();
    Error_Handler();
  }

  return true;
}

static void MotorPulseLab_SetTimerFrequency(uint16_t frequency_hz)
{
  enum
  {
    /* 单位：Hz；当前 TIM4 输入时钟实测配置为 16 MHz，ARR=9。 */
    TIM4_INPUT_CLOCK_HZ = 16000000U,
    TIM4_AUTO_RELOAD_COUNTS = 10U
  };
  uint32_t divider = TIM4_INPUT_CLOCK_HZ /
                     ((uint32_t)frequency_hz * TIM4_AUTO_RELOAD_COUNTS);

  if (divider == 0U)
  {
    Error_Handler();
  }

  MotorPulseLab_SetTimerPrescaler((uint16_t)(divider - 1U));
}

/* P10：TIM3 与 TIM4 同样使用 16 MHz 输入时钟、ARR=9，单位为脉冲/s。 */
static void VisualPitch_SetTimerFrequency(uint16_t frequency_hz)
{
  enum
  {
    TIM3_INPUT_CLOCK_HZ = 16000000U,
    TIM3_AUTO_RELOAD_COUNTS = 10U
  };
  uint32_t divider = TIM3_INPUT_CLOCK_HZ /
                     ((uint32_t)frequency_hz * TIM3_AUTO_RELOAD_COUNTS);

  if (divider == 0U)
  {
    Error_Handler();
  }

  __HAL_TIM_SET_PRESCALER(&htim3, (uint16_t)(divider - 1U));
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

/* P09：停止视觉水平轴；已接管时保持 EN，避免轴被手动转动。 */
static void VisualPan_StopHardware(void)
{
  if (visual_pan_running)
  {
    (void)HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    (void)HAL_TIM_Base_Stop_IT(&htim4);
    visual_pan_running = false;
  }

  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin,
                    control_takeover_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* P10：停止视觉俯仰轴；接管期间保持 EN，防止轴被手动转动。 */
static void VisualPitch_StopHardware(void)
{
  if (visual_pitch_running)
  {
    (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
    (void)HAL_TIM_Base_Stop_IT(&htim3);
    visual_pitch_running = false;
  }

  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin,
                    control_takeover_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* P10：把带符号俯仰速度转换为 TIM3 STEP 输出。正值为向上，负值为向下。 */
static void VisualPitch_ApplySpeed(int32_t speed_pulses_per_second)
{
  int32_t magnitude;
  int32_t direction_sign;
  uint16_t timer_frequency;

  if (!control_takeover_active || !pitch_zeroed || pitch_running ||
      dual_test_running ||
      (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED))
  {
    VisualPitch_StopHardware();
    return;
  }

  if (speed_pulses_per_second == 0)
  {
    VisualPitch_StopHardware();
    return;
  }

  direction_sign = speed_pulses_per_second > 0 ? 1 : -1;
  magnitude = speed_pulses_per_second > 0 ? speed_pulses_per_second
                                          : -speed_pulses_per_second;
  timer_frequency = (uint16_t)(magnitude < 25 ? 25 : magnitude);

  /* 方向翻转前先停脉冲，保证 DIR 不在脉冲输出期间变化。 */
  if (visual_pitch_running &&
      (visual_pitch_direction_sign != direction_sign))
  {
    VisualPitch_StopHardware();
  }

  if (!visual_pitch_running)
  {
    int32_t next_position = pitch_position_pulses + direction_sign;

    if ((next_position < -(int32_t)PITCH_LIMIT_PULSES) ||
        (next_position > (int32_t)PITCH_LIMIT_PULSES))
    {
      VisualPitch_StopHardware();
      return;
    }

    VisualPitch_SetTimerFrequency(timer_frequency);
    HAL_GPIO_WritePin(PITCH_DIR_GPIO_Port, PITCH_DIR_Pin,
                      direction_sign > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_SET);
    visual_pitch_direction_sign = direction_sign;
    visual_pitch_running = true;
    if ((HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK) ||
        (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK))
    {
      VisualPitch_StopHardware();
      Error_Handler();
    }
  }
  else
  {
    VisualPitch_SetTimerFrequency(timer_frequency);
  }
}

/* P09 第一版：把抽象速度目标转换成 TIM4 STEP 频率，单位为脉冲/s。 */
static void VisualPan_ApplySpeed(int32_t speed_pulses_per_second)
{
  int32_t magnitude;
  int32_t direction_sign;
  uint16_t timer_frequency;

  if (!control_takeover_active || pitch_running || dual_test_running ||
      (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED) ||
      !RelativePositionTracker_IsValid(&relative_position_tracker))
  {
    VisualPan_StopHardware();
    return;
  }

  if (speed_pulses_per_second == 0)
  {
    VisualPan_StopHardware();
    return;
  }

  direction_sign = speed_pulses_per_second > 0 ? 1 : -1;
  magnitude = speed_pulses_per_second > 0 ? speed_pulses_per_second
                                          : -speed_pulses_per_second;
  /* 低于 25 脉冲/s 时仍保持一个可计算的定时器频率；死区通常会先把
   * 这类小误差变成 0，实际参数仍需结合电机实测。 */
  timer_frequency = (uint16_t)(magnitude < 25 ? 25 : magnitude);
  MotorPulseLab_SetTimerFrequency(timer_frequency);

  /* 方向变化前先停止 STEP，确保 DIR 不在脉冲输出期间翻转。 */
  if (visual_pan_running && (visual_pan_direction_sign != direction_sign))
  {
    VisualPan_StopHardware();
  }

  if (!visual_pan_running)
  {
    int32_t position = RelativePositionTracker_GetPositionPulses(
        &relative_position_tracker);
    int32_t next_position = position + direction_sign;

    if ((next_position < horizontal_motion_limits.min_pulses) ||
        (next_position > horizontal_motion_limits.max_pulses))
    {
      VisualPan_StopHardware();
      return;
    }

    HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin,
                      direction_sign > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);
    visual_pan_direction_sign = direction_sign;
    visual_pan_running = true;
    if ((HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK) ||
        (HAL_TIM_Base_Start_IT(&htim4) != HAL_OK))
    {
      VisualPan_StopHardware();
      Error_Handler();
    }
  }

}

static void MotorPulseLab_StartFixedHardware(MotorPulseLabDirection direction)
{
  MotorPulseLab_StartHardware(
      direction,
      (uint16_t)(16000000U /
                 ((uint32_t)MotorPulseLab_GetStepFrequencyHz(&motor_pulse_lab) *
                  10U) -
                 1U));

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

  if (PanView_Uart1Transmit(&huart1, message, message_size,
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
      (PanView_Uart1Transmit(&huart1, (uint8_t *)relative_position_message,
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

  if (PanView_Uart1Transmit(&huart1, message, message_size,
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
}

static const char *VisualStateText(VisualState state)
{
  switch (state)
  {
    case VISUAL_STATE_TRACKING: return "TRACKING";
    case VISUAL_STATE_LOCKED: return "LOCKED";
    case VISUAL_STATE_LOST: return "LOST";
    case VISUAL_STATE_FAULT: return "FAULT";
    case VISUAL_STATE_SEARCH:
    default: return "SEARCH";
  }
}

static bool VisualState_PlayHitAudio(uint32_t current_tick)
{
  char message[64];
  int length;

  if (visual_audio_played &&
      ((current_tick - visual_audio_last_tick) < VISUAL_AUDIO_COOLDOWN_MS))
  {
    return false;
  }

  if (!AudioPlayer_Play(PANVIEW_AUDIO_HIT))
  {
    return false;
  }

  visual_audio_last_tick = current_tick;
  visual_audio_played = true;
  length = snprintf(message, sizeof(message), "AUDIO effect=hit\\r\\n");
  if ((length > 0) && (length < (int)sizeof(message)))
  {
    (void)PanView_Uart1Transmit(&huart1, (uint8_t *)message, (uint16_t)length,
                            BOOT_LOG_TIMEOUT_MS);
  }
  return true;
}

static bool VisualState_AtLimitFault(void)
{
  int32_t pan_position = RelativePositionTracker_GetPositionPulses(
      &relative_position_tracker);

  if ((latest_pan_speed_target > 0) &&
      (pan_position >= horizontal_motion_limits.max_pulses))
  {
    return true;
  }
  if ((latest_pan_speed_target < 0) &&
      (pan_position <= horizontal_motion_limits.min_pulses))
  {
    return true;
  }
  if ((latest_pitch_speed_target > 0) &&
      (pitch_position_pulses >= (int32_t)PITCH_LIMIT_PULSES))
  {
    return true;
  }
  if ((latest_pitch_speed_target < 0) &&
      (pitch_position_pulses <= -(int32_t)PITCH_LIMIT_PULSES))
  {
    return true;
  }
  return false;
}

static void VisualState_Set(VisualState next_state, const char *reason)
{
  char message[96];
  int length;

  if (visual_state_initialized && (visual_state == next_state))
  {
    return;
  }

  visual_state = next_state;
  visual_state_initialized = true;
  if ((strcmp(reason, "boot") != 0) && (next_state == VISUAL_STATE_LOCKED))
  {
    (void)VisualState_PlayHitAudio(HAL_GetTick());
  }
  length = snprintf(message, sizeof(message), "VIS state=%s reason=%s\r\n",
                    VisualStateText(next_state), reason);
  if ((length > 0) && (length < (int)sizeof(message)))
  {
    (void)PanView_Uart1Transmit(&huart1, (uint8_t *)message, (uint16_t)length,
                            BOOT_LOG_TIMEOUT_MS);
  }
}

/* P10 状态机：通信先判 LOST，安全条件再判 FAULT，误差稳定 500 ms 才 LOCKED。 */
static void VisualState_Update(uint32_t current_tick)
{
  bool target_present = latest_k230_result_valid &&
                        latest_k230_result.target_present;
  bool communication_timed_out =
      (last_k230_valid_tick == 0U) ||
      ((current_tick - last_k230_valid_tick) > VISUAL_TARGET_TIMEOUT_MS);
  bool centered = target_present &&
                  (latest_k230_error.error_x <=
                       visual_track_config.deadzone_pixels) &&
                  (latest_k230_error.error_x >=
                       -visual_track_config.deadzone_pixels) &&
                  (latest_k230_error.error_y <=
                       visual_track_config.deadzone_pixels) &&
                  (latest_k230_error.error_y >=
                       -visual_track_config.deadzone_pixels);
  VisualState next_state;
  const char *reason;

  if (safety_fault_latched != PANVIEW_SAFETY_CLEAR)
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_FAULT;
    reason = PanView_SafetyFaultText(safety_fault_latched);
  }
  else if (!control_takeover_active || !visual_tracking_enabled)
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_SEARCH;
    reason = "waiting_for_takeover";
  }
  else if (communication_timed_out)
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_LOST;
    reason = "vision_timeout";
  }
  else if (!RelativePositionTracker_IsValid(&relative_position_tracker) ||
           !pitch_zeroed)
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_FAULT;
    reason = "position_not_zeroed";
  }
  else if (VisualState_AtLimitFault())
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_FAULT;
    reason = "software_limit";
  }
  else if (!target_present)
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_SEARCH;
    reason = "target_absent";
  }
  else if (!centered)
  {
    visual_lock_candidate_tick = 0U;
    next_state = VISUAL_STATE_TRACKING;
    reason = "error_outside_deadzone";
  }
  else
  {
    if (visual_lock_candidate_tick == 0U)
    {
      visual_lock_candidate_tick = current_tick;
    }
    if ((current_tick - visual_lock_candidate_tick) >= VISUAL_LOCK_HOLD_MS)
    {
      next_state = VISUAL_STATE_LOCKED;
      reason = "centered_for_500ms";
    }
    else
    {
      next_state = VISUAL_STATE_TRACKING;
      reason = "center_candidate";
    }
  }

  VisualState_Set(next_state, reason);

  if ((next_state == VISUAL_STATE_TRACKING) && target_present)
  {
    VisualPan_ApplySpeed(latest_pan_speed_target);
    VisualPitch_ApplySpeed(latest_pitch_speed_target);
  }
  else
  {
    VisualPan_StopHardware();
    VisualPitch_StopHardware();
    if (next_state == VISUAL_STATE_FAULT)
    {
      HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_RESET);
    }
  }
}

static void TouchUi_StopAllMotion(void)
{
  VisualPan_StopHardware();
  VisualPitch_StopHardware();
  if (dual_test_running)
  {
    Dual_StopHardware();
  }
  if (pitch_running)
  {
    Pitch_StopPulses();
  }
  if (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED)
  {
    MotorPulseLab_StopHardware();
  }
  RelativePositionTracker_Invalidate(&relative_position_tracker);
  pitch_zeroed = false;
  control_takeover_active = false;
  VisualTrackController_Reset(&pan_track_state);
  VisualTrackController_Reset(&pitch_track_state);
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_RESET);
}

static const char *PanView_SafetyFaultText(PanViewSafetyFault fault)
{
  switch (fault)
  {
    case PANVIEW_SAFETY_VISION_TIMEOUT: return "vision_timeout";
    case PANVIEW_SAFETY_LIMIT: return "software_limit";
    case PANVIEW_SAFETY_CONTROL_STALE: return "control_task_stale";
    case PANVIEW_SAFETY_CLEAR:
    default: return "clear";
  }
}

/*
 * 安全停机路径不打印日志、不等待队列，也不依赖 TelemetryTask。
 * 只做可重复的硬件停止、释放 EN 和清除位置有效性；下一次启动必须重新回中。
 */
static void PanView_SafetyStop(PanViewSafetyFault fault)
{
  if (safety_fault_latched != PANVIEW_SAFETY_CLEAR)
  {
    return;
  }

  safety_fault_latched = fault;
  VisualPan_StopHardware();
  VisualPitch_StopHardware();
  if (dual_test_running)
  {
    Dual_StopHardware();
  }
  if (pitch_running)
  {
    Pitch_StopPulses();
  }
  if (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED)
  {
    MotorPulseLab_StopHardware();
  }

  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_RESET);
  control_takeover_active = false;
  visual_tracking_enabled = false;
  pitch_zeroed = false;
  latest_pan_speed_target = 0;
  latest_pitch_speed_target = 0;
  RelativePositionTracker_Invalidate(&relative_position_tracker);
  VisualTrackController_Reset(&pan_track_state);
  VisualTrackController_Reset(&pitch_track_state);
  visual_lock_candidate_tick = 0U;
  visual_state = VISUAL_STATE_FAULT;
  visual_state_initialized = true;
}

void PanView_SafetyStep(uint32_t current_tick)
{
  if ((safety_fault_latched != PANVIEW_SAFETY_CLEAR) ||
      !control_takeover_active || !visual_tracking_enabled)
  {
    return;
  }

  if ((last_pan_view_step_tick == 0U) ||
      ((current_tick - last_pan_view_step_tick) >
       SAFETY_CONTROL_STALE_TIMEOUT_MS))
  {
    PanView_SafetyStop(PANVIEW_SAFETY_CONTROL_STALE);
  }
  else if ((last_k230_valid_tick == 0U) ||
           ((current_tick - last_k230_valid_tick) > VISUAL_TARGET_TIMEOUT_MS))
  {
    PanView_SafetyStop(PANVIEW_SAFETY_VISION_TIMEOUT);
  }
  else if (VisualState_AtLimitFault())
  {
    PanView_SafetyStop(PANVIEW_SAFETY_LIMIT);
  }
}

void PanView_InputStep(uint32_t current_tick)
{
    /* P10：只验证 FT6336G 原始触摸点，状态或坐标变化时输出一条日志。 */
    if (touch_ready && PeriodicTask_IsDue(&touch_poll_task, current_tick) &&
        FT6336G_ReadPoint(&touch_point))
    {
      bool touch_changed = (touch_point.touch_count != touch_last_count) ||
                           ((touch_point.touch_count > 0U) &&
                            ((touch_point.x != touch_last_x) ||
                             (touch_point.y != touch_last_y)));
      if (touch_changed)
      {
        /* 当前显示旋转 180 度，触摸原始 X/Y 也需要分别反向。 */
        uint16_t touch_screen_x =
            (touch_point.x > TOUCH_SCREEN_MAX_X)
                ? 0U
                : (uint16_t)(TOUCH_SCREEN_MAX_X - touch_point.x);
        uint16_t touch_screen_y =
            (touch_point.y > TOUCH_SCREEN_MAX_Y)
                ? 0U
                : (uint16_t)(TOUCH_SCREEN_MAX_Y - touch_point.y);
        ui_touch_raw_x = touch_point.x;
        ui_touch_raw_y = touch_point.y;
        ui_touch_screen_x = touch_screen_x;
        ui_touch_screen_y = touch_screen_y;
        ui_touch_count = touch_point.touch_count;
        const char *touch_zone;
        if (touch_point.touch_count == 0U)
        {
          touch_zone = "release";
        }
        else if (touch_point.touch_count > 1U)
        {
          touch_zone = "multi";
        }
        else if ((touch_screen_x < TOUCH_SCREEN_MID_X) &&
                 (touch_screen_y < TOUCH_SCREEN_MID_Y))
        {
          touch_zone = "top_left";
        }
        else if ((touch_screen_x >= TOUCH_SCREEN_MID_X) &&
                 (touch_screen_y < TOUCH_SCREEN_MID_Y))
        {
          touch_zone = "top_right";
        }
        else if (touch_screen_x < TOUCH_SCREEN_MID_X)
        {
          touch_zone = "bottom_left";
        }
        else
        {
          touch_zone = "bottom_right";
        }
        int touch_runtime_length = snprintf(
            touch_runtime_message, sizeof(touch_runtime_message),
            "TOUCH point count=%u raw=(%u,%u) screen=(%u,%u) zone=%s\r\n",
            (unsigned int)touch_point.touch_count,
            (unsigned int)touch_point.x, (unsigned int)touch_point.y,
            (unsigned int)touch_screen_x, (unsigned int)touch_screen_y,
            touch_zone);
        if ((touch_runtime_length > 0) &&
            (touch_runtime_length < (int)sizeof(touch_runtime_message)))
        {
          (void)PanView_Uart1Transmit(&huart1, (uint8_t *)touch_runtime_message,
                                  (uint16_t)touch_runtime_length,
                                  BOOT_LOG_TIMEOUT_MS);
        }
        if ((touch_point.touch_count == 1U) &&
            (touch_last_count == 0U))
        {
          PanView_RtosPublishTouch(touch_screen_x, touch_screen_y);
        }
        touch_last_count = touch_point.touch_count;
        touch_last_x = touch_point.x;
        touch_last_y = touch_point.y;
      }
    }


}

void PanView_AppStep(void)
{
    uint32_t current_tick = HAL_GetTick();
    last_pan_view_step_tick = current_tick;
    PanView_RtosProcessUiEvents();

    /* P10：处理 K230 有效结果，同时生成水平和俯仰速度目标。 */
    if (k230_uart_rx_frame.pending)
    {
      k230_uart_last_frame_size = k230_uart_rx_frame.size;
      if (VisionTextResult_Parse(k230_uart_rx_frame.data,
                                 k230_uart_rx_frame.size,
                                 &latest_k230_result))
      {
        latest_k230_result_valid = true;
        latest_k230_error = VisionError_FromTarget(
            1920U, 1080U, latest_k230_result.target_present,
            latest_k230_result.center_x, latest_k230_result.center_y);
        latest_pan_speed_target = VisualTrackController_Update(
            &visual_track_config, &pan_track_state,
            latest_k230_error.target_present, latest_k230_error.error_x,
            current_tick);
        latest_pitch_speed_target = VisualTrackController_Update(
            &visual_track_config, &pitch_track_state,
            latest_k230_error.target_present, latest_k230_error.error_y,
            current_tick);
        {
          VisionTargetMessage vision_message = {
              (uint8_t)(latest_k230_error.target_present ? 1U : 0U),
              latest_k230_error.error_x,
              latest_k230_error.error_y,
              latest_k230_result.center_x,
              latest_k230_result.center_y,
              current_tick};
          PanView_RtosPublishVision(&vision_message);
        }
        last_k230_valid_tick = current_tick;
        k230_text_valid_count++;
      }
      else
      {
        k230_text_error_count++;
      }
      UartRxFrame_Release(&k230_uart_rx_frame);
    }

    if ((last_k230_valid_tick == 0U) ||
        ((current_tick - last_k230_valid_tick) > VISUAL_TARGET_TIMEOUT_MS))
    {
      VisualPan_StopHardware();
      VisualPitch_StopHardware();
      VisualTrackController_Reset(&pan_track_state);
      VisualTrackController_Reset(&pitch_track_state);
      latest_pan_speed_target = 0;
      latest_pitch_speed_target = 0;
    }

    VisualState_Update(current_tick);




    if (motor_profile_move_complete_pending)
    {
      motor_profile_move_complete_pending = false;
      if (PanView_Uart1Transmit(&huart1, motor_profile_move_complete_message,
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

      /* 运动完成后在主循环读取一次真实反馈，和理论脉冲坐标对照。 */
      MotorTtlReadPositionAndError();
      MotorTtlReadSystemStatus();
    }

    /* 运动期间周期读取系统状态；串口读取在主循环中执行，不放入脉冲中断。 */
    if (PeriodicTask_IsDue(&motor_feedback_task, current_tick) &&
        MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED)
    {
      MotorTtlReadSystemStatus();
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
        VisualPan_StopHardware();
        HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);

        if (PanView_Uart1Transmit(&huart1, key0_pressed_message,
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

        if (PanView_Uart1Transmit(&huart1, key1_pressed_message,
                              sizeof(key1_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }

        if (visual_pan_running)
        {
          direction_message = motor_direction_ignored_message;
          direction_message_size = sizeof(motor_direction_ignored_message) - 1U;
        }
        else if (MotorPulseLab_ToggleDirection(&motor_pulse_lab))
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

        if (PanView_Uart1Transmit(&huart1, direction_message, direction_message_size,
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

        if (PanView_Uart1Transmit(&huart1, key2_pressed_message,
                              sizeof(key2_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }

        if (visual_pan_running ||
            (MotorPulseLab_GetState(&motor_pulse_lab) != MOTOR_PULSE_LAB_STOPPED))
        {
          if (PanView_Uart1Transmit(&huart1, motor_profile_move_ignored_message,
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
            if (PanView_Uart1Transmit(&huart1, fixed_move_message,
                                  fixed_move_message_size,
                                  BOOT_LOG_TIMEOUT_MS) != HAL_OK)
            {
              Error_Handler();
            }
          }
          else
          {
            if (PanView_Uart1Transmit(&huart1, motor_profile_move_ignored_message,
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
        if (PanView_Uart1Transmit(&huart1, key_up_pressed_message,
                              sizeof(key_up_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }

        if ((MotorPulseLab_GetState(&motor_pulse_lab) == MOTOR_PULSE_LAB_STOPPED) &&
            !pitch_running && !dual_test_running && !visual_pan_running &&
            !visual_pitch_running)
        {
          ExecuteManualZeroStart();
        }
        else if (PanView_Uart1Transmit(&huart1, relative_position_zero_ignored_message,
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
      else if ((command == UART_TEXT_COMMAND_PITCH_UP) ||
               (command == UART_TEXT_COMMAND_PITCH_DOWN))
      {
        if (!pitch_zeroed)
        {
          response = pitch_not_zeroed_message;
          response_size = sizeof(pitch_not_zeroed_message) - 1U;
        }
        else
        {
          /* 已实测：DIR=Low 为俯仰向上，DIR=High 为俯仰向下。 */
          GPIO_PinState direction = command == UART_TEXT_COMMAND_PITCH_UP
                                         ? GPIO_PIN_RESET
                                         : GPIO_PIN_SET;
          int32_t delta_pulses = command == UART_TEXT_COMMAND_PITCH_UP
                                     ? (int32_t)PITCH_TEST_PULSES
                                     : -(int32_t)PITCH_TEST_PULSES;
          int32_t target_pulses = pitch_position_pulses + delta_pulses;

          if ((target_pulses > (int32_t)PITCH_LIMIT_PULSES) ||
              (target_pulses < -(int32_t)PITCH_LIMIT_PULSES))
          {
            response = pitch_limit_message;
            response_size = sizeof(pitch_limit_message) - 1U;
          }
          else if (Pitch_StartTestMove(direction, delta_pulses))
          {
            response = command == UART_TEXT_COMMAND_PITCH_UP
                           ? pitch_up_message
                           : pitch_down_message;
            response_size = command == UART_TEXT_COMMAND_PITCH_UP
                                ? sizeof(pitch_up_message) - 1U
                                : sizeof(pitch_down_message) - 1U;
          }
          else
          {
            response = pitch_busy_message;
            response_size = sizeof(pitch_busy_message) - 1U;
          }
        }
      }
      else if (command == UART_TEXT_COMMAND_PITCH_STOP)
      {
        bool interrupted = pitch_running || dual_test_running ||
                           visual_pitch_running;
        if (dual_test_running)
        {
          Dual_StopHardware();
        }
        else if (visual_pitch_running)
        {
          VisualPitch_StopHardware();
        }
        else
        {
          Pitch_StopPulses();
        }
        pitch_active_delta_pulses = 0;
        if (interrupted)
        {
          pitch_zeroed = false;
          RelativePositionTracker_Invalidate(&relative_position_tracker);
          response = pitch_rezero_message;
          response_size = sizeof(pitch_rezero_message) - 1U;
        }
        else
        {
          response = pitch_stop_message;
          response_size = sizeof(pitch_stop_message) - 1U;
        }
      }
      else if ((command == UART_TEXT_COMMAND_DUAL_TEST) ||
               (command == UART_TEXT_COMMAND_DUAL_TEST_NEG))
      {
        int32_t direction_sign = command == UART_TEXT_COMMAND_DUAL_TEST
                                     ? 1
                                     : -1;
        if (Dual_StartTestMove(direction_sign))
        {
          response = command == UART_TEXT_COMMAND_DUAL_TEST
                         ? dual_test_message
                         : dual_test_neg_message;
          response_size = command == UART_TEXT_COMMAND_DUAL_TEST
                              ? sizeof(dual_test_message) - 1U
                              : sizeof(dual_test_neg_message) - 1U;
        }
        else
        {
          response = dual_test_rejected_message;
          response_size = sizeof(dual_test_rejected_message) - 1U;
        }
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
          (PanView_Uart1Transmit(&huart1, response, response_size,
                             UART_RX_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }

      UartRxFrame_Release(&uart_rx_frame);
    }

}

void PanView_UiRefreshStep(uint32_t current_tick)
{
  PanViewUiData ui_data = MainUiData();
  bool ui_state_changed = ui_trace_last_state != (int32_t)ui_data.state;

  if (ui_state_changed)
  {
    static const uint8_t ui_refresh_begin[] = "UI refresh begin\\r\\n";
    (void)PanView_Uart1Transmit(&huart1, (uint8_t *)ui_refresh_begin,
                                sizeof(ui_refresh_begin) - 1U,
                                UART_RX_LOG_TIMEOUT_MS);
  }

  PanViewUi_Refresh(&ui_data, current_tick);

  if (ui_state_changed)
  {
    static const uint8_t ui_refresh_end[] = "UI refresh end\\r\\n";
    (void)PanView_Uart1Transmit(&huart1, (uint8_t *)ui_refresh_end,
                                sizeof(ui_refresh_end) - 1U,
                                UART_RX_LOG_TIMEOUT_MS);
    ui_trace_last_state = (int32_t)ui_data.state;
  }
}

void PanView_TelemetryStep(uint32_t current_tick)
{    if (PeriodicTask_IsDue(&k230_uart_log_task, current_tick))
    {
      int k230_log_length = snprintf(
          k230_uart_log_message, sizeof(k230_uart_log_message),
          "K230 state=%s safety=%s RX frames=%lu drop=%lu text_ok=%lu text_err=%lu last_size=%u target=%u count=%u cx=%d cy=%d err_x=%d err_y=%d pan_speed=%ld pitch_speed=%ld pan_run=%u pitch_run=%u pan_pos=%ld pitch_pos=%ld\r\n",
          VisualStateText(visual_state),
        PanView_SafetyFaultText(safety_fault_latched),
          (unsigned long)k230_uart_rx_frame.received_count,
          (unsigned long)k230_uart_rx_frame.dropped_count,
          (unsigned long)k230_text_valid_count,
          (unsigned long)k230_text_error_count,
          (unsigned int)k230_uart_last_frame_size,
          (unsigned int)(latest_k230_result_valid &&
                                 latest_k230_result.target_present),
          (unsigned int)(latest_k230_result_valid
                             ? latest_k230_result.target_count
                             : 0U),
          latest_k230_result_valid ? latest_k230_result.center_x : 0,
          latest_k230_result_valid ? latest_k230_result.center_y : 0,
          latest_k230_result_valid ? latest_k230_error.error_x : 0,
          latest_k230_result_valid ? latest_k230_error.error_y : 0,
          (long)(latest_k230_result_valid ? latest_pan_speed_target : 0),
          (long)(latest_k230_result_valid ? latest_pitch_speed_target : 0),
          (unsigned int)visual_pan_running,
          (unsigned int)visual_pitch_running,
          (long)RelativePositionTracker_GetPositionPulses(
              &relative_position_tracker),
          (long)pitch_position_pulses);

      if ((k230_log_length <= 0) ||
          (k230_log_length >= (int)sizeof(k230_uart_log_message)) ||
          (PanView_Uart1Transmit(&huart1, (uint8_t *)k230_uart_log_message,
                             (uint16_t)k230_log_length,
                             BOOT_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }
    }    /* 5 s：输出心跳、实际间隔和其他任务计数，作为非阻塞调度证据。 */
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
          (PanView_Uart1Transmit(&huart1,
                             (uint8_t *)heartbeat_message,
                             (uint16_t)heartbeat_length,
                             BOOT_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (dual_test_running && ((htim == &htim3) || (htim == &htim4)))
  {
    /* 以 TIM3 更新事件作为双轴联动的公共脉冲计数时基。 */
    if (htim == &htim3)
    {
      if (dual_test_remaining_pulses > 0U)
      {
        dual_test_remaining_pulses--;
      }

      if (dual_test_remaining_pulses == 0U)
      {
        (void)RelativePositionTracker_ApplyCompletedPulseDelta(
            &relative_position_tracker,
            dual_test_direction_sign * (int32_t)DUAL_TEST_PULSES);
        pitch_position_pulses +=
            dual_test_direction_sign * (int32_t)DUAL_TEST_PULSES;
        Dual_StopHardware();
      }
    }
    return;
  }

  if (htim == &htim3)
  {
    if (pitch_running && (pitch_remaining_pulses > 0U))
    {
      pitch_remaining_pulses--;
      if (pitch_remaining_pulses == 0U)
      {
        pitch_position_pulses += pitch_active_delta_pulses;
        pitch_active_delta_pulses = 0;
        Pitch_StopPulses();
      }
    }
    else if (visual_pitch_running)
    {
      int32_t next_position = pitch_position_pulses +
                              visual_pitch_direction_sign;

      /* 先判断下一脉冲是否越界，再提交一个已完成脉冲。 */
      if ((next_position < -(int32_t)PITCH_LIMIT_PULSES) ||
          (next_position > (int32_t)PITCH_LIMIT_PULSES))
      {
        VisualPitch_StopHardware();
      }
      else
      {
        pitch_position_pulses = next_position;
        if ((next_position == -(int32_t)PITCH_LIMIT_PULSES) ||
            (next_position == (int32_t)PITCH_LIMIT_PULSES))
        {
          VisualPitch_StopHardware();
        }
      }
    }
    return;
  }

  if (htim == &htim4)
  {
    if ((MotorPulseLab_GetState(&motor_pulse_lab) ==
         MOTOR_PULSE_LAB_FIXED_MOVE) &&
        MotorPulseLab_OnPulsePeriod(&motor_pulse_lab))
    {
      if (HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1) != HAL_OK)
      {
        Error_Handler();
      }

      if (HAL_TIM_Base_Stop_IT(&htim4) != HAL_OK)
      {
        Error_Handler();
      }

      HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin,
                        control_takeover_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
      motor_profile_move_complete_pending = true;
    }
    else if (MotorPulseLab_GetState(&motor_pulse_lab) ==
             MOTOR_PULSE_LAB_FIXED_MOVE)
    {
      /* 新分频在下一次 TIM4 更新事件生效，脉冲数量不受影响。 */
      MotorPulseLab_SetTimerFrequency(
          MotorPulseLab_GetStepFrequencyHz(&motor_pulse_lab));
    }
    else if (visual_pan_running)
    {
      int32_t position = RelativePositionTracker_GetPositionPulses(
          &relative_position_tracker);
      int32_t next_position = position + visual_pan_direction_sign;

      /* 先判断下一脉冲是否越界，再提交一个已完成脉冲。 */
      if ((next_position < horizontal_motion_limits.min_pulses) ||
          (next_position > horizontal_motion_limits.max_pulses) ||
          !RelativePositionTracker_ApplyCompletedPulseDelta(
              &relative_position_tracker, visual_pan_direction_sign))
      {
        VisualPan_StopHardware();
      }
      else if ((next_position == horizontal_motion_limits.min_pulses) ||
               (next_position == horizontal_motion_limits.max_pulses))
      {
        VisualPan_StopHardware();
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
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();
  /* IWDG 由 SafetyTask 在确认任务创建成功后启动；不能在调度器启动前
   * 提前开启，否则任务创建失败时无人刷新看门狗，会形成复位死循环。 */
  /* USER CODE BEGIN 2 */
  AudioPlayer_Init();
  {
    const char *audio_probe = "AUDIO ES8388 probe=failed\\r\\n";
    if (Es8388_Probe() && Es8388_InitPlayback())
    {
      audio_probe = "AUDIO ES8388 init=ok\\r\\n";
    }
    (void)PanView_Uart1Transmit(&huart1, (uint8_t *)audio_probe,
                            (uint16_t)strlen(audio_probe),
                            BOOT_LOG_TIMEOUT_MS);
  }
  /* 这些运行时对象由 PanView_AppStep 共用，先在初始化阶段清零。 */
  touch_last_count = 0U;
  touch_last_x = 0U;
  touch_last_y = 0U;
  last_heartbeat_log_ms = HAL_GetTick();
  key_scan_count = 0U;
  status_task_count = 0U;
  heartbeat_count = 0U;
  last_pan_view_step_tick = HAL_GetTick();
  safety_fault_latched = PANVIEW_SAFETY_CLEAR;

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
  PeriodicTask_Init(&motor_feedback_task, MOTOR_FEEDBACK_PERIOD_MS,
                    HAL_GetTick());
  PeriodicTask_Init(&heartbeat_task, HEARTBEAT_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&k230_uart_log_task, K230_UART_LOG_PERIOD_MS,
                    HAL_GetTick());
  PeriodicTask_Init(&touch_poll_task, TOUCH_POLL_PERIOD_MS, HAL_GetTick());
  UartRxFrame_Init(&uart_rx_frame);
  UartRxFrame_Init(&k230_uart_rx_frame);
  UartTextLineAccumulator_Init(&k230_uart_line_accumulator);
  VisionFrameParser_Init(&vision_frame_parser);
  CommunicationWatchdog_Init(&communication_watchdog, COMMUNICATION_TIMEOUT_MS);
  FrameSequenceTracker_Init(&frame_sequence_tracker);
  MotorPulseLab_Init(&motor_pulse_lab);
  horizontal_motion_limits = MotionLimits_HorizontalDefault();
  RelativePositionTracker_Init(&relative_position_tracker);
  visual_pan_running = false;
  visual_pan_direction_sign = 0;
  visual_pitch_running = false;
  visual_pitch_direction_sign = 0;
  last_k230_valid_tick = 0U;
  MotorPulseLab_StopHardware();
  Pitch_DisableHardware();
  if (MotorTtlProbe())
  {
    MotorTtlReadOption();
    MotorTtlReadPositionAndError();
    MotorTtlReadSystemStatus();
  }
  StartUartRxDma();
  StartK230UartRxDma();
  if (ILI9341_Init())
  {
    tft_ready = true;
    uint8_t tft_id[4] = {0U};
    char tft_id_message[64];

    {
      uint8_t touch_chip_id = 0U;
      uint8_t touch_chip_high = 0U;
      uint8_t touch_vendor = 0U;
      uint8_t touch_count = 0U;
      char touch_message[96];
      touch_ready = FT6336G_Init();
      bool touch_id_ok = touch_ready && FT6336G_ReadChipId(&touch_chip_id);
      bool touch_diag_ok = touch_id_ok &&
                           FT6336G_ReadRegister(0xA3U, &touch_chip_high) &&
                           FT6336G_ReadRegister(0xA8U, &touch_vendor) &&
                           FT6336G_ReadRegister(0x02U, &touch_count);
      int touch_length = snprintf(
          touch_message, sizeof(touch_message),
          "TOUCH probe=%s id_low=%02X id_high=%02X vendor=%02X points=%u\r\n",
          touch_diag_ok ? "ok" : (touch_ready ? "id_read_failed" : "failed"),
          touch_chip_id, touch_chip_high, touch_vendor,
          (unsigned int)touch_count);
      if ((touch_length > 0) &&
          (touch_length < (int)sizeof(touch_message)))
      {
        (void)PanView_Uart1Transmit(&huart1, (uint8_t *)touch_message,
                                (uint16_t)touch_length,
                                BOOT_LOG_TIMEOUT_MS);
      }
    }

    if (ILI9341_ReadId(tft_id))
    {
      int tft_id_length = snprintf(tft_id_message, sizeof(tft_id_message),
                                   "TFT ID=%02X %02X %02X %02X\r\n",
                                   tft_id[0], tft_id[1], tft_id[2], tft_id[3]);
      if ((tft_id_length > 0) &&
          (tft_id_length < (int)sizeof(tft_id_message)))
      {
        (void)PanView_Uart1Transmit(&huart1, (uint8_t *)tft_id_message,
                                (uint16_t)tft_id_length, BOOT_LOG_TIMEOUT_MS);
      }
    }

    PanViewUi_Init(UI_LANGUAGE_ZH);
  }
  /* 所有外设初始化完成后发送启动日志；失败则进入统一错误处理。 */
  if (PanView_Uart1Transmit(&huart1, boot_message,
                        sizeof(boot_message) - 1U,
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
  VisualState_Set(VISUAL_STATE_SEARCH, "boot");
  {
    PanViewUiData initial_ui_data = MainUiData();
    PanViewUi_Refresh(&initial_ui_data, HAL_GetTick());
  }
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

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

  /** Macro to configure the PLL multiplication factor
  */
  __HAL_RCC_PLL_PLLM_CONFIG(16);

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
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
