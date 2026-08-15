#include "periodic_task.h"

void PeriodicTask_Init(PeriodicTask *task, uint32_t period_ms, uint32_t start_ms)
{
    task->period_ms = period_ms;
    task->last_run_ms = start_ms;
}

bool PeriodicTask_IsDue(PeriodicTask *task, uint32_t now_ms)
{
    /* 无符号减法可正确处理 HAL_GetTick() 的 32 位回绕。 */
    if ((now_ms - task->last_run_ms) < task->period_ms) {
        return false;
    }

    /* 以本次执行时刻为下一周期的起点，避免一次调用重复触发。 */
    task->last_run_ms = now_ms;
    return true;
}
