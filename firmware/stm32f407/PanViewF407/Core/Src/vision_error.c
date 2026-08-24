#include "vision_error.h"

VisionError VisionError_FromTarget(uint16_t image_width, uint16_t image_height,
                                   bool target_present, int16_t target_x,
                                   int16_t target_y)
{
  VisionError error;
  int32_t center_x;
  int32_t center_y;

  error.target_present = target_present;
  if ((!target_present) || (image_width == 0U) || (image_height == 0U))
  {
    error.error_x = 0;
    error.error_y = 0;
    return error;
  }

  center_x = (int32_t)image_width / 2;
  center_y = (int32_t)image_height / 2;
  error.error_x = (int16_t)((int32_t)target_x - center_x);
  error.error_y = (int16_t)((int32_t)target_y - center_y);
  return error;
}
