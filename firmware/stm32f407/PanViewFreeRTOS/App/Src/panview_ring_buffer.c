#include "panview_ring_buffer.h"

/* 将位置向前移动一格，到达数组末尾后回到 0。 */
static uint16_t PanView_RingBuffer_NextIndex(uint16_t index)
{
  index++;
  if (index >= PANVIEW_RING_BUFFER_CAPACITY)
  {
    index = 0U;
  }
  return index;
}

void PanView_RingBuffer_Init(PanViewRingBuffer *buffer)
{
  if (buffer == 0)
  {
    return;
  }

  buffer->write_index = 0U;
  buffer->read_index = 0U;
}

uint16_t PanView_RingBuffer_Write(PanViewRingBuffer *buffer,
                                  const uint8_t *data,
                                  uint16_t length)
{
  uint16_t written = 0U;
  uint16_t write_index;

  if ((buffer == 0) || (data == 0))
  {
    return 0U;
  }

  write_index = buffer->write_index;

  while (written < length)
  {
    uint16_t next_index = PanView_RingBuffer_NextIndex(write_index);

    /* 下一格已经是读位置，说明缓冲区没有剩余空间。 */
    if (next_index == buffer->read_index)
    {
      break;
    }

    buffer->data[write_index] = data[written];
    write_index = next_index;
    written++;
  }

  /* 数据全部复制完成后，再一次性公布新的写位置。 */
  buffer->write_index = write_index;
  return written;
}

uint16_t PanView_RingBuffer_Read(PanViewRingBuffer *buffer,
                                 uint8_t *output,
                                 uint16_t output_size)
{
  uint16_t read_count = 0U;
  uint16_t read_index;

  if ((buffer == 0) || (output == 0))
  {
    return 0U;
  }

  read_index = buffer->read_index;

  while ((read_count < output_size) && (read_index != buffer->write_index))
  {
    output[read_count] = buffer->data[read_index];
    read_index = PanView_RingBuffer_NextIndex(read_index);
    read_count++;
  }

  /* 数据全部复制完成后，再一次性公布新的读位置。 */
  buffer->read_index = read_index;
  return read_count;
}

uint16_t PanView_RingBuffer_GetUsed(const PanViewRingBuffer *buffer)
{
  uint16_t write_index;
  uint16_t read_index;

  if (buffer == 0)
  {
    return 0U;
  }

  write_index = buffer->write_index;
  read_index = buffer->read_index;

  if (write_index >= read_index)
  {
    return (uint16_t)(write_index - read_index);
  }

  return (uint16_t)(PANVIEW_RING_BUFFER_CAPACITY - read_index + write_index);
}
