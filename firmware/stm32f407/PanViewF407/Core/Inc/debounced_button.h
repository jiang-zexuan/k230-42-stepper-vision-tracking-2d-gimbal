#ifndef DEBOUNCED_BUTTON_H
#define DEBOUNCED_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool previous_pressed;
    bool press_pending;
    uint32_t press_started_ms;
} DebouncedButton;

void DebouncedButton_Init(DebouncedButton *button, bool initial_pressed);
bool DebouncedButton_Update(DebouncedButton *button, bool pressed,
                            uint32_t now_ms, uint32_t debounce_ms);

#endif
