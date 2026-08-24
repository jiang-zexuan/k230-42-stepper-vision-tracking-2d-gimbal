#include "relative_position_tracker.h"

void RelativePositionTracker_Init(RelativePositionTracker *tracker)
{
  tracker->valid = false;
  tracker->position_pulses = 0;
}

void RelativePositionTracker_SetManualZero(RelativePositionTracker *tracker)
{
  tracker->valid = true;
  tracker->position_pulses = 0;
}

void RelativePositionTracker_Invalidate(RelativePositionTracker *tracker)
{
  tracker->valid = false;
}

bool RelativePositionTracker_IsValid(const RelativePositionTracker *tracker)
{
  return tracker->valid;
}

bool RelativePositionTracker_ApplyCompletedPulseDelta(
    RelativePositionTracker *tracker, int32_t pulse_delta)
{
  if (!tracker->valid)
  {
    return false;
  }

  tracker->position_pulses += pulse_delta;
  return true;
}

int32_t RelativePositionTracker_GetPositionPulses(
    const RelativePositionTracker *tracker)
{
  return tracker->position_pulses;
}
