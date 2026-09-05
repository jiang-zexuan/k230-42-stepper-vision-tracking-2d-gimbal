#include "panview_text_line.h"

void PanView_TextLine_Init(PanViewTextLineAccumulator *accumulator)
{
  if (accumulator == 0)
  {
    return;
  }

  accumulator->size = 0U;
  accumulator->discarding = 0U;
}

uint8_t PanView_TextLine_FeedByte(PanViewTextLineAccumulator *accumulator,
                                  uint8_t byte,
                                  char *output,
                                  uint16_t output_capacity)
{
  uint16_t i;

  if ((accumulator == 0) || (output == 0) || (output_capacity == 0U))
  {
    return 0U;
  }

  /* CR 只是行尾的一部分，不写入正文；真正提交由 LF 触发。 */
  if (byte == '\r')
  {
    return 0U;
  }

  if (byte == '\n')
  {
    if (accumulator->discarding != 0U)
    {
      accumulator->size = 0U;
      accumulator->discarding = 0U;
      return 0U;
    }

    if ((accumulator->size + 1U) > output_capacity)
    {
      accumulator->size = 0U;
      return 0U;
    }

    for (i = 0U; i < accumulator->size; i++)
    {
      output[i] = accumulator->data[i];
    }
    output[accumulator->size] = '\0';
    accumulator->size = 0U;
    return 1U;
  }

  if (accumulator->discarding != 0U)
  {
    return 0U;
  }

  if (accumulator->size >= (PANVIEW_TEXT_LINE_CAPACITY - 1U))
  {
    /* 当前行过长：丢弃到下一个换行，避免数组越界。 */
    accumulator->size = 0U;
    accumulator->discarding = 1U;
    return 0U;
  }

  accumulator->data[accumulator->size++] = (char)byte;
  return 0U;
}
