#include "vision_text_result_parser.h"

#include <stdio.h>
#include <string.h>

enum
{
  VISION_TEXT_RESULT_MAX_LINE_SIZE = 255U
};

bool VisionTextResult_Parse(const uint8_t *data, uint16_t size,
                            VisionTextResult *result)
{
  char line[VISION_TEXT_RESULT_MAX_LINE_SIZE + 1U];
  unsigned long timestamp_ms;
  unsigned int target;
  unsigned int count;
  int x;
  int y;
  int width;
  int height;
  float center_x;
  float center_y;
  float fps;
  char confidence[32];
  int parsed_fields;

  if ((data == NULL) || (result == NULL) || (size == 0U) ||
      (size > VISION_TEXT_RESULT_MAX_LINE_SIZE))
  {
    return false;
  }

  memcpy(line, data, size);
  line[size] = '\0';

  parsed_fields = sscanf(line, "PV04 t_ms=%lu target=%u count=%u",
                         &timestamp_ms, &target, &count);
  if ((parsed_fields != 3) || (target > 1U))
  {
    return false;
  }

  result->timestamp_ms = (uint32_t)timestamp_ms;
  result->target_count = (uint16_t)count;
  result->target_present = target == 1U;
  result->center_x = 0;
  result->center_y = 0;

  if (!result->target_present)
  {
    return true;
  }

  parsed_fields = sscanf(
      line,
      "PV04 t_ms=%lu target=%u count=%u x=%d y=%d w=%d h=%d cx=%f cy=%f confidence=%31s fps=%f",
      &timestamp_ms, &target, &count, &x, &y, &width, &height, &center_x,
      &center_y, confidence, &fps);
  if (parsed_fields != 11)
  {
    return false;
  }

  result->center_x = (int16_t)center_x;
  result->center_y = (int16_t)center_y;
  return true;
}
