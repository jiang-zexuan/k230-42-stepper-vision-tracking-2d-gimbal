#ifndef VISUAL_TRACK_CONTROLLER_H
#define VISUAL_TRACK_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  int16_t deadzone_pixels;
  float kp;
  float ki;
  float kd;
  float integral_limit;
  int32_t max_speed_pulses_per_second;
  int32_t max_acceleration_pulses_per_second2;
} VisualTrackControllerConfig;

typedef struct
{
  float integral;
  float derivative;
  int16_t previous_error;
  uint32_t previous_tick_ms;
  int32_t previous_output;
  bool previous_valid;
} VisualTrackControllerState;

void VisualTrackController_Reset(VisualTrackControllerState *state);

/*
 * 根据图像误差计算带符号的速度目标。输出方向沿用现有装配约定：
 * 图像误差为正时输出负速度，图像误差为负时输出正速度。
 */
int32_t VisualTrackController_Update(
    const VisualTrackControllerConfig *config,
    VisualTrackControllerState *state, bool target_present, int16_t error,
    uint32_t tick_ms);

#endif
