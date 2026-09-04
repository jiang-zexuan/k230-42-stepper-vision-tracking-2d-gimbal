#ifndef PANVIEW_SAFETY_H
#define PANVIEW_SAFETY_H

#include <stdint.h>

/* T08 当前统一管理的安全故障类型。 */
typedef enum
{
  PANVIEW_SAFETY_CLEAR = 0,
  PANVIEW_SAFETY_CONTROL_TASK_STALE,
  PANVIEW_SAFETY_SOFTWARE_LIMIT,
  PANVIEW_SAFETY_UART_ERROR,
  PANVIEW_SAFETY_EMERGENCY_STOP
} PanViewSafetyFault;

/* 安全模块对外提供的状态快照。 */
typedef struct
{
  PanViewSafetyFault fault;
  uint8_t latched;
  uint32_t fault_tick_ms;
} PanViewSafetyStatus;

/* 初始化为无故障状态。系统启动时调用一次。 */
void PanView_Safety_Init(void);

/* 设置故障；首次故障会被锁存，后续不会被普通状态覆盖。 */
void PanView_Safety_RaiseFault(PanViewSafetyFault fault,
                               uint32_t tick_ms);

/* 软件急停入口：供输入任务或故障注入测试调用。 */
void PanView_Safety_RequestEmergencyStop(uint32_t tick_ms);

/* 读取当前安全状态；不会修改内部状态。 */
void PanView_Safety_GetStatus(PanViewSafetyStatus *status);

/* 清除故障，后续由 T08 的重新回中/复位流程调用。 */
void PanView_Safety_ClearFault(void);

#endif /* PANVIEW_SAFETY_H */
