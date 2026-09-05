#include "panview_safety.h"

/* 当前只有 SafetyTask 读写，后续接入中断时再补充临界区保护。 */
static PanViewSafetyStatus safety_status;

void PanView_Safety_Init(void)
{
  safety_status.fault = PANVIEW_SAFETY_CLEAR;
  safety_status.latched = 0U;
  safety_status.fault_tick_ms = 0U;
}

void PanView_Safety_RaiseFault(PanViewSafetyFault fault,
                               uint32_t tick_ms)
{
  /* 无故障不是一个需要锁存的故障请求。 */
  if (fault == PANVIEW_SAFETY_CLEAR)
  {
    return;
  }

  /* 锁存后保留第一次故障，避免后续现象覆盖真正的起因。 */
  if (safety_status.latched == 0U)
  {
    safety_status.fault = fault;
    safety_status.fault_tick_ms = tick_ms;
    safety_status.latched = 1U;
  }
}

void PanView_Safety_RequestEmergencyStop(uint32_t tick_ms)
{
  PanView_Safety_RaiseFault(PANVIEW_SAFETY_EMERGENCY_STOP, tick_ms);
}

void PanView_Safety_GetStatus(PanViewSafetyStatus *status)
{
  if (status == 0)
  {
    return;
  }

  *status = safety_status;
}

void PanView_Safety_ClearFault(void)
{
  safety_status.fault = PANVIEW_SAFETY_CLEAR;
  safety_status.latched = 0U;
  safety_status.fault_tick_ms = 0U;
}
