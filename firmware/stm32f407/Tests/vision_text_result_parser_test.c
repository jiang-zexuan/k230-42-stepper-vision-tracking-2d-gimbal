#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/vision_text_result_parser.h"

static void test_parse_accepts_a_detected_target(void)
{
  const uint8_t line[] =
      "PV04 t_ms=1234 target=1 count=2 x=720 y=180 w=300 h=300 cx=870.0 cy=330.0 confidence=unavailable fps=24.6\r\n";
  VisionTextResult result;

  assert(VisionTextResult_Parse(line, sizeof(line) - 1U, &result));
  assert(result.timestamp_ms == 1234U);
  assert(result.target_count == 2U);
  assert(result.target_present);
  assert(result.center_x == 870);
  assert(result.center_y == 330);
}

static void test_parse_accepts_an_empty_target_result(void)
{
  const uint8_t line[] =
      "PV04 t_ms=1300 target=0 count=0 confidence=unavailable fps=25.0\r\n";
  VisionTextResult result;

  assert(VisionTextResult_Parse(line, sizeof(line) - 1U, &result));
  assert(result.timestamp_ms == 1300U);
  assert(!result.target_present);
  assert(result.target_count == 0U);
}

static void test_parse_rejects_an_incomplete_target_line(void)
{
  const uint8_t line[] = "PV04 t_ms=1 target=1 count=1\r\n";
  VisionTextResult result;

  assert(!VisionTextResult_Parse(line, sizeof(line) - 1U, &result));
}

int main(void)
{
  test_parse_accepts_a_detected_target();
  test_parse_accepts_an_empty_target_result();
  test_parse_rejects_an_incomplete_target_line();
  puts("vision_text_result_parser_test: PASS");
  return 0;
}
