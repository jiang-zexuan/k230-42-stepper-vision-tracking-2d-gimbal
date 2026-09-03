#ifndef PANVIEW_RING_BUFFER_H
#define PANVIEW_RING_BUFFER_H

#include <stdint.h>

/*
 * UART 字节环形缓冲区的物理大小。
 * 为了用读写位置区分“空”和“满”，实际最多保存 1023 字节。
 */
#define PANVIEW_RING_BUFFER_CAPACITY 1024U

typedef struct
{
  uint8_t data[PANVIEW_RING_BUFFER_CAPACITY];

  /* 中断负责推进写位置，VisionRxTask 负责推进读位置。 */
  volatile uint16_t write_index;
  volatile uint16_t read_index;
} PanViewRingBuffer;

/* 清空环形缓冲区。 */
void PanView_RingBuffer_Init(PanViewRingBuffer *buffer);

/* 按顺序写入字节，返回实际写入数量。 */
uint16_t PanView_RingBuffer_Write(PanViewRingBuffer *buffer,
                                  const uint8_t *data,
                                  uint16_t length);

/* 按顺序读取字节，返回实际读取数量。 */
uint16_t PanView_RingBuffer_Read(PanViewRingBuffer *buffer,
                                 uint8_t *output,
                                 uint16_t output_size);

/* 返回当前等待任务读取的字节数。 */
uint16_t PanView_RingBuffer_GetUsed(const PanViewRingBuffer *buffer);

#endif
