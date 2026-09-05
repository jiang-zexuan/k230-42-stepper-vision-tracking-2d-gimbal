#include "panview_pv04_parser.h"

#include <stdio.h>
#include <string.h>

uint8_t PanView_Pv04_Parse(const char *line, VisionResult *result)
{
  unsigned long timestamp;
  unsigned int target;
  unsigned int count;
  int x, y, width, height;
  int center_x, center_x_fraction;
  int center_y, center_y_fraction;
  int fps_integer, fps_fraction;
  char confidence[32];
  int fields;

  if ((line == 0) || (result == 0))
  {
    return 0U;
  }

  /* 先解析所有帧都共有的三个字段。 */
  fields = sscanf(line, "PV04 t_ms=%lu target=%u count=%u",
                  &timestamp, &target, &count);
  if ((fields != 3) || (target > 1U))
  {
    return 0U;
  }

  memset(result, 0, sizeof(*result));
  result->source_timestamp_ms = (uint32_t)timestamp;
  result->target_count = (uint16_t)count;
  result->target_present = (uint8_t)target;
  result->frame_width_px = 1920U;
  result->frame_height_px = 1080U;

  /* target=0 的帧没有坐标，公共字段已经足够。 */
  if (target == 0U)
  {
    return 1U;
  }

  /* 有目标时，必须同时具备框坐标、中心坐标和帧率。 */
  fields = sscanf(line,
                  "PV04 t_ms=%lu target=%u count=%u x=%d y=%d w=%d h=%d "
                  "cx=%d.%d cy=%d.%d confidence=%31s fps=%d.%d",
                  &timestamp, &target, &count, &x, &y, &width, &height,
                  &center_x, &center_x_fraction,
                  &center_y, &center_y_fraction,
                  confidence, &fps_integer, &fps_fraction);
  /* timestamp、target、count、x、y、w、h、cx整数、cx小数、
   * cy整数、cy小数、confidence、fps整数、fps小数，共 14 项。 */
  if (fields != 14)
  {
    return 0U;
  }

  result->target_x_px = (int16_t)x;
  result->target_y_px = (int16_t)y;
  result->target_width_px = (uint16_t)width;
  result->target_height_px = (uint16_t)height;
  result->center_x_px = (float)center_x;
  result->center_y_px = (float)center_y;
  /* 当前先不把文本帧率转换为浮点，避免依赖精简库的 scanf 浮点支持。 */
  (void)center_x_fraction;
  (void)center_y_fraction;
  (void)fps_integer;
  (void)fps_fraction;
  result->source_fps = 0.0f;

  /* 当前协议可能发送 confidence=unavailable，因此暂不转换它。 */
  result->confidence = 0.0f;
  return 1U;
}
