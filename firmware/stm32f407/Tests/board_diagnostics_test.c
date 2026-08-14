#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../App/board_diagnostics.h"

static void test_init_keeps_led_off_and_counter_zero(void)
{
    BoardDiagnosticsState state;

    BoardDiagnostics_Init(&state);

    assert(state.status_led_on == false);
    assert(state.button_press_count == 0U);
}

static void test_each_key0_press_toggles_led_and_increments_counter(void)
{
    BoardDiagnosticsState state;

    BoardDiagnostics_Init(&state);
    BoardDiagnostics_OnKey0Pressed(&state);
    assert(state.status_led_on == true);
    assert(state.button_press_count == 1U);

    BoardDiagnostics_OnKey0Pressed(&state);
    assert(state.status_led_on == false);
    assert(state.button_press_count == 2U);
}

int main(void)
{
    test_init_keeps_led_off_and_counter_zero();
    test_each_key0_press_toggles_led_and_increments_counter();
    puts("board_diagnostics_test: PASS");
    return 0;
}
