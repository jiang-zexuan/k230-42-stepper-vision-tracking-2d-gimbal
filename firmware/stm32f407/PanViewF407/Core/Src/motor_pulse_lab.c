#include "motor_pulse_lab.h"

enum
{
  /* 单位：脉冲/秒；依据 P06 当前低速实测配置，待后续联合机械负载调参。 */
  MOTOR_PROFILE_MIN_SPEED_HZ = 80U,
  /* 单位：脉冲/秒；当前实验允许的最高 STEP 频率。 */
  MOTOR_PROFILE_MAX_SPEED_HZ = 500U,
  /* 单位：脉冲/秒^2；每走一脉冲按 v^2 = v0^2 + 2as 更新速度。 */
  MOTOR_PROFILE_ACCELERATION_PULSES_PER_SECOND_SQUARED = 1200U
};

static uint16_t MotorPulseLab_IntegerSqrt(uint32_t value)
{
  uint32_t candidate = 0U;
  uint32_t bit = 1UL << 30;

  while (bit > value)
  {
    bit >>= 2U;
  }

  while (bit != 0U)
  {
    if (value >= candidate + bit)
    {
      value -= candidate + bit;
      candidate = (candidate >> 1U) + bit;
    }
    else
    {
      candidate >>= 1U;
    }

    bit >>= 2U;
  }

  return (uint16_t)candidate;
}

static uint16_t MotorPulseLab_AcceleratedSpeed(uint16_t speed_hz)
{
  uint32_t squared_speed = (uint32_t)speed_hz * speed_hz;
  uint32_t next_squared_speed =
      squared_speed + 2U * MOTOR_PROFILE_ACCELERATION_PULSES_PER_SECOND_SQUARED;
  uint16_t next_speed_hz = MotorPulseLab_IntegerSqrt(next_squared_speed);

  return next_speed_hz > MOTOR_PROFILE_MAX_SPEED_HZ
             ? MOTOR_PROFILE_MAX_SPEED_HZ
             : next_speed_hz;
}

static uint16_t MotorPulseLab_DeceleratedSpeed(uint16_t speed_hz)
{
  uint32_t squared_speed = (uint32_t)speed_hz * speed_hz;
  uint32_t minimum_squared_speed =
      (uint32_t)MOTOR_PROFILE_MIN_SPEED_HZ * MOTOR_PROFILE_MIN_SPEED_HZ;
  uint32_t speed_delta_squared =
      2U * MOTOR_PROFILE_ACCELERATION_PULSES_PER_SECOND_SQUARED;

  if (squared_speed <= minimum_squared_speed + speed_delta_squared)
  {
    return MOTOR_PROFILE_MIN_SPEED_HZ;
  }

  return MotorPulseLab_IntegerSqrt(squared_speed - speed_delta_squared);
}

static uint16_t MotorPulseLab_MaxSpeedForRemainingDistance(uint16_t remaining_pulses)
{
  uint32_t minimum_squared_speed =
      (uint32_t)MOTOR_PROFILE_MIN_SPEED_HZ * MOTOR_PROFILE_MIN_SPEED_HZ;
  uint32_t stopping_speed_squared =
      minimum_squared_speed +
      2U * MOTOR_PROFILE_ACCELERATION_PULSES_PER_SECOND_SQUARED *
          remaining_pulses;
  uint16_t stopping_speed_hz =
      MotorPulseLab_IntegerSqrt(stopping_speed_squared);

  return stopping_speed_hz > MOTOR_PROFILE_MAX_SPEED_HZ
             ? MOTOR_PROFILE_MAX_SPEED_HZ
             : stopping_speed_hz;
}

static void MotorPulseLab_UpdateProfileSpeed(MotorPulseLab *lab)
{
  uint16_t braking_speed_hz =
      MotorPulseLab_MaxSpeedForRemainingDistance(lab->remaining_pulses);

  if (lab->step_frequency_hz < braking_speed_hz)
  {
    lab->step_frequency_hz = MotorPulseLab_AcceleratedSpeed(
        lab->step_frequency_hz);
    lab->motion_phase = lab->step_frequency_hz == MOTOR_PROFILE_MAX_SPEED_HZ
                            ? MOTOR_PULSE_LAB_CRUISING
                            : MOTOR_PULSE_LAB_ACCELERATING;
  }
  else if (lab->step_frequency_hz > braking_speed_hz)
  {
    lab->step_frequency_hz = MotorPulseLab_DeceleratedSpeed(
        lab->step_frequency_hz);
    lab->motion_phase = MOTOR_PULSE_LAB_DECELERATING;
  }
  else
  {
    lab->motion_phase = lab->step_frequency_hz == MOTOR_PROFILE_MAX_SPEED_HZ
                            ? MOTOR_PULSE_LAB_CRUISING
                            : MOTOR_PULSE_LAB_DECELERATING;
  }
}

void MotorPulseLab_Init(MotorPulseLab *lab)
{
  lab->state = MOTOR_PULSE_LAB_STOPPED;
  lab->direction = MOTOR_PULSE_LAB_DIRECTION_LOW;
  lab->remaining_pulses = 0U;
  lab->step_frequency_hz = 0U;
  lab->motion_phase = MOTOR_PULSE_LAB_MOTION_IDLE;
}

MotorPulseLabState MotorPulseLab_Toggle(MotorPulseLab *lab)
{
  if (lab->state != MOTOR_PULSE_LAB_STOPPED)
  {
    lab->state = MOTOR_PULSE_LAB_STOPPED;
    lab->remaining_pulses = 0U;
    lab->step_frequency_hz = 0U;
    lab->motion_phase = MOTOR_PULSE_LAB_MOTION_IDLE;
  }
  else
  {
    lab->state = MOTOR_PULSE_LAB_RUNNING;
  }

  return lab->state;
}

MotorPulseLabState MotorPulseLab_GetState(const MotorPulseLab *lab)
{
  return lab->state;
}

bool MotorPulseLab_ToggleDirection(MotorPulseLab *lab)
{
  if (lab->state != MOTOR_PULSE_LAB_STOPPED)
  {
    return false;
  }

  if (lab->direction == MOTOR_PULSE_LAB_DIRECTION_LOW)
  {
    lab->direction = MOTOR_PULSE_LAB_DIRECTION_HIGH;
  }
  else
  {
    lab->direction = MOTOR_PULSE_LAB_DIRECTION_LOW;
  }

  return true;
}

MotorPulseLabDirection MotorPulseLab_GetDirection(const MotorPulseLab *lab)
{
  return lab->direction;
}

bool MotorPulseLab_StartProfileMove(MotorPulseLab *lab)
{
  if (lab->state != MOTOR_PULSE_LAB_STOPPED)
  {
    return false;
  }

  lab->state = MOTOR_PULSE_LAB_FIXED_MOVE;
  lab->remaining_pulses = MOTOR_PULSE_LAB_PROFILE_TOTAL_PULSES;
  lab->step_frequency_hz = MOTOR_PROFILE_MIN_SPEED_HZ;
  lab->motion_phase = MOTOR_PULSE_LAB_ACCELERATING;
  return true;
}

bool MotorPulseLab_OnPulsePeriod(MotorPulseLab *lab)
{
  if ((lab->state != MOTOR_PULSE_LAB_FIXED_MOVE) ||
      (lab->remaining_pulses == 0U))
  {
    return false;
  }

  lab->remaining_pulses--;
  if (lab->remaining_pulses == 0U)
  {
    lab->state = MOTOR_PULSE_LAB_STOPPED;
    lab->step_frequency_hz = 0U;
    lab->motion_phase = MOTOR_PULSE_LAB_MOTION_IDLE;
    return true;
  }

  MotorPulseLab_UpdateProfileSpeed(lab);

  return false;
}

uint16_t MotorPulseLab_GetRemainingPulses(const MotorPulseLab *lab)
{
  return lab->remaining_pulses;
}

uint16_t MotorPulseLab_GetStepFrequencyHz(const MotorPulseLab *lab)
{
  return lab->step_frequency_hz;
}

MotorPulseLabMotionPhase MotorPulseLab_GetMotionPhase(const MotorPulseLab *lab)
{
  return lab->motion_phase;
}
