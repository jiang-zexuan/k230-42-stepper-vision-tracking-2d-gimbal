#include <assert.h>
#include <stdio.h>

#include "panview_speed_ramp.h"

int main(void)
{
  int32_t speed;

  /* 从停止开始时，每个 10 ms 周期只增加 40 step/s。 */
  speed = PanView_SpeedRamp_Step(0, 820, 40);
  assert(speed == 40);
  speed = PanView_SpeedRamp_Step(speed, 820, 40);
  assert(speed == 80);

  /* 收到停止目标时同样逐步减速，不能从 80 直接跳到 0。 */
  speed = PanView_SpeedRamp_Step(speed, 0, 40);
  assert(speed == 40);
  speed = PanView_SpeedRamp_Step(speed, 0, 40);
  assert(speed == 0);

  /* 反向命令必须先回到 0，不能从正速度直接跨到负速度。 */
  speed = PanView_SpeedRamp_Step(80, -820, 40);
  assert(speed == 40);
  speed = PanView_SpeedRamp_Step(speed, -820, 40);
  assert(speed == 0);
  speed = PanView_SpeedRamp_Step(speed, -820, 40);
  assert(speed == -40);

  /* 已经接近目标时直接到达目标，避免在目标附近来回变化。 */
  speed = PanView_SpeedRamp_Step(800, 820, 40);
  assert(speed == 820);

  printf("SPEED_RAMP_TEST=PASS\n");
  return 0;
}
