#ifndef RELATIVE_POSITION_TRACKER_H
#define RELATIVE_POSITION_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

/* 仅表示软件脉冲坐标；没有外部原点或位置反馈时不代表真实绝对位置。 */
typedef struct
{
  bool valid;
  int32_t position_pulses;
} RelativePositionTracker;

void RelativePositionTracker_Init(RelativePositionTracker *tracker);
void RelativePositionTracker_SetManualZero(RelativePositionTracker *tracker);
void RelativePositionTracker_Invalidate(RelativePositionTracker *tracker);
bool RelativePositionTracker_IsValid(const RelativePositionTracker *tracker);
bool RelativePositionTracker_ApplyCompletedPulseDelta(
    RelativePositionTracker *tracker, int32_t pulse_delta);
int32_t RelativePositionTracker_GetPositionPulses(
    const RelativePositionTracker *tracker);

#endif
