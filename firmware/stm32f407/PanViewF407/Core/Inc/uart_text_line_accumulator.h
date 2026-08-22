#ifndef UART_TEXT_LINE_ACCUMULATOR_H
#define UART_TEXT_LINE_ACCUMULATOR_H

#include <stdint.h>

#include "uart_rx_frame.h"

/* 将不定长 UART 字节片段按 CR/LF 拼成完整文本行。 */
typedef struct
{
  uint8_t data[UART_RX_FRAME_CAPACITY];
  uint16_t size;
  uint8_t discarding_until_newline;
  uint32_t overflow_count;
} UartTextLineAccumulator;

void UartTextLineAccumulator_Init(UartTextLineAccumulator *accumulator);

/*
 * 消费一段 DMA 数据；遇到换行时尝试把完整行放入邮箱。
 * 返回 true 表示本次消费至少完成并提交了一行。
 */
uint8_t UartTextLineAccumulator_Consume(
    UartTextLineAccumulator *accumulator, UartRxFrame *mailbox,
    const uint8_t *data, uint16_t size);

#endif
