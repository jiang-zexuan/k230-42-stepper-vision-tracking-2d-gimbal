#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/motor_pulse_lab.h"

static void test_toggle_changes_between_stopped_and_running(void)
{
    MotorPulseLab lab;

    MotorPulseLab_Init(&lab);

    assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_STOPPED);
    assert(MotorPulseLab_Toggle(&lab) == MOTOR_PULSE_LAB_RUNNING);
    assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_RUNNING);
    assert(MotorPulseLab_Toggle(&lab) == MOTOR_PULSE_LAB_STOPPED);
    assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_STOPPED);
}

static void test_direction_changes_only_when_stopped(void)
{
    MotorPulseLab lab;

    MotorPulseLab_Init(&lab);

    assert(MotorPulseLab_GetDirection(&lab) == MOTOR_PULSE_LAB_DIRECTION_LOW);
    assert(MotorPulseLab_ToggleDirection(&lab));
    assert(MotorPulseLab_GetDirection(&lab) == MOTOR_PULSE_LAB_DIRECTION_HIGH);

    assert(MotorPulseLab_Toggle(&lab) == MOTOR_PULSE_LAB_RUNNING);
    assert(!MotorPulseLab_ToggleDirection(&lab));
    assert(MotorPulseLab_GetDirection(&lab) == MOTOR_PULSE_LAB_DIRECTION_HIGH);
}

static void test_profile_move_accelerates_and_decelerates_from_remaining_distance(void)
{
    MotorPulseLab lab;
    uint16_t pulse_index;

    MotorPulseLab_Init(&lab);

    assert(MotorPulseLab_StartProfileMove(&lab));
    assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_FIXED_MOVE);
    assert(MotorPulseLab_GetRemainingPulses(&lab) == 800U);
    assert(MotorPulseLab_GetStepFrequencyHz(&lab) == 80U);
    assert(MotorPulseLab_GetMotionPhase(&lab) == MOTOR_PULSE_LAB_ACCELERATING);

    for (pulse_index = 0U; pulse_index < 120U; pulse_index++)
    {
        assert(!MotorPulseLab_OnPulsePeriod(&lab));
    }

    assert(MotorPulseLab_GetMotionPhase(&lab) == MOTOR_PULSE_LAB_CRUISING);
    assert(MotorPulseLab_GetStepFrequencyHz(&lab) == 500U);

    for (pulse_index = 0U; pulse_index < 580U; pulse_index++)
    {
        assert(!MotorPulseLab_OnPulsePeriod(&lab));
    }

    assert(MotorPulseLab_GetMotionPhase(&lab) == MOTOR_PULSE_LAB_DECELERATING);
    assert(MotorPulseLab_GetStepFrequencyHz(&lab) < 500U);

    for (pulse_index = 0U; pulse_index < 99U; pulse_index++)
    {
        assert(!MotorPulseLab_OnPulsePeriod(&lab));
    }

    assert(MotorPulseLab_OnPulsePeriod(&lab));
    assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_STOPPED);
    assert(MotorPulseLab_GetRemainingPulses(&lab) == 0U);
}

int main(void)
{
    test_toggle_changes_between_stopped_and_running();
    test_direction_changes_only_when_stopped();
    test_profile_move_accelerates_and_decelerates_from_remaining_distance();
    puts("motor_pulse_lab_test: PASS");
    return 0;
}
