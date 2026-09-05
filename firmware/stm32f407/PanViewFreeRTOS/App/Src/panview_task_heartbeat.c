/*
 * 文件用途：实现任务心跳表及其访问函数。
 * 心跳表只在本文件内部保存，其他模块通过头文件中的函数访问它。
 */
#include "FreeRTOS.h"
#include "panview_task_heartbeat.h"
#include "task.h"

/*
 * 保存所有任务心跳记录的统一表。
 * 数组下标由 PanViewHeartbeatId 枚举值决定。
 */
static volatile PanViewHeartbeatRecord heartbeat_records[
    PANVIEW_HEARTBEAT_COUNT];

/* 保存每个心跳编号对应的任务句柄。 */
static osThreadId_t heartbeat_task_handles[PANVIEW_HEARTBEAT_COUNT];

void PanView_TaskHeartbeat_Update(PanViewHeartbeatId heartbeat_id,
                                  uint32_t current_tick)
{
  /* 防止错误编号访问数组范围之外的内存。 */
  if (heartbeat_id >= PANVIEW_HEARTBEAT_COUNT)
  {
    return;
  }

  /* 先保存本次运行时间，再把累计运行次数加一。 */
  heartbeat_records[heartbeat_id].last_tick = current_tick;
  heartbeat_records[heartbeat_id].count++;
}

void PanView_TaskHeartbeat_UpdateStack(PanViewHeartbeatId heartbeat_id,
                                       uint32_t stack_words)
{
  /* 防止错误编号访问数组范围之外的内存。 */
  if (heartbeat_id >= PANVIEW_HEARTBEAT_COUNT)
  {
    return;
  }

  /* 保存任务当前查询到的历史最低剩余栈空间。 */
  heartbeat_records[heartbeat_id].stack_high_water_mark_words = stack_words;
}

void PanView_TaskHeartbeat_UpdateState(PanViewHeartbeatId heartbeat_id,
                                       uint32_t state)
{
  /* 防止错误编号访问数组范围之外的内存。 */
  if (heartbeat_id >= PANVIEW_HEARTBEAT_COUNT)
  {
    return;
  }

  /* 保存任务最近一次查询到的 RTOS 状态。 */
  heartbeat_records[heartbeat_id].state = state;
}

void PanView_TaskHeartbeat_RegisterTask(PanViewHeartbeatId heartbeat_id,
                                        osThreadId_t task_handle)
{
  /* 防止错误编号访问数组范围之外的内存。 */
  if (heartbeat_id >= PANVIEW_HEARTBEAT_COUNT)
  {
    return;
  }

  /* 保存任务句柄，后续由本模块统一查询任务信息。 */
  heartbeat_task_handles[heartbeat_id] = task_handle;
}

void PanView_TaskHeartbeat_CollectRuntimeInfo(void)
{
  uint32_t heartbeat_id;

  /* 依次查询所有已经注册的任务。 */
  for (heartbeat_id = 0U;
       heartbeat_id < (uint32_t)PANVIEW_HEARTBEAT_COUNT;
       heartbeat_id++)
  {
    osThreadId_t task_handle = heartbeat_task_handles[heartbeat_id];

    /* 未注册的任务没有有效资源信息。 */
    if (task_handle == NULL)
    {
      heartbeat_records[heartbeat_id].stack_high_water_mark_words = 0U;
      heartbeat_records[heartbeat_id].state = (uint32_t)osThreadError;
      continue;
    }

    /* 读取任务历史最低剩余栈空间，单位：word。 */
    heartbeat_records[heartbeat_id].stack_high_water_mark_words =
        (uint32_t)uxTaskGetStackHighWaterMark(
            (TaskHandle_t)task_handle);

    /* 读取任务当前的 CMSIS-RTOS 状态。 */
    heartbeat_records[heartbeat_id].state =
        (uint32_t)osThreadGetState(task_handle);
  }
}

const volatile PanViewHeartbeatRecord *PanView_TaskHeartbeat_Get(
    PanViewHeartbeatId heartbeat_id)
{
  /* 无效编号不能返回有效记录。 */
  if (heartbeat_id >= PANVIEW_HEARTBEAT_COUNT)
  {
    return 0;
  }

  /* 返回指定编号对应记录的地址，不复制整条记录。 */
  return &heartbeat_records[heartbeat_id];
}
