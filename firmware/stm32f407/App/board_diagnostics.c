#include "board_diagnostics.h"

void BoardDiagnostics_Init(BoardDiagnosticsState *state)
{
    state->status_led_on = false;
    state->button_press_count = 0U;
}

void BoardDiagnostics_OnKey0Pressed(BoardDiagnosticsState *state)
{
    state->status_led_on = !state->status_led_on;
    state->button_press_count++;
}
