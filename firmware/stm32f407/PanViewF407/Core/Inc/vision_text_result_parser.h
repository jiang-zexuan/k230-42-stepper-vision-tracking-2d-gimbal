#ifndef VISION_TEXT_RESULT_PARSER_H
#define VISION_TEXT_RESULT_PARSER_H

#include <stdbool.h>
#include <stdint.h>

/* P08 过渡协议：仅解析当前 K230 P04 发布器输出的单行 ASCII 文本。 */
typedef struct
{
  uint32_t timestamp_ms;
  uint16_t target_count;
  bool target_present;
  int16_t center_x;
  int16_t center_y;
} VisionTextResult;

/* 成功时返回 true；目标不存在同样是有效视觉结果。 */
bool VisionTextResult_Parse(const uint8_t *data, uint16_t size,
                            VisionTextResult *result);

#endif
