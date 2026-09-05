#ifndef PANVIEW_TEXT_LINE_H
#define PANVIEW_TEXT_LINE_H

#include <stdint.h>

#define PANVIEW_TEXT_LINE_CAPACITY 256U

typedef struct
{
  char data[PANVIEW_TEXT_LINE_CAPACITY];
  uint16_t size;
  uint8_t discarding;
} PanViewTextLineAccumulator;

void PanView_TextLine_Init(PanViewTextLineAccumulator *accumulator);

/* 输入一个字节；遇到 LF 时把完整行复制到 output 并返回 1。 */
uint8_t PanView_TextLine_FeedByte(PanViewTextLineAccumulator *accumulator,
                                  uint8_t byte,
                                  char *output,
                                  uint16_t output_capacity);

#endif
