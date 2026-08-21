#ifndef MOTION_LIMITS_H
#define MOTION_LIMITS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * 当前水平轴设计限值：人工回中为 0，允许 -90 deg 到 +90 deg。
 * 依据：X42S 当前 16 细分实测为 3200 脉冲/圈，因此 90 deg = 800 脉冲。
 * 这些常量只适用于当前水平轴装配和当前驱动器细分设置；细分改变后必须重算。
 */
#define MOTION_LIMITS_HORIZONTAL_MIN_PULSES (-800L)
#define MOTION_LIMITS_HORIZONTAL_MAX_PULSES (800L)

typedef struct
{
  int32_t min_pulses;
  int32_t max_pulses;
} MotionLimits;

typedef enum
{
  MOTION_LIMITS_ACCEPTED = 0,
  MOTION_LIMITS_REJECT_POSITION_INVALID,
  MOTION_LIMITS_REJECT_OUT_OF_RANGE,
  MOTION_LIMITS_REJECT_INVALID_ARGUMENT
} MotionLimitsResult;

MotionLimits MotionLimits_HorizontalDefault(void);
MotionLimitsResult MotionLimits_CheckRelativeMove(
    const MotionLimits *limits, bool position_valid, int32_t current_pulses,
    int32_t delta_pulses, int32_t *target_pulses);

#endif
