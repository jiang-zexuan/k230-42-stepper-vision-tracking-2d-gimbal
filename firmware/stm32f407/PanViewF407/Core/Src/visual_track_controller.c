#include "visual_track_controller.h"

static int32_t absolute_value(int16_t value)
{
  return value < 0 ? -(int32_t)value : (int32_t)value;
}

static int32_t speed_from_error(const VisualTrackControllerConfig *config,
                                bool target_present, int16_t error)
{
  int32_t magnitude;
  int32_t speed;

  if ((config == 0) || (!target_present) ||
      (config->deadzone_pixels < 0) ||
      (config->gain_pulses_per_pixel < 0) ||
      (config->max_speed_pulses_per_second <= 0) ||
      (absolute_value(error) <= config->deadzone_pixels))
  {
    return 0;
  }

  magnitude = absolute_value(error) - config->deadzone_pixels;
  speed = magnitude * config->gain_pulses_per_pixel;
  if (speed > config->max_speed_pulses_per_second)
  {
    speed = config->max_speed_pulses_per_second;
  }

  return error < 0 ? speed : -speed;
}

int32_t VisualTrackController_HorizontalSpeed(
    const VisualTrackControllerConfig *config, bool target_present,
    int16_t error_x)
{
  /* 当前装配实测：目标在左侧时水平轴取正方向；图像 X 正方向向右，故取反。 */
  return speed_from_error(config, target_present, error_x);
}

int32_t VisualTrackController_VerticalSpeed(
    const VisualTrackControllerConfig *config, bool target_present,
    int16_t error_y)
{
  /* P07 已实测俯仰 DIR=Low 为向上；图像 Y 正方向向下，故取反。 */
  return speed_from_error(config, target_present, error_y);
}
