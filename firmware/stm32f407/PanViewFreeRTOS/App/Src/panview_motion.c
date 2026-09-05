#include "panview_motion.h"

static float limit_float_change(float current, float previous)
{
  float difference = current - previous;

  if (difference > PANVIEW_MOTION_MAX_VISION_CHANGE_PX)
  {
    return previous + PANVIEW_MOTION_MAX_VISION_CHANGE_PX;
  }
  if (difference < -PANVIEW_MOTION_MAX_VISION_CHANGE_PX)
  {
    return previous - PANVIEW_MOTION_MAX_VISION_CHANGE_PX;
  }
  return current;
}

void PanView_Motion_LimitVisionChange(const VisionResult *input,
                                      VisionResult *filtered)
{
  static float previous_x = 0.0f;
  static float previous_y = 0.0f;
  static uint32_t previous_tick = 0U;

  if ((input == 0) || (filtered == 0))
  {
    return;
  }

  *filtered = *input;

  if (input->target_present == 0U)
  {
    previous_x = input->center_x_px;
    previous_y = input->center_y_px;
    previous_tick = input->received_tick_ms;
    return;
  }

  if (input->received_tick_ms != previous_tick)
  {
    filtered->center_x_px = limit_float_change(input->center_x_px, previous_x);
    filtered->center_y_px = limit_float_change(input->center_y_px, previous_y);
    previous_x = filtered->center_x_px;
    previous_y = filtered->center_y_px;
    previous_tick = input->received_tick_ms;
  }
  else
  {
    filtered->center_x_px = previous_x;
    filtered->center_y_px = previous_y;
  }
}

void PanView_Motion_CalculateError(const VisionResult *vision,
                                   int16_t *error_x_px,
                                   int16_t *error_y_px)
{
  int32_t frame_center_x;
  int32_t frame_center_y;
  int32_t calculated_error_x;
  int32_t calculated_error_y;

  /* 输出指针无效时，函数不能继续写内存。 */
  if ((error_x_px == 0) || (error_y_px == 0))
  {
    return;
  }

  /* 默认输出为 0：无目标或输入无效时，运动任务应保持停止。 */
  *error_x_px = 0;
  *error_y_px = 0;

  if ((vision == 0) || (vision->target_present == 0U))
  {
    return;
  }

  /* 用整数除法得到画面中心，单位仍然是像素。 */
  frame_center_x = (int32_t)vision->frame_width_px / 2;
  frame_center_y = (int32_t)vision->frame_height_px / 2;

  calculated_error_x = (int32_t)vision->center_x_px - frame_center_x;
  calculated_error_y = (int32_t)vision->center_y_px - frame_center_y;

  /* 当前图像尺寸为 1920x1080，结果能安全放入 int16_t。 */
  *error_x_px = (int16_t)calculated_error_x;
  *error_y_px = (int16_t)calculated_error_y;
}

void PanView_Motion_CreateCommand(const VisionResult *vision,
                                  uint32_t generated_tick_ms,
                                  MotionCommand *command)
{
  int16_t error_x;
  int16_t error_y;
  static float integral_x = 0.0f;
  static float integral_y = 0.0f;
  static float derivative_x = 0.0f;
  static float derivative_y = 0.0f;
  static int16_t previous_error_x = 0;
  static int16_t previous_error_y = 0;
  static int32_t previous_output_x = 0;
  static int32_t previous_output_y = 0;
  static uint32_t previous_tick = 0U;
  static uint8_t previous_valid = 0U;

  if (command == 0)
  {
    return;
  }

  command->pan_speed_steps_per_second = 0;
  command->pitch_speed_steps_per_second = 0;
  command->generated_tick_ms = generated_tick_ms;
  command->valid = 0U;

  if ((vision == 0) || (vision->target_present == 0U))
  {
    integral_x = integral_y = 0.0f;
    derivative_x = derivative_y = 0.0f;
    previous_error_x = previous_error_y = 0;
    previous_output_x = previous_output_y = 0;
    previous_valid = 0U;
    return;
  }

  PanView_Motion_CalculateError(vision, &error_x, &error_y);
  {
    float dt = previous_valid != 0U
                   ? (float)(generated_tick_ms - previous_tick) / 1000.0f
                   : 0.0f;
    float raw_dx = (dt > 0.0f) ? ((float)error_x - previous_error_x) / dt : 0.0f;
    float raw_dy = (dt > 0.0f) ? ((float)error_y - previous_error_y) / dt : 0.0f;
    float output_x;
    float output_y;

    if ((error_x <= PANVIEW_MOTION_DEAD_ZONE_PX) &&
        (error_x >= -PANVIEW_MOTION_DEAD_ZONE_PX))
    {
      integral_x = 0.0f;
      derivative_x = 0.0f;
      output_x = 0.0f;
    }
    else
    {
      derivative_x = 0.35f * raw_dx + 0.65f * derivative_x;
      integral_x += (dt > 0.0f) ? (float)error_x * dt : 0.0f;
      if (integral_x > PANVIEW_MOTION_INTEGRAL_LIMIT) integral_x = PANVIEW_MOTION_INTEGRAL_LIMIT;
      if (integral_x < -PANVIEW_MOTION_INTEGRAL_LIMIT) integral_x = -PANVIEW_MOTION_INTEGRAL_LIMIT;
      output_x = PANVIEW_MOTION_PAN_GAIN_STEPS_PER_PIXEL * error_x +
                 PANVIEW_MOTION_I_GAIN * integral_x + PANVIEW_MOTION_D_GAIN * derivative_x;
    }

    if ((error_y <= PANVIEW_MOTION_DEAD_ZONE_PX) &&
        (error_y >= -PANVIEW_MOTION_DEAD_ZONE_PX))
    {
      integral_y = 0.0f;
      derivative_y = 0.0f;
      output_y = 0.0f;
    }
    else
    {
      derivative_y = 0.35f * raw_dy + 0.65f * derivative_y;
      integral_y += (dt > 0.0f) ? (float)error_y * dt : 0.0f;
      if (integral_y > PANVIEW_MOTION_INTEGRAL_LIMIT) integral_y = PANVIEW_MOTION_INTEGRAL_LIMIT;
      if (integral_y < -PANVIEW_MOTION_INTEGRAL_LIMIT) integral_y = -PANVIEW_MOTION_INTEGRAL_LIMIT;
      output_y = PANVIEW_MOTION_PITCH_GAIN_STEPS_PER_PIXEL * error_y +
                 PANVIEW_MOTION_I_GAIN * integral_y + PANVIEW_MOTION_D_GAIN * derivative_y;
    }

    if (output_x > 800.0f) output_x = 800.0f;
    if (output_x < -800.0f) output_x = -800.0f;
    if (output_y > 800.0f) output_y = 800.0f;
    if (output_y < -800.0f) output_y = -800.0f;

    command->pan_speed_steps_per_second = (int32_t)output_x;
    command->pitch_speed_steps_per_second = (int32_t)output_y;
  }
  previous_error_x = error_x;
  previous_error_y = error_y;
  previous_tick = generated_tick_ms;
  previous_output_x = command->pan_speed_steps_per_second;
  previous_output_y = command->pitch_speed_steps_per_second;
  (void)previous_output_x;
  (void)previous_output_y;
  previous_valid = 1U;
  command->valid = 1U;
}

static int32_t approach_speed(int32_t target_speed, int32_t previous_speed)
{
  int64_t speed_difference = (int64_t)target_speed - previous_speed;

  if (speed_difference > PANVIEW_MOTION_SLEW_STEP_STEPS_PER_SECOND)
  {
    return previous_speed + PANVIEW_MOTION_SLEW_STEP_STEPS_PER_SECOND;
  }
  if (speed_difference < -PANVIEW_MOTION_SLEW_STEP_STEPS_PER_SECOND)
  {
    return previous_speed - PANVIEW_MOTION_SLEW_STEP_STEPS_PER_SECOND;
  }

  /* 已经离目标很近时，直接到达目标，避免在目标附近来回变化。 */
  return target_speed;
}

void PanView_Motion_ApplySlewRate(const MotionCommand *target_command,
                                  const MotionCommand *previous_command,
                                  MotionCommand *limited_command)
{
  int32_t previous_pan_speed = 0;
  int32_t previous_pitch_speed = 0;

  if ((target_command == 0) || (limited_command == 0))
  {
    return;
  }

  /* 无目标时立即停止，不让旧速度继续存在。 */
  if (target_command->valid == 0U)
  {
    limited_command->pan_speed_steps_per_second = 0;
    limited_command->pitch_speed_steps_per_second = 0;
    limited_command->generated_tick_ms = target_command->generated_tick_ms;
    limited_command->valid = 0U;
    return;
  }

  if (previous_command != 0)
  {
    previous_pan_speed = previous_command->pan_speed_steps_per_second;
    previous_pitch_speed = previous_command->pitch_speed_steps_per_second;
  }

  limited_command->pan_speed_steps_per_second = approach_speed(
      target_command->pan_speed_steps_per_second, previous_pan_speed);
  limited_command->pitch_speed_steps_per_second = approach_speed(
      target_command->pitch_speed_steps_per_second, previous_pitch_speed);
  limited_command->generated_tick_ms = target_command->generated_tick_ms;
  limited_command->valid = 1U;
}

uint8_t PanView_Motion_UpdateLock(uint8_t target_present,
                                  int16_t error_x_px,
                                  int16_t error_y_px,
                                  uint8_t previous_locked)
{
  if (target_present == 0U)
  {
    return 0U;
  }

  if (previous_locked != 0U)
  {
    /* 已锁定时，只有误差真正超过较大的退出阈值才解除锁定。 */
    if ((error_x_px > PANVIEW_MOTION_LOCK_EXIT_PX) ||
        (error_x_px < -PANVIEW_MOTION_LOCK_EXIT_PX) ||
        (error_y_px > PANVIEW_MOTION_LOCK_EXIT_PX) ||
        (error_y_px < -PANVIEW_MOTION_LOCK_EXIT_PX))
    {
      return 0U;
    }
    return 1U;
  }

  /* 未锁定时，双轴都进入较小的范围才建立锁定。 */
  if ((error_x_px <= PANVIEW_MOTION_LOCK_ENTER_PX) &&
      (error_x_px >= -PANVIEW_MOTION_LOCK_ENTER_PX) &&
      (error_y_px <= PANVIEW_MOTION_LOCK_ENTER_PX) &&
      (error_y_px >= -PANVIEW_MOTION_LOCK_ENTER_PX))
  {
    return 1U;
  }

  return 0U;
}
