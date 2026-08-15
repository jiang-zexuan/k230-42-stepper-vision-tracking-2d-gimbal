#ifndef DEBOUNCED_BUTTON_H
#define DEBOUNCED_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

/*
 * 非阻塞按键消抖状态机。
 * previous_pressed 记录上一轮电平；press_pending 表示正在等待按下稳定；
 * press_started_ms 记录开始等待的时刻。
 */
typedef struct {
    bool previous_pressed;
    bool press_pending;
    uint32_t press_started_ms;
} DebouncedButton;

/* initial_pressed 应传入上电时读取到的按键状态，避免误报一次按下。 */
void DebouncedButton_Init(DebouncedButton *button, bool initial_pressed);

/*
 * 每次扫描调用一次。pressed 连续保持 debounce_ms 后只返回一次 true，
 * 松开前不会再次上报；所有时间参数单位均为 ms。
 */
bool DebouncedButton_Update(DebouncedButton *button, bool pressed,
                            uint32_t now_ms, uint32_t debounce_ms);

#endif
