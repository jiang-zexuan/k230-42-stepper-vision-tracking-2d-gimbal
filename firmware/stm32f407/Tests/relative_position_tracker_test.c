#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/relative_position_tracker.h"

static void test_tracker_requires_manual_zero_before_tracking_moves(void)
{
    RelativePositionTracker tracker;

    RelativePositionTracker_Init(&tracker);

    assert(!RelativePositionTracker_IsValid(&tracker));
    assert(!RelativePositionTracker_ApplyCompletedPulseDelta(&tracker, 800));

    RelativePositionTracker_SetManualZero(&tracker);
    assert(RelativePositionTracker_IsValid(&tracker));
    assert(RelativePositionTracker_GetPositionPulses(&tracker) == 0);

    assert(RelativePositionTracker_ApplyCompletedPulseDelta(&tracker, 800));
    assert(RelativePositionTracker_GetPositionPulses(&tracker) == 800);
    assert(RelativePositionTracker_ApplyCompletedPulseDelta(&tracker, -800));
    assert(RelativePositionTracker_GetPositionPulses(&tracker) == 0);

    RelativePositionTracker_Invalidate(&tracker);
    assert(!RelativePositionTracker_IsValid(&tracker));
}

int main(void)
{
    test_tracker_requires_manual_zero_before_tracking_moves();
    puts("relative_position_tracker_test: PASS");
    return 0;
}
