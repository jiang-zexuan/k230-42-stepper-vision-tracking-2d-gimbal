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
        /* 松开即回到初始状态，下一次按下可以重新进入等待稳定阶段。 */
        button->previous_pressed = false;
        button->press_pending = false;
        return false;
    }

    if (!button->previous_pressed) {
        /* 检测到按下沿：开始计时，但不在这里阻塞等待。 */
        button->press_pending = true;
        button->press_started_ms = now_ms;
    }

    button->previous_pressed = true;

    /* 按下时间不足消抖阈值时，主循环可继续运行其他任务。 */
    if (!button->press_pending ||
        ((now_ms - button->press_started_ms) < debounce_ms)) {
        return false;
    }

    /* 确认本次按下，只上报一次；保持按下不会重复触发。 */
    button->press_pending = false;
    return true;
}
