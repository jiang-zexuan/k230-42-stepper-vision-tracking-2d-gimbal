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

/* 输出是抽象的带符号速度目标，轴向正负由各轴实测方向约定解释。 */
int32_t VisualTrackController_HorizontalSpeed(
    const VisualTrackControllerConfig *config, bool target_present,
    int16_t error_x);

/* 俯仰轴正方向约定为向上；图像 Y 向下为正，因此这里需要反向映射。 */
int32_t VisualTrackController_VerticalSpeed(
    const VisualTrackControllerConfig *config, bool target_present,
    int16_t error_y);

#endif
