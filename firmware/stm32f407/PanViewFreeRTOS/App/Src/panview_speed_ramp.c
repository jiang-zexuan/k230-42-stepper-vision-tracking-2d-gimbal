#include "panview_speed_ramp.h"

int32_t PanView_SpeedRamp_Step(int32_t current_speed,
                               int32_t target_speed,
                               int32_t max_change)
{
  int32_t effective_target = target_speed;
  int64_t difference;

  if (max_change <= 0)
  {
    return current_speed;
  }

  /*
   * 当前速度和目标速度方向相反时，本轮目标先改为 0。
   * 这样电机一定会先减速停止，再从下一轮开始向反方向加速。
   */
  if (((current_speed > 0) && (target_speed < 0)) ||
      ((current_speed < 0) && (target_speed > 0)))
  {
    effective_target = 0;
  }

  difference = (int64_t)effective_target - current_speed;
  if (difference > max_change)
  {
    return current_speed + max_change;
  }
  if (difference < -max_change)
  {
    return current_speed - max_change;
  }

  return effective_target;
}
