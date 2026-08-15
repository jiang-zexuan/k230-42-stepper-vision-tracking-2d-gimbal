#ifndef PERIODIC_TASK_H
#define PERIODIC_TASK_H

#include <stdbool.h>
#include <stdint.h>

/*
 * 软件周期任务的最小状态：任务周期和上次实际执行的时刻。
 * 它不创建线程，也不阻塞 CPU；主循环反复查询它是否到期。
 */
typedef struct {
    uint32_t period_ms;
    uint32_t last_run_ms;
} PeriodicTask;

/* period_ms 和 start_ms 的单位均为 ms，时间来源为 HAL_GetTick()。 */
void PeriodicTask_Init(PeriodicTask *task, uint32_t period_ms, uint32_t start_ms);

/* 到期时更新 last_run_ms 并返回 true；未到期时不改变任务状态。 */
bool PeriodicTask_IsDue(PeriodicTask *task, uint32_t now_ms);

#endif
