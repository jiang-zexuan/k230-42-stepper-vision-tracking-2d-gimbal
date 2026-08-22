#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/vision_error.h"

static void test_target_at_image_center_has_zero_error(void)
{
  VisionError error = VisionError_FromTarget(1920U, 1080U, true, 960, 540);

  assert(error.target_present);
  assert(error.error_x == 0);
  assert(error.error_y == 0);
}

static void test_error_sign_follows_image_axes(void)
{
  VisionError error = VisionError_FromTarget(1920U, 1080U, true, 1060, 440);

  assert(error.error_x == 100);
  assert(error.error_y == -100);
}

static void test_missing_target_is_not_an_error_command(void)
{
  VisionError error = VisionError_FromTarget(1920U, 1080U, false, 100, 100);

  assert(!error.target_present);
  assert(error.error_x == 0);
  assert(error.error_y == 0);
}

int main(void)
{
  test_target_at_image_center_has_zero_error();
  test_error_sign_follows_image_axes();
  test_missing_target_is_not_an_error_command();
  puts("vision_error_test: PASS");
  return 0;
}
