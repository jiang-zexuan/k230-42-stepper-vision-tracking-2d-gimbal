#ifndef VISUAL_TRACK_CONTROLLER_H
#define VISUAL_TRACK_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  int16_t deadzone_pixels;
  int16_t gain_pulses_per_pixel;
  int32_t max_speed_pulses_per_second;
} VisualTrackControllerConfig;

/* 输出是抽象的带符号速度目标，正负与电机 DIR 电平的对应关系待实测。 */
int32_t VisualTrackController_HorizontalSpeed(
    const VisualTrackControllerConfig *config, bool target_present,
    int16_t error_x);

#endif
