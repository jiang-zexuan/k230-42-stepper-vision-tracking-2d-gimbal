#ifndef PANVIEW_MOTION_H
#define PANVIEW_MOTION_H

#include <stdint.h>

#include "panview_messages.h"

/*
 * 根据一帧视觉结果计算目标中心相对画面中心的像素误差。
 * 正值表示目标中心在画面中心的右侧/下方，负值表示在左侧/上方。
 */
void PanView_Motion_CalculateError(const VisionResult *vision,
                                   int16_t *error_x_px,
                                   int16_t *error_y_px);

/* T06-2 的固定参数：误差单位 px，速度单位 step/s。 */
#define PANVIEW_MOTION_DEAD_ZONE_PX 40
#define PANVIEW_MOTION_GAIN_STEPS_PER_PIXEL 2
#define PANVIEW_MOTION_MAX_SPEED_STEPS_PER_SECOND 800

/* 每个 10 ms 运动周期允许速度改变的最大值，单位 step/s。 */
#define PANVIEW_MOTION_SLEW_STEP_STEPS_PER_SECOND 40

/* 锁定滞回阈值，单位 px：小阈值进入锁定，大阈值退出锁定。 */
#define PANVIEW_MOTION_LOCK_ENTER_PX 40
#define PANVIEW_MOTION_LOCK_EXIT_PX 60

/* 根据视觉结果生成影子运动命令；不会操作任何电机硬件。 */
void PanView_Motion_CreateCommand(const VisionResult *vision,
                                 uint32_t generated_tick_ms,
                                 MotionCommand *command);

/*
 * 让输出速度逐步接近目标速度，避免速度突然跳变。
 * target_command 是本轮想达到的速度，previous_command 是上一轮实际输出。
 */
void PanView_Motion_ApplySlewRate(const MotionCommand *target_command,
                                  const MotionCommand *previous_command,
                                  MotionCommand *limited_command);

/* 根据当前误差和上一轮锁定状态，计算本轮是否保持目标锁定。 */
uint8_t PanView_Motion_UpdateLock(uint8_t target_present,
                                  int16_t error_x_px,
                                  int16_t error_y_px,
                                  uint8_t previous_locked);

#endif /* PANVIEW_MOTION_H */
