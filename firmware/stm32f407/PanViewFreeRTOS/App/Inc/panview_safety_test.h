#ifndef PANVIEW_SAFETY_TEST_H
#define PANVIEW_SAFETY_TEST_H

#include <stdint.h>

/* 注入一次软件急停故障，仅供 T08 测试代码调用。 */
void PanView_SafetyTest_InjectEmergencyStop(uint32_t tick_ms);

#endif /* PANVIEW_SAFETY_TEST_H */
