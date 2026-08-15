#include "debounced_button.h"

void DebouncedButton_Init(DebouncedButton *button, bool initial_pressed)
{
    button->previous_pressed = initial_pressed;
    button->press_pending = false;
    button->press_started_ms = 0U;
}

bool DebouncedButton_Update(DebouncedButton *button, bool pressed,
                            uint32_t now_ms, uint32_t debounce_ms)
{
    if (!pressed) {
        button->previous_pressed = false;
        button->press_pending = false;
        return false;
    }

    if (!button->previous_pressed) {
        button->press_pending = true;
        button->press_started_ms = now_ms;
    }

    button->previous_pressed = true;

    if (!button->press_pending ||
        ((now_ms - button->press_started_ms) < debounce_ms)) {
        return false;
    }

    button->press_pending = false;
    return true;
}
