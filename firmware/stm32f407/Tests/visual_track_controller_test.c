#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/visual_track_controller.h"

static const VisualTrackControllerConfig config = {
    40, 2, 800};

static void test_deadzone_stops_small_error(void)
{
  assert(VisualTrackController_HorizontalSpeed(&config, true, 40) == 0);
  assert(VisualTrackController_HorizontalSpeed(&config, true, -20) == 0);
}

static void test_error_sign_is_preserved(void)
{
  /* 图像右侧为 err_x 正，但当前云台右侧对应水平负方向。 */
  assert(VisualTrackController_HorizontalSpeed(&config, true, 100) == -120);
  assert(VisualTrackController_HorizontalSpeed(&config, true, -100) == 120);
}

static void test_speed_is_limited_and_missing_target_stops(void)
{
  assert(VisualTrackController_HorizontalSpeed(&config, true, 1000) == -800);
  assert(VisualTrackController_HorizontalSpeed(&config, false, 1000) == 0);
}

static void test_vertical_error_maps_up_and_down(void)
{
  /* 图像上方为 err_y 负，俯仰向上定义为正速度。 */
  assert(VisualTrackController_VerticalSpeed(&config, true, -100) == 120);
  assert(VisualTrackController_VerticalSpeed(&config, true, 100) == -120);
  assert(VisualTrackController_VerticalSpeed(&config, true, -40) == 0);
}

int main(void)
{
  test_deadzone_stops_small_error();
  test_error_sign_is_preserved();
  test_speed_is_limited_and_missing_target_stops();
  test_vertical_error_maps_up_and_down();
  puts("visual_track_controller_test: PASS");
  return 0;
}
