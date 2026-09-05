#include <assert.h>
#include <stdint.h>

#include "../App/Inc/panview_ring_buffer.h"

static void Test_WriteAndRead(void)
{
  PanViewRingBuffer buffer;
  const uint8_t input[] = {10U, 20U, 30U, 40U};
  uint8_t output[4] = {0U};

  PanView_RingBuffer_Init(&buffer);

  assert(PanView_RingBuffer_GetUsed(&buffer) == 0U);
  assert(PanView_RingBuffer_Write(&buffer, input, sizeof(input)) == sizeof(input));
  assert(PanView_RingBuffer_GetUsed(&buffer) == sizeof(input));
  assert(PanView_RingBuffer_Read(&buffer, output, sizeof(output)) == sizeof(output));

  for (uint16_t index = 0U; index < sizeof(input); index++)
  {
    assert(output[index] == input[index]);
  }
}

static void Test_WrapAround(void)
{
  PanViewRingBuffer buffer;
  uint8_t first_input[900];
  uint8_t second_input[300];
  uint8_t discard[800];
  uint8_t output[400];

  for (uint16_t index = 0U; index < sizeof(first_input); index++)
  {
    first_input[index] = (uint8_t)(index % 251U);
  }

  for (uint16_t index = 0U; index < sizeof(second_input); index++)
  {
    second_input[index] = (uint8_t)((index + 17U) % 251U);
  }

  PanView_RingBuffer_Init(&buffer);
  assert(PanView_RingBuffer_Write(&buffer, first_input, sizeof(first_input)) == sizeof(first_input));
  assert(PanView_RingBuffer_Read(&buffer, discard, sizeof(discard)) == sizeof(discard));
  assert(PanView_RingBuffer_Write(&buffer, second_input, sizeof(second_input)) == sizeof(second_input));
  assert(PanView_RingBuffer_Read(&buffer, output, sizeof(output)) == sizeof(output));

  for (uint16_t index = 0U; index < 100U; index++)
  {
    assert(output[index] == first_input[index + 800U]);
  }

  for (uint16_t index = 0U; index < sizeof(second_input); index++)
  {
    assert(output[index + 100U] == second_input[index]);
  }
}

static void Test_FullBufferDoesNotOverwriteUnreadData(void)
{
  PanViewRingBuffer buffer;
  uint8_t input[1100];
  uint8_t output[PANVIEW_RING_BUFFER_CAPACITY];

  for (uint16_t index = 0U; index < sizeof(input); index++)
  {
    input[index] = (uint8_t)(index % 251U);
  }

  PanView_RingBuffer_Init(&buffer);
  assert(PanView_RingBuffer_Write(&buffer, input, sizeof(input)) ==
         (PANVIEW_RING_BUFFER_CAPACITY - 1U));
  assert(PanView_RingBuffer_GetUsed(&buffer) ==
         (PANVIEW_RING_BUFFER_CAPACITY - 1U));
  assert(PanView_RingBuffer_Read(&buffer, output, sizeof(output)) ==
         (PANVIEW_RING_BUFFER_CAPACITY - 1U));

  for (uint16_t index = 0U; index < (PANVIEW_RING_BUFFER_CAPACITY - 1U); index++)
  {
    assert(output[index] == input[index]);
  }
}

int main(void)
{
  Test_WriteAndRead();
  Test_WrapAround();
  Test_FullBufferDoesNotOverwriteUnreadData();
  return 0;
}
