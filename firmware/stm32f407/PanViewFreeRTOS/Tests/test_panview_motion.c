#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "panview_motion.h"

/* 调参时两条轴必须拥有独立的 P 参数，避免修改俯仰时影响水平轴。 */
#ifndef PANVIEW_MOTION_PAN_GAIN_STEPS_PER_PIXEL
#error "Missing independent pan P gain"
#endif

#ifndef PANVIEW_MOTION_PITCH_GAIN_STEPS_PER_PIXEL
#error "Missing independent pitch P gain"
#endif

static VisionResult make_result(uint8_t present, float center_x, float center_y)
{
  VisionResult result;
  memset(&result, 0, sizeof(result));
  result.target_present = present;
  result.frame_width_px = 1920U;
  result.frame_height_px = 1080U;
  result.center_x_px = center_x;
  result.center_y_px = center_y;
  return result;
}

int main(void)
{
  VisionResult vision;
  int16_t error_x;
  int16_t error_y;

  vision = make_result(0U, 1200.0f, 800.0f);
  PanView_Motion_CalculateError(&vision, &error_x, &error_y);
  assert(error_x == 0);
  assert(error_y == 0);

  vision = make_result(1U, 960.0f, 540.0f);
  PanView_Motion_CalculateError(&vision, &error_x, &error_y);
  assert(error_x == 0);
  assert(error_y == 0);

  vision = make_result(1U, 1035.0f, 393.0f);
  PanView_Motion_CalculateError(&vision, &error_x, &error_y);
  assert(error_x == 75);
  assert(error_y == -147);

  vision = make_result(1U, 800.0f, 700.0f);
  PanView_Motion_CalculateError(&vision, &error_x, &error_y);
  assert(error_x == -160);
  assert(error_y == 160);

  MotionCommand command;
  PanView_Motion_CreateCommand(&vision, 1234U, &command);
  assert(command.valid == 1U);
  /* 当前 P=2.5，因此 160 px 误差对应 400 step/s。 */
  assert(command.pan_speed_steps_per_second == -400);
  assert(command.pitch_speed_steps_per_second == 400);

  vision = make_result(1U, 1010.0f, 540.0f);
  PanView_Motion_CreateCommand(&vision, 1235U, &command);
  assert(command.pan_speed_steps_per_second == 125);

  vision = make_result(1U, 1920.0f, 540.0f);
  PanView_Motion_CreateCommand(&vision, 1236U, &command);
  assert(command.pan_speed_steps_per_second == 800);

  vision = make_result(0U, 1920.0f, 540.0f);
  PanView_Motion_CreateCommand(&vision, 1237U, &command);
  assert(command.valid == 0U);
  assert(command.pan_speed_steps_per_second == 0);

  /* T06-4：有目标时，速度每个周期最多只变化 40 step/s。 */
  MotionCommand previous_command = {0};
  MotionCommand target_command = {0};
  MotionCommand limited_command = {0};

  target_command.valid = 1U;
  target_command.pan_speed_steps_per_second = 800;
  target_command.pitch_speed_steps_per_second = -200;
  target_command.generated_tick_ms = 2000U;

  PanView_Motion_ApplySlewRate(&target_command, &previous_command,
                               &limited_command);
  assert(limited_command.valid == 1U);
  assert(limited_command.pan_speed_steps_per_second == 40);
  assert(limited_command.pitch_speed_steps_per_second == -40);

  /* 以上一次输出为起点，下一周期继续逐步接近目标速度。 */
  previous_command = limited_command;
  PanView_Motion_ApplySlewRate(&target_command, &previous_command,
                               &limited_command);
  assert(limited_command.pan_speed_steps_per_second == 80);
  assert(limited_command.pitch_speed_steps_per_second == -80);

  /* 目标消失时不缓慢减速，立即输出无效命令和零速度。 */
  target_command.valid = 0U;
  target_command.pan_speed_steps_per_second = 0;
  target_command.pitch_speed_steps_per_second = 0;
  target_command.generated_tick_ms = 2010U;
  PanView_Motion_ApplySlewRate(&target_command, &limited_command,
                               &previous_command);
  assert(previous_command.valid == 0U);
  assert(previous_command.pan_speed_steps_per_second == 0);
  assert(previous_command.pitch_speed_steps_per_second == 0);

  /* T06-5：双轴都进入 40 px 范围后才认为目标已经锁定。 */
  uint8_t target_locked = 0U;
  target_locked = PanView_Motion_UpdateLock(1U, 30, -40, target_locked);
  assert(target_locked == 1U);

  /* 已锁定后，误差处于 40～60 px 时继续保持锁定，避免反复启停。 */
  target_locked = PanView_Motion_UpdateLock(1U, 50, -55, target_locked);
  assert(target_locked == 1U);

  /* 任意一轴超过退出阈值 60 px，就解除锁定。 */
  target_locked = PanView_Motion_UpdateLock(1U, 61, 0, target_locked);
  assert(target_locked == 0U);

  /* 无目标时必须解除锁定。 */
  target_locked = PanView_Motion_UpdateLock(0U, 0, 0, 1U);
  assert(target_locked == 0U);

  printf("T06_ERROR_TEST=PASS center=(960,540)\n");
  return 0;
}
