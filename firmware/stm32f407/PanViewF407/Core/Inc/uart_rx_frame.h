#ifndef UART_RX_FRAME_H
#define UART_RX_FRAME_H

#include <stdbool.h>
#include <stdint.h>

enum {
    /* 单位：byte；P03 首次 DMA 接收实验的单段最大长度。 */
    UART_RX_FRAME_CAPACITY = 64U
};

/*
 * UART 中断与主循环之间的单帧邮箱。
 * 中断写入 pending 帧；主循环处理完后调用 Release 允许下一帧进入。
 */
typedef struct {
    uint8_t data[UART_RX_FRAME_CAPACITY];
    uint16_t size;
    volatile bool pending;
    uint32_t received_count;
    uint32_t dropped_count;
} UartRxFrame;

void UartRxFrame_Init(UartRxFrame *frame);

/* 从 DMA 缓冲区复制一段已接收数据；邮箱忙时丢弃新数据并返回 false。 */
bool UartRxFrame_Store(UartRxFrame *frame, const uint8_t *data, uint16_t size);

/* 主循环处理完当前帧后调用，允许中断保存下一帧。 */
void UartRxFrame_Release(UartRxFrame *frame);

#endif
