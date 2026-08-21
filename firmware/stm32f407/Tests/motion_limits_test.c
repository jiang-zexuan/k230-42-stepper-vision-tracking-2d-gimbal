#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/motion_limits.h"

static void test_move_requires_valid_manual_zero(void)
{
  MotionLimits limits = MotionLimits_HorizontalDefault();
  int32_t target_pulses = 0;

  assert(MotionLimits_CheckRelativeMove(&limits, false, 0, 800,
                                       &target_pulses) ==
         MOTION_LIMITS_REJECT_POSITION_INVALID);
}

static void test_boundary_accepts_inward_and_rejects_outward_moves(void)
{
  MotionLimits limits = MotionLimits_HorizontalDefault();
  int32_t target_pulses = 0;

  assert(MotionLimits_CheckRelativeMove(&limits, true, 0, 800,
                                       &target_pulses) ==
         MOTION_LIMITS_ACCEPTED);
  assert(target_pulses == 800);

  assert(MotionLimits_CheckRelativeMove(&limits, true, 800, 800,
                                       &target_pulses) ==
         MOTION_LIMITS_REJECT_OUT_OF_RANGE);
  assert(MotionLimits_CheckRelativeMove(&limits, true, 800, -800,
                                       &target_pulses) ==
         MOTION_LIMITS_ACCEPTED);
  assert(target_pulses == 0);

  assert(MotionLimits_CheckRelativeMove(&limits, true, 0, -800,
                                       &target_pulses) ==
         MOTION_LIMITS_ACCEPTED);
  assert(target_pulses == -800);
  assert(MotionLimits_CheckRelativeMove(&limits, true, -800, -800,
                                       &target_pulses) ==
         MOTION_LIMITS_REJECT_OUT_OF_RANGE);
  assert(MotionLimits_CheckRelativeMove(&limits, true, -800, 800,
                                       &target_pulses) ==
         MOTION_LIMITS_ACCEPTED);
  assert(target_pulses == 0);
}

static void test_limits_are_inclusive(void)
{
  MotionLimits limits = MotionLimits_HorizontalDefault();
  int32_t target_pulses = 0;

  assert(MotionLimits_CheckRelativeMove(&limits, true, 0, 800,
                                       &target_pulses) ==
         MOTION_LIMITS_ACCEPTED);
  assert(target_pulses == limits.max_pulses);
  assert(MotionLimits_CheckRelativeMove(&limits, true, 0, -800,
                                       &target_pulses) ==
         MOTION_LIMITS_ACCEPTED);
  assert(target_pulses == limits.min_pulses);
}

int main(void)
{
  test_move_requires_valid_manual_zero();
  test_boundary_accepts_inward_and_rejects_outward_moves();
  test_limits_are_inclusive();
  puts("motion_limits_test: PASS");
  return 0;
}
