#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../PanViewF407/Core/Inc/uart_rx_frame.h"

static void test_store_copies_received_bytes_into_the_pending_frame(void)
{
    UartRxFrame frame;
    const uint8_t input[] = "HELLO";

    UartRxFrame_Init(&frame);

    assert(UartRxFrame_Store(&frame, input, sizeof(input) - 1U) == true);
    assert(frame.pending == true);
    assert(frame.size == sizeof(input) - 1U);
    assert(frame.received_count == 1U);
    assert(memcmp(frame.data, input, sizeof(input) - 1U) == 0);
}

static void test_store_drops_a_new_frame_until_the_pending_frame_is_released(void)
{
    UartRxFrame frame;
    const uint8_t first[] = "A";
    const uint8_t second[] = "B";

    UartRxFrame_Init(&frame);

    assert(UartRxFrame_Store(&frame, first, sizeof(first) - 1U) == true);
    assert(UartRxFrame_Store(&frame, second, sizeof(second) - 1U) == false);
    assert(frame.dropped_count == 1U);
    assert(frame.data[0] == 'A');

    UartRxFrame_Release(&frame);
    assert(UartRxFrame_Store(&frame, second, sizeof(second) - 1U) == true);
    assert(frame.data[0] == 'B');
}

int main(void)
{
    test_store_copies_received_bytes_into_the_pending_frame();
    test_store_drops_a_new_frame_until_the_pending_frame_is_released();
    puts("uart_rx_frame_test: PASS");
    return 0;
}
