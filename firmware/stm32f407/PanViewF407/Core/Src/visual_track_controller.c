#include "visual_track_controller.h"

static int32_t absolute_value(int16_t value)
{
  return value < 0 ? -(int32_t)value : (int32_t)value;
}

int32_t VisualTrackController_HorizontalSpeed(
    const VisualTrackControllerConfig *config, bool target_present,
    int16_t error_x)
{
  int32_t magnitude;
  int32_t speed;

  if ((config == 0) || (!target_present) ||
      (config->deadzone_pixels < 0) ||
      (config->gain_pulses_per_pixel < 0) ||
      (config->max_speed_pulses_per_second <= 0) ||
      (absolute_value(error_x) <= config->deadzone_pixels))
  {
    return 0;
  }

  magnitude = absolute_value(error_x) - config->deadzone_pixels;
  speed = magnitude * config->gain_pulses_per_pixel;
  if (speed > config->max_speed_pulses_per_second)
  {
    speed = config->max_speed_pulses_per_second;
  }
  /* 当前装配实测：目标在左侧时水平轴取正方向；图像 X 正方向向右，故取反。 */
  return error_x < 0 ? speed : -speed;
}
