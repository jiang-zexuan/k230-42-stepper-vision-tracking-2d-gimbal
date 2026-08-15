#ifndef PERIODIC_TASK_H
#define PERIODIC_TASK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t period_ms;
    uint32_t last_run_ms;
} PeriodicTask;

void PeriodicTask_Init(PeriodicTask *task, uint32_t period_ms, uint32_t start_ms);
bool PeriodicTask_IsDue(PeriodicTask *task, uint32_t now_ms);

#endif
