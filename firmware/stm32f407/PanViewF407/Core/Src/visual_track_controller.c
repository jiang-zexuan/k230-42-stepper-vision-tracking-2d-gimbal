#include "visual_track_controller.h"

static float absolute_float(float value)
{
  return value < 0.0f ? -value : value;
}

static float clamp_float(float value, float minimum, float maximum)
{
  if (value < minimum)
  {
    return minimum;
  }
  if (value > maximum)
  {
    return maximum;
  }
  return value;
}

void VisualTrackController_Reset(VisualTrackControllerState *state)
{
  if (state == 0)
  {
    return;
  }

  state->integral = 0.0f;
  state->derivative = 0.0f;
  state->previous_error = 0;
  state->previous_tick_ms = 0U;
  state->previous_output = 0;
  state->previous_valid = false;
}

static int32_t slew_output(int32_t previous_output, float desired_output,
                           float dt_seconds, int32_t max_acceleration)
{
  float maximum_delta;
  float delta;

  if ((dt_seconds <= 0.0f) || (max_acceleration <= 0))
  {
    return (int32_t)desired_output;
  }

  maximum_delta = (float)max_acceleration * dt_seconds;
  delta = desired_output - (float)previous_output;
  if (delta > maximum_delta)
  {
    delta = maximum_delta;
  }
  else if (delta < -maximum_delta)
  {
    delta = -maximum_delta;
  }
  return previous_output + (int32_t)delta;
}

int32_t VisualTrackController_Update(
    const VisualTrackControllerConfig *config,
    VisualTrackControllerState *state, bool target_present, int16_t error,
    uint32_t tick_ms)
{
  float effective_error;
  float derivative_raw = 0.0f;
  float integral_candidate;
  float control;
  float output;
  float maximum;
  float dt_seconds = 0.0f;

  if ((config == 0) || (state == 0) || !target_present ||
      (config->deadzone_pixels < 0) || (config->kp < 0.0f) ||
      (config->ki < 0.0f) || (config->kd < 0.0f) ||
      (config->integral_limit < 0.0f) ||
      (config->max_speed_pulses_per_second <= 0) ||
      (config->max_acceleration_pulses_per_second2 <= 0))
  {
    VisualTrackController_Reset(state);
    return 0;
  }

  effective_error = (float)error;
  if (state->previous_valid)
  {
    uint32_t elapsed_ms = tick_ms - state->previous_tick_ms;
    if (elapsed_ms > 0U)
    {
      dt_seconds = (float)elapsed_ms / 1000.0f;
      derivative_raw =
          ((float)error - (float)state->previous_error) / dt_seconds;
    }
  }

  /* 中心区域沿加速度限制减速到 0，避免到位后因机械惯性反复启停。 */
  if ((error <= config->deadzone_pixels) &&
      (error >= -config->deadzone_pixels))
  {
    state->integral = 0.0f;
    state->derivative = 0.0f;
    state->previous_error = error;
    state->previous_tick_ms = tick_ms;
    state->previous_valid = true;
    state->previous_output = slew_output(
        state->previous_output, 0.0f, dt_seconds,
        config->max_acceleration_pulses_per_second2);
    return state->previous_output;
  }

  /* 低通微分项，降低视觉检测抖动和不均匀帧周期带来的尖峰。 */
  state->derivative = (state->previous_valid)
                          ? (0.35f * derivative_raw +
                             0.65f * state->derivative)
                          : 0.0f;

  integral_candidate = state->integral;
  if (dt_seconds > 0.0f)
  {
    integral_candidate += effective_error * dt_seconds;
  }
  integral_candidate = clamp_float(integral_candidate,
                                   -config->integral_limit,
                                   config->integral_limit);

  control = (config->kp * effective_error) +
            (config->ki * integral_candidate) +
            (config->kd * state->derivative);
  maximum = (float)config->max_speed_pulses_per_second;
  output = clamp_float(-control, -maximum, maximum);

  /* 输出饱和且误差仍推动同一方向时冻结积分，避免解除饱和后猛冲。 */
  if ((absolute_float(control) > maximum) &&
      (((control > 0.0f) && (effective_error > 0.0f)) ||
       ((control < 0.0f) && (effective_error < 0.0f))))
  {
    control = (config->kp * effective_error) +
              (config->ki * state->integral) +
              (config->kd * state->derivative);
    output = clamp_float(-control, -maximum, maximum);
  }
  else
  {
    state->integral = integral_candidate;
  }

  state->previous_error = error;
  state->previous_tick_ms = tick_ms;
  state->previous_valid = true;
  state->previous_output = slew_output(
      state->previous_output, output, dt_seconds,
      config->max_acceleration_pulses_per_second2);
  return state->previous_output;
}
