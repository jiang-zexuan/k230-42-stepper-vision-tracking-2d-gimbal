#include "motion_limits.h"

#include <stddef.h>

MotionLimits MotionLimits_HorizontalDefault(void)
{
  MotionLimits limits = {
      MOTION_LIMITS_HORIZONTAL_MIN_PULSES,
      MOTION_LIMITS_HORIZONTAL_MAX_PULSES};
  return limits;
}

MotionLimitsResult MotionLimits_CheckRelativeMove(
    const MotionLimits *limits, bool position_valid, int32_t current_pulses,
    int32_t delta_pulses, int32_t *target_pulses)
{
  int64_t target;

  if ((limits == NULL) || (target_pulses == NULL) ||
      (limits->min_pulses > limits->max_pulses))
  {
    return MOTION_LIMITS_REJECT_INVALID_ARGUMENT;
  }

  if (!position_valid)
  {
    return MOTION_LIMITS_REJECT_POSITION_INVALID;
  }

  /* 用 64 位中间值，避免 int32_t 的当前位置加增量发生溢出。 */
  target = (int64_t)current_pulses + (int64_t)delta_pulses;
  if ((target < limits->min_pulses) || (target > limits->max_pulses))
  {
    return MOTION_LIMITS_REJECT_OUT_OF_RANGE;
  }

  *target_pulses = (int32_t)target;
  return MOTION_LIMITS_ACCEPTED;
}
