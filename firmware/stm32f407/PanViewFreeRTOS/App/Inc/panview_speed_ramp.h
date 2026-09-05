#ifndef PANVIEW_SPEED_RAMP_H
#define PANVIEW_SPEED_RAMP_H

#include <stdint.h>

/*
 * 让当前速度向目标速度靠近一次，每次最多改变 max_change。
 * 如果目标方向与当前方向相反，必须先减速到 0，下一轮才开始反向加速。
 */
int32_t PanView_SpeedRamp_Step(int32_t current_speed,
                               int32_t target_speed,
                               int32_t max_change);

#endif /* PANVIEW_SPEED_RAMP_H */
