#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/frame_sequence_tracker.h"

static void test_first_and_consecutive_sequences_are_accepted(void)
{
    FrameSequenceTracker tracker;

    FrameSequenceTracker_Init(&tracker);

    assert(FrameSequenceTracker_Observe(&tracker, 10U) == FRAME_SEQUENCE_FIRST);
    assert(FrameSequenceTracker_Observe(&tracker, 11U) == FRAME_SEQUENCE_IN_ORDER);
    assert(tracker.missing_count == 0U);
}

static void test_forward_jump_counts_missing_frames(void)
{
    FrameSequenceTracker tracker;

    FrameSequenceTracker_Init(&tracker);
    (void)FrameSequenceTracker_Observe(&tracker, 10U);

    assert(FrameSequenceTracker_Observe(&tracker, 13U) == FRAME_SEQUENCE_GAP);
    assert(tracker.missing_count == 2U);
    assert(tracker.last_sequence == 13U);
}

static void test_duplicate_and_old_sequences_do_not_move_the_latest_sequence(void)
{
    FrameSequenceTracker tracker;

    FrameSequenceTracker_Init(&tracker);
    (void)FrameSequenceTracker_Observe(&tracker, 20U);

    assert(FrameSequenceTracker_Observe(&tracker, 20U) == FRAME_SEQUENCE_DUPLICATE);
    assert(FrameSequenceTracker_Observe(&tracker, 19U) == FRAME_SEQUENCE_OUT_OF_ORDER);
    assert(tracker.duplicate_count == 1U);
    assert(tracker.out_of_order_count == 1U);
    assert(tracker.last_sequence == 20U);
}

int main(void)
{
    test_first_and_consecutive_sequences_are_accepted();
    test_forward_jump_counts_missing_frames();
    test_duplicate_and_old_sequences_do_not_move_the_latest_sequence();
    puts("frame_sequence_tracker_test: PASS");
    return 0;
}
