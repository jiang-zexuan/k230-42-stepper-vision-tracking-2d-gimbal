/*
 * 文件用途：定义 PanView 任务之间共享的消息类型。
 *
 * 本文件只描述数据和单位，不负责串口接收、消息队列或控制逻辑。
 */
#ifndef PANVIEW_MESSAGES_H
#define PANVIEW_MESSAGES_H

#include <stdint.h>

/*
 * K230 当前 PV04 文本帧中的视觉结果。
 *
 * 坐标单位统一为源图像像素（px）。当前实测日志对应的源图像为
 * 1920x1080，但宽高不是 PV04 文本帧中的字段，由接收侧配置填写。
 */
typedef struct
{
  /* K230 ticks_ms() 产生的时间戳，单位：ms。 */
  uint32_t source_timestamp_ms;

  /* 当前帧检测到的目标数量；无目标帧为 0。 */
  uint16_t target_count;

  /* 目标状态：0 表示无可用目标，1 表示存在可用目标。 */
  uint8_t target_present;

  /*
   * 视觉源图像尺寸，单位：px。
   * 这两个字段不是当前 PV04 文本直接发送的字段，
   * 由 VisionRxTask 按当前 K230 pipeline 配置填写。
   */
  uint16_t frame_width_px;
  uint16_t frame_height_px;

  /* 主目标框左上角坐标，单位：px。 */
  int16_t target_x_px;
  int16_t target_y_px;

  /* 主目标框尺寸，单位：px。 */
  uint16_t target_width_px;
  uint16_t target_height_px;

  /* 主目标中心坐标，单位：px。 */
  float center_x_px;
  float center_y_px;

  /*
   * 发布器附带的诊断字段。
   * 当前 p402.txt 中 confidence=0.0000、fps=0.00，数值意义仍待验证。
   */
  float confidence;
  float source_fps;

  /* STM32 收到并完成一帧解析时的本地 RTOS/HAL 节拍，单位：ms。 */
  uint32_t received_tick_ms;
} VisionResult;

/*
 * MotionTask 输出给 StepperTask 的双轴运动命令。
 * MotionTask 负责生成命令；StepperTask 是唯一允许执行电机动作的任务。
 */
typedef struct
{
  /* 水平轴目标速度，单位：step/s；正负号表示方向。 */
  int32_t pan_speed_steps_per_second;

  /* 俯仰轴目标速度，单位：step/s；正负号表示方向。 */
  int32_t pitch_speed_steps_per_second;

  /* 命令生成时的 STM32 本地节拍，单位：ms。 */
  uint32_t generated_tick_ms;

  /* 0 表示命令不可执行；1 表示命令内容有效。 */
  uint8_t valid;
} MotionCommand;

/* InputTask 发布给 AppControlTask 的输入请求类型。 */
typedef enum
{
  PANVIEW_INPUT_EVENT_START_REQUEST = 0,
  PANVIEW_INPUT_EVENT_STOP_REQUEST
} PanViewInputEventType;

/*
 * InputTask 发布的输入事件。
 * 输入任务只记录操作者请求；系统状态是否切换由 AppControlTask 决定。
 */
typedef struct
{
  PanViewInputEventType type;

  /* 事件产生时的 STM32 本地节拍，单位：ms。 */
  uint32_t occurred_tick_ms;
} InputEvent;

/* AppControlTask 发布给 UiTask/AudioTask 的提示事件类型。 */
typedef enum
{
  PANVIEW_INDICATOR_EVENT_NONE = 0,
  PANVIEW_INDICATOR_EVENT_TRACKING_STARTED,
  PANVIEW_INDICATOR_EVENT_TRACKING_STOPPED,
  PANVIEW_INDICATOR_EVENT_TARGET_LOCKED,
  PANVIEW_INDICATOR_EVENT_TARGET_LOST
} PanViewIndicatorEventType;

/*
 * 提示事件只描述“发生了什么”，不直接操作 LED、屏幕或音频硬件。
 */
typedef struct
{
  PanViewIndicatorEventType type;

  /* 事件产生时的 STM32 本地节拍，单位：ms。 */
  uint32_t occurred_tick_ms;
} IndicatorEvent;

/*
 * 对外展示的视觉状态。状态转换由 AppControlTask 负责，
 * UiTask 和 TelemetryTask 只读取快照中的结果。
 */
typedef enum
{
  PANVIEW_VISUAL_STATE_SEARCH = 0,
  PANVIEW_VISUAL_STATE_TRACKING,
  PANVIEW_VISUAL_STATE_LOCKED,
  PANVIEW_VISUAL_STATE_LOST,
  PANVIEW_VISUAL_STATE_FAULT
} PanViewVisualState;

/*
 * PanView 对外提供的只读运行状态快照。
 * 该结构体不包含硬件句柄；读取它不会直接触发电机、屏幕或音频动作。
 */
typedef struct
{
  PanViewVisualState visual_state;

  /* 视觉跟踪总开关：0 表示关闭，1 表示开启。 */
  uint8_t visual_tracking_enabled;

  /* 最近一次视觉结果中的目标状态：0 表示无目标，1 表示有目标。 */
  uint8_t target_present;

  /* 目标相对画面中心的误差，单位：px；由 MotionTask 计算。 */
  int16_t error_x_px;
  int16_t error_y_px;

  /* 当前运动命令的双轴速度，单位：step/s。 */
  int32_t pan_speed_steps_per_second;
  int32_t pitch_speed_steps_per_second;

  /* 最近一次有效视觉结果到达的本地节拍，单位：ms。 */
  uint32_t last_vision_received_tick_ms;

  /* 生成本快照时的本地节拍，单位：ms。 */
  uint32_t snapshot_tick_ms;
} PanViewStateSnapshot;

#endif /* PANVIEW_MESSAGES_H */
