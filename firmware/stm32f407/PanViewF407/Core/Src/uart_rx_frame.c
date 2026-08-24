#include "uart_rx_frame.h"

#include <string.h>

void UartRxFrame_Init(UartRxFrame *frame)
{
    frame->size = 0U;
    frame->pending = false;
    frame->received_count = 0U;
    frame->dropped_count = 0U;
}

bool UartRxFrame_Store(UartRxFrame *frame, const uint8_t *data, uint16_t size)
{
    if ((frame->pending) || (size == 0U) || (size > UART_RX_FRAME_CAPACITY)) {
        frame->dropped_count++;
        return false;
    }

    memcpy(frame->data, data, size);
    frame->size = size;
    frame->received_count++;
    frame->pending = true;
    return true;
}

void UartRxFrame_Release(UartRxFrame *frame)
{
    frame->pending = false;
}
