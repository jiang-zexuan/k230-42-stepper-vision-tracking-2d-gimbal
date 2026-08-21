#include "motor_pulse_lab.h"

typedef struct
{
  uint16_t pulse_count;
  MotorPulseLabMotionPhase phase;
} MotorPulseLabProfileStage;

/* 4 * 80 + 160 + 4 * 80 = 800 脉冲，对应当前 90 度实验运动。 */
static const MotorPulseLabProfileStage motor_profile[] = {
    {80U, MOTOR_PULSE_LAB_ACCELERATING},
    {80U, MOTOR_PULSE_LAB_ACCELERATING},
    {80U, MOTOR_PULSE_LAB_ACCELERATING},
    {80U, MOTOR_PULSE_LAB_ACCELERATING},
    {160U, MOTOR_PULSE_LAB_CRUISING},
    {80U, MOTOR_PULSE_LAB_DECELERATING},
    {80U, MOTOR_PULSE_LAB_DECELERATING},
    {80U, MOTOR_PULSE_LAB_DECELERATING},
    {80U, MOTOR_PULSE_LAB_DECELERATING}};

enum
{
  MOTOR_PULSE_LAB_PROFILE_STAGE_COUNT =
      sizeof(motor_profile) / sizeof(motor_profile[0])
};

static void MotorPulseLab_LoadProfileStage(MotorPulseLab *lab,
                                            uint8_t stage_index)
{
  lab->profile_stage_index = stage_index;
  lab->stage_remaining_pulses = motor_profile[stage_index].pulse_count;
  lab->motion_phase = motor_profile[stage_index].phase;
}

void MotorPulseLab_Init(MotorPulseLab *lab)
{
  lab->state = MOTOR_PULSE_LAB_STOPPED;
  lab->direction = MOTOR_PULSE_LAB_DIRECTION_LOW;
  lab->remaining_pulses = 0U;
  lab->stage_remaining_pulses = 0U;
  lab->profile_stage_index = 0U;
  lab->motion_phase = MOTOR_PULSE_LAB_MOTION_IDLE;
}

MotorPulseLabState MotorPulseLab_Toggle(MotorPulseLab *lab)
{
  if (lab->state != MOTOR_PULSE_LAB_STOPPED)
  {
    lab->state = MOTOR_PULSE_LAB_STOPPED;
    lab->remaining_pulses = 0U;
    lab->stage_remaining_pulses = 0U;
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
  MotorPulseLab_LoadProfileStage(lab, 0U);
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
  lab->stage_remaining_pulses--;
  if (lab->remaining_pulses == 0U)
  {
    lab->state = MOTOR_PULSE_LAB_STOPPED;
    lab->motion_phase = MOTOR_PULSE_LAB_MOTION_IDLE;
    return true;
  }

  if (lab->stage_remaining_pulses == 0U)
  {
    MotorPulseLab_LoadProfileStage(lab, lab->profile_stage_index + 1U);
  }

  return false;
}

uint16_t MotorPulseLab_GetRemainingPulses(const MotorPulseLab *lab)
{
  return lab->remaining_pulses;
}

MotorPulseLabMotionPhase MotorPulseLab_GetMotionPhase(const MotorPulseLab *lab)
{
  return lab->motion_phase;
}

uint8_t MotorPulseLab_GetProfileStageIndex(const MotorPulseLab *lab)
{
  return lab->profile_stage_index;
}
