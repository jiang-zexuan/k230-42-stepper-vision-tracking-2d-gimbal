#include "uart_text_line_accumulator.h"

#include <string.h>

void UartTextLineAccumulator_Init(UartTextLineAccumulator *accumulator)
{
  accumulator->size = 0U;
  accumulator->discarding_until_newline = 0U;
  accumulator->overflow_count = 0U;
}

uint8_t UartTextLineAccumulator_Consume(
    UartTextLineAccumulator *accumulator, UartRxFrame *mailbox,
    const uint8_t *data, uint16_t size)
{
  uint16_t index;
  uint8_t completed = 0U;

  for (index = 0U; index < size; index++)
  {
    uint8_t byte = data[index];

    if (byte == '\r')
    {
      continue;
    }

    if (byte == '\n')
    {
      if ((!accumulator->discarding_until_newline) && (accumulator->size > 0U))
      {
        if (UartRxFrame_Store(mailbox, accumulator->data, accumulator->size))
        {
          completed = 1U;
        }
      }
      accumulator->size = 0U;
      accumulator->discarding_until_newline = 0U;
      continue;
    }

    if (accumulator->discarding_until_newline)
    {
      continue;
    }

    if (accumulator->size < UART_RX_FRAME_CAPACITY)
    {
      accumulator->data[accumulator->size++] = byte;
    }
    else
    {
      accumulator->size = 0U;
      accumulator->discarding_until_newline = 1U;
      accumulator->overflow_count++;
    }
  }

  return completed;
}
