#ifndef FRAME_SEQUENCE_TRACKER_H
#define FRAME_SEQUENCE_TRACKER_H

#include <stdint.h>

typedef enum {
    FRAME_SEQUENCE_FIRST,
    FRAME_SEQUENCE_IN_ORDER,
    FRAME_SEQUENCE_GAP,
    FRAME_SEQUENCE_DUPLICATE,
    FRAME_SEQUENCE_OUT_OF_ORDER
} FrameSequenceResult;

/*
 * 8 bit 序号的观察统计。跳号仅反映接收端看到的缺口，
 * 不能单独证明丢失发生在发送端、链路或本地缓冲区。
 */
typedef struct {
    uint8_t has_sequence;
    uint8_t last_sequence;
    uint32_t missing_count;
    uint32_t duplicate_count;
    uint32_t out_of_order_count;
} FrameSequenceTracker;

void FrameSequenceTracker_Init(FrameSequenceTracker *tracker);
FrameSequenceResult FrameSequenceTracker_Observe(FrameSequenceTracker *tracker,
                                                  uint8_t sequence);

#endif
