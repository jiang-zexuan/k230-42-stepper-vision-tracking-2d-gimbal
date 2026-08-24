#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/visual_track_controller.h"

static const VisualTrackControllerConfig config = {
    20, 4.0f, 0.20f, 0.05f, 300.0f, 800, 5000};
static VisualTrackControllerState state;

static void test_deadzone_stops_small_error(void)
{
  VisualTrackController_Reset(&state);
  assert(VisualTrackController_Update(&config, &state, true, 20, 0U) == 0);
  assert(state.previous_valid);
  assert(VisualTrackController_Update(&config, &state, true, -20, 20U) == 0);
  assert(state.previous_valid);
}

static void test_error_sign_is_preserved(void)
{
  /* 图像右侧为 err_x 正，但当前云台右侧对应水平负方向。 */
  VisualTrackController_Reset(&state);
  assert(VisualTrackController_Update(&config, &state, true, 100, 0U) == -400);
  VisualTrackController_Reset(&state);
  assert(VisualTrackController_Update(&config, &state, true, -100, 0U) == 400);
}

static void test_speed_is_limited_and_missing_target_stops(void)
{
  VisualTrackController_Reset(&state);
  assert(VisualTrackController_Update(&config, &state, true, 1000, 0U) == -800);
  assert(VisualTrackController_Update(&config, &state, false, 1000, 100U) == 0);
  assert(!state.previous_valid);
}

static void test_vertical_error_maps_up_and_down(void)
{
  /* 图像上方为 err_y 负，俯仰向上定义为正速度。 */
  VisualTrackController_Reset(&state);
  assert(VisualTrackController_Update(&config, &state, true, -100, 0U) == 400);
  assert(VisualTrackController_Update(&config, &state, true, -150, 100U) > 400);
  assert(VisualTrackController_Update(&config, &state, true, -20, 200U) < 400);
  assert(VisualTrackController_Update(&config, &state, true, -20, 1000U) == 0);
}

static void test_integral_is_limited(void)
{
  uint32_t tick;

  VisualTrackController_Reset(&state);
  (void)VisualTrackController_Update(&config, &state, true, 100, 0U);
  for (tick = 100U; tick <= 10000U; tick += 100U)
  {
    (void)VisualTrackController_Update(&config, &state, true, 100, tick);
  }
  assert(state.integral <= config.integral_limit);
  assert(state.integral >= -config.integral_limit);
}

int main(void)
{
  test_deadzone_stops_small_error();
  test_error_sign_is_preserved();
  test_speed_is_limited_and_missing_target_stops();
  test_vertical_error_maps_up_and_down();
  test_integral_is_limited();
  puts("visual_track_controller_test: PASS");
  return 0;
}
