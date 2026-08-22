#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../PanViewF407/Core/Inc/uart_text_line_accumulator.h"

static void test_fragments_are_joined_until_newline(void)
{
  UartTextLineAccumulator accumulator;
  UartRxFrame mailbox;
  const uint8_t first[] = "PV04 t_ms=1 target=1";
  const uint8_t second[] = " count=1\r\n";

  UartTextLineAccumulator_Init(&accumulator);
  UartRxFrame_Init(&mailbox);

  assert(UartTextLineAccumulator_Consume(&accumulator, &mailbox, first,
                                         sizeof(first) - 1U) == 0U);
  assert(!mailbox.pending);
  assert(UartTextLineAccumulator_Consume(&accumulator, &mailbox, second,
                                         sizeof(second) - 1U) == 1U);
  assert(mailbox.pending);
  assert(mailbox.size == strlen("PV04 t_ms=1 target=1 count=1"));
  assert(memcmp(mailbox.data, "PV04 t_ms=1 target=1 count=1",
                mailbox.size) == 0);
}

static void test_crlf_and_lf_are_both_accepted(void)
{
  UartTextLineAccumulator accumulator;
  UartRxFrame mailbox;
  const uint8_t input[] = "A\r\nB\n";

  UartTextLineAccumulator_Init(&accumulator);
  UartRxFrame_Init(&mailbox);

  assert(UartTextLineAccumulator_Consume(&accumulator, &mailbox, input,
                                         sizeof(input) - 1U) == 1U);
  assert(mailbox.size == 1U);
  assert(mailbox.data[0] == 'A');
  assert(mailbox.dropped_count == 1U);
}

int main(void)
{
  test_fragments_are_joined_until_newline();
  test_crlf_and_lf_are_both_accepted();
  puts("uart_text_line_accumulator_test: PASS");
  return 0;
}
