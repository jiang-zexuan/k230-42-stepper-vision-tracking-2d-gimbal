#include "periodic_task.h"

void PeriodicTask_Init(PeriodicTask *task, uint32_t period_ms, uint32_t start_ms)
{
    task->period_ms = period_ms;
    task->last_run_ms = start_ms;
}

bool PeriodicTask_IsDue(PeriodicTask *task, uint32_t now_ms)
{
    if ((now_ms - task->last_run_ms) < task->period_ms) {
        return false;
    }

    task->last_run_ms = now_ms;
    return true;
}
