#ifndef PANVIEW_BOARD_DIAGNOSTICS_H
#define PANVIEW_BOARD_DIAGNOSTICS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool status_led_on;
    uint32_t button_press_count;
} BoardDiagnosticsState;

void BoardDiagnostics_Init(BoardDiagnosticsState *state);
void BoardDiagnostics_OnKey0Pressed(BoardDiagnosticsState *state);

#endif
