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

static void test_profile_move_advances_through_acceleration_and_deceleration(void)
{
    MotorPulseLab lab;
    uint16_t pulse_index;

    MotorPulseLab_Init(&lab);

    assert(MotorPulseLab_StartProfileMove(&lab));
    assert(MotorPulseLab_GetState(&lab) == MOTOR_PULSE_LAB_FIXED_MOVE);
    assert(MotorPulseLab_GetRemainingPulses(&lab) == 800U);
    assert(MotorPulseLab_GetMotionPhase(&lab) == MOTOR_PULSE_LAB_ACCELERATING);
    assert(MotorPulseLab_GetProfileStageIndex(&lab) == 0U);

    for (pulse_index = 0U; pulse_index < 320U; pulse_index++)
    {
        assert(!MotorPulseLab_OnPulsePeriod(&lab));
    }

    assert(MotorPulseLab_GetMotionPhase(&lab) == MOTOR_PULSE_LAB_CRUISING);
    assert(MotorPulseLab_GetProfileStageIndex(&lab) == 4U);

    for (pulse_index = 0U; pulse_index < 160U; pulse_index++)
    {
        assert(!MotorPulseLab_OnPulsePeriod(&lab));
    }

    assert(MotorPulseLab_GetMotionPhase(&lab) == MOTOR_PULSE_LAB_DECELERATING);
    assert(MotorPulseLab_GetProfileStageIndex(&lab) == 5U);

    for (pulse_index = 0U; pulse_index < 319U; pulse_index++)
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
    test_profile_move_advances_through_acceleration_and_deceleration();
    puts("motor_pulse_lab_test: PASS");
    return 0;
}
