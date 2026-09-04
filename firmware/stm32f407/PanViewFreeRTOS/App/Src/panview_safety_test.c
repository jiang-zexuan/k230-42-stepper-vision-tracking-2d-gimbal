#include "panview_safety_test.h"
#include "panview_safety.h"

/*
 * 这个模块只负责“制造测试故障”。
 * 它不直接控制电机，电机仍由 SafetyTask 的统一停机路径处理。
 */
void PanView_SafetyTest_InjectEmergencyStop(uint32_t tick_ms)
{
  PanView_Safety_RequestEmergencyStop(tick_ms);
}
