/*
 * 文件用途：提供 PanView 各个 FreeRTOS 任务的统一心跳记录接口。
 *
 * 其他模块只通过本文件声明的函数访问心跳表，
 * 不直接修改心跳表内部数据。
 */
#ifndef PANVIEW_TASK_HEARTBEAT_H
#define PANVIEW_TASK_HEARTBEAT_H

#include <stdint.h>
#include "cmsis_os2.h"

/*
 * 任务心跳记录的统一编号。
 * 每个任务对应一个编号，编号会作为心跳表的数组下标使用。
 */
typedef enum
{
  PANVIEW_HEARTBEAT_ARCHITECTURE = 0,
  PANVIEW_HEARTBEAT_SAFETY,
  PANVIEW_HEARTBEAT_STEPPER,
  PANVIEW_HEARTBEAT_VISION_RX,
  PANVIEW_HEARTBEAT_MOTION,
  PANVIEW_HEARTBEAT_INPUT,
  PANVIEW_HEARTBEAT_APP_CONTROL,
  PANVIEW_HEARTBEAT_UI,
  PANVIEW_HEARTBEAT_AUDIO,
  PANVIEW_HEARTBEAT_TELEMETRY,
  PANVIEW_HEARTBEAT_COUNT
} PanViewHeartbeatId;

/*
 * 单个任务的心跳记录。
 * count 表示任务累计运行次数；
 * last_tick 表示任务最近一次运行时的 RTOS/HAL 时间戳；
 * stack_high_water_mark_words 表示历史最低剩余栈空间；
 * state 表示任务最近一次查询到的 RTOS 状态。
 */
typedef struct
{
  volatile uint32_t count;
  volatile uint32_t last_tick;
  volatile uint32_t stack_high_water_mark_words;
  volatile uint32_t state;
} PanViewHeartbeatRecord;

/*
 * 更新指定任务的一次心跳记录。
 * 任务每完成一轮主要工作，就可以调用一次这个函数。
 */
void PanView_TaskHeartbeat_Update(PanViewHeartbeatId heartbeat_id,
                                  uint32_t current_tick);

/* 更新指定任务的历史最低剩余栈空间，单位：word。 */
void PanView_TaskHeartbeat_UpdateStack(PanViewHeartbeatId heartbeat_id,
                                       uint32_t stack_words);

/* 更新指定任务最近一次查询到的 RTOS 任务状态。 */
void PanView_TaskHeartbeat_UpdateState(PanViewHeartbeatId heartbeat_id,
                                       uint32_t state);

/* 注册任务句柄，供心跳模块后续读取任务资源和状态。 */
void PanView_TaskHeartbeat_RegisterTask(PanViewHeartbeatId heartbeat_id,
                                        osThreadId_t task_handle);

/* 统一读取全部已注册任务的栈余量和 RTOS 状态。 */
void PanView_TaskHeartbeat_CollectRuntimeInfo(void);

/*
 * 读取指定任务的心跳记录，供 SafetyTask、TelemetryTask 和调试器使用。
 * 如果传入的编号无效，函数返回空指针。
 */
const volatile PanViewHeartbeatRecord *PanView_TaskHeartbeat_Get(
    PanViewHeartbeatId heartbeat_id);

#endif /* PANVIEW_TASK_HEARTBEAT_H */
