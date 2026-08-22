#ifndef MOTOR_PULSE_LAB_H
#define MOTOR_PULSE_LAB_H

#include <stdbool.h>
#include <stdint.h>

#define MOTOR_PULSE_LAB_PROFILE_TOTAL_PULSES 800U

typedef enum
{
  MOTOR_PULSE_LAB_STOPPED = 0,
  MOTOR_PULSE_LAB_RUNNING,
  MOTOR_PULSE_LAB_FIXED_MOVE
} MotorPulseLabState;

typedef enum
{
  /* DIR 逻辑电平；实际轴向顺逆时针由本课实测确认。 */
  MOTOR_PULSE_LAB_DIRECTION_LOW = 0,
  MOTOR_PULSE_LAB_DIRECTION_HIGH
} MotorPulseLabDirection;

typedef enum
{
  MOTOR_PULSE_LAB_MOTION_IDLE = 0,
  MOTOR_PULSE_LAB_ACCELERATING,
  MOTOR_PULSE_LAB_CRUISING,
  MOTOR_PULSE_LAB_DECELERATING
} MotorPulseLabMotionPhase;

typedef struct
{
  MotorPulseLabState state;
  MotorPulseLabDirection direction;
  uint16_t remaining_pulses;
  uint16_t step_frequency_hz;
  MotorPulseLabMotionPhase motion_phase;
} MotorPulseLab;

void MotorPulseLab_Init(MotorPulseLab *lab);
MotorPulseLabState MotorPulseLab_Toggle(MotorPulseLab *lab);
MotorPulseLabState MotorPulseLab_GetState(const MotorPulseLab *lab);
bool MotorPulseLab_ToggleDirection(MotorPulseLab *lab);
MotorPulseLabDirection MotorPulseLab_GetDirection(const MotorPulseLab *lab);
bool MotorPulseLab_StartProfileMove(MotorPulseLab *lab);
bool MotorPulseLab_OnPulsePeriod(MotorPulseLab *lab);
uint16_t MotorPulseLab_GetRemainingPulses(const MotorPulseLab *lab);
uint16_t MotorPulseLab_GetStepFrequencyHz(const MotorPulseLab *lab);
MotorPulseLabMotionPhase MotorPulseLab_GetMotionPhase(const MotorPulseLab *lab);

#endif
