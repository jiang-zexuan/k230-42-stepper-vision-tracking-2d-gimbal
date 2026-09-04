#include "panview_motion.h"

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

static int32_t limit_speed(int32_t speed)
{
  if (speed > PANVIEW_MOTION_MAX_SPEED_STEPS_PER_SECOND)
  {
    return PANVIEW_MOTION_MAX_SPEED_STEPS_PER_SECOND;
  }
  if (speed < -PANVIEW_MOTION_MAX_SPEED_STEPS_PER_SECOND)
  {
    return -PANVIEW_MOTION_MAX_SPEED_STEPS_PER_SECOND;
  }
  return speed;
}

static int32_t error_to_speed(int16_t error_px)
{
  if ((error_px <= PANVIEW_MOTION_DEAD_ZONE_PX) &&
      (error_px >= -PANVIEW_MOTION_DEAD_ZONE_PX))
  {
    return 0;
  }
  return limit_speed((int32_t)error_px *
                     PANVIEW_MOTION_GAIN_STEPS_PER_PIXEL);
}

void PanView_Motion_CreateCommand(const VisionResult *vision,
                                  uint32_t generated_tick_ms,
                                  MotionCommand *command)
{
  int16_t error_x;
  int16_t error_y;

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
    return;
  }

  PanView_Motion_CalculateError(vision, &error_x, &error_y);
  command->pan_speed_steps_per_second = error_to_speed(error_x);
  command->pitch_speed_steps_per_second = error_to_speed(error_y);
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
