#ifndef PANVIEW_STEPPER_H
#define PANVIEW_STEPPER_H

#include <stdint.h>

#include "panview_messages.h"
#include "stm32f4xx_hal.h"

typedef struct
{
  int32_t pan_position_steps;
  int32_t pitch_position_steps;
  /* 当前真正交给定时器执行的渐变后速度，单位：step/s。 */
  int32_t pan_applied_speed_steps_per_second;
  int32_t pitch_applied_speed_steps_per_second;
  uint8_t pan_running;
  uint8_t pitch_running;
  /* 1 表示已经确认软件零点并开始跟踪，0 表示暂停且电机失能。 */
  uint8_t system_running;
} PanViewStepperStatus;

/* 初始化步进执行模块的内部状态。 */
void PanView_Stepper_Init(void);

/* KEY_UP 动作：开始时确认双轴零点，运行中再次调用则暂停并失能。 */
void PanView_Stepper_ToggleSystemRun(void);

/* 安全任务专用：立即停止双轴脉冲并释放 EN，保持系统暂停。 */
void PanView_Stepper_SafetyStop(void);

/* 执行一份运动命令；模块内部负责限位、PWM、方向和使能。 */
void PanView_Stepper_Execute(const MotionCommand *command);

/* 读取只读状态，用于串口日志和调试观察。 */
void PanView_Stepper_GetStatus(PanViewStepperStatus *status);

/* 定时器更新中断入口：每个 STEP 周期累计一次理论位置。 */
void PanView_Stepper_OnTimerElapsed(TIM_HandleTypeDef *htim);

#endif /* PANVIEW_STEPPER_H */
