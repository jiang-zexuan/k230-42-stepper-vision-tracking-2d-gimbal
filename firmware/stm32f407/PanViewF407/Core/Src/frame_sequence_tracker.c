#include "frame_sequence_tracker.h"

void FrameSequenceTracker_Init(FrameSequenceTracker *tracker)
{
    tracker->has_sequence = 0U;
    tracker->last_sequence = 0U;
    tracker->missing_count = 0U;
    tracker->duplicate_count = 0U;
    tracker->out_of_order_count = 0U;
}

FrameSequenceResult FrameSequenceTracker_Observe(FrameSequenceTracker *tracker,
                                                  uint8_t sequence)
{
    uint8_t difference;

    if (tracker->has_sequence == 0U)
    {
        tracker->has_sequence = 1U;
        tracker->last_sequence = sequence;
        return FRAME_SEQUENCE_FIRST;
    }

    difference = (uint8_t)(sequence - tracker->last_sequence);
    if (difference == 0U)
    {
        tracker->duplicate_count++;
        return FRAME_SEQUENCE_DUPLICATE;
    }

    if (difference == 1U)
    {
        tracker->last_sequence = sequence;
        return FRAME_SEQUENCE_IN_ORDER;
    }

    /* 差值小于 128 表示向前跳号；更大值视为到达顺序落后。 */
    if (difference < 128U)
    {
        tracker->missing_count += (uint32_t)(difference - 1U);
        tracker->last_sequence = sequence;
        return FRAME_SEQUENCE_GAP;
    }

    tracker->out_of_order_count++;
    return FRAME_SEQUENCE_OUT_OF_ORDER;
}
