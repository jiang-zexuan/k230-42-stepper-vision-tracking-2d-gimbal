#ifndef VISION_ERROR_H
#define VISION_ERROR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  bool target_present;
  int16_t error_x;
  int16_t error_y;
} VisionError;

/* 误差定义：目标中心坐标 - 画面中心坐标；图像坐标向右、向下为正。 */
VisionError VisionError_FromTarget(uint16_t image_width, uint16_t image_height,
                                   bool target_present, int16_t target_x,
                                   int16_t target_y);

#endif
