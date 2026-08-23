#ifndef PANVIEW_ES8388_H
#define PANVIEW_ES8388_H

#include <stdbool.h>
#include <stdint.h>

#include "i2s.h"

/* ES8388 的 7 位 I2C 地址通常为 0x10，HAL 接口使用左移一位后的地址。 */
#define ES8388_I2C_ADDRESS (0x10U << 1U)

bool Es8388_Probe(void);
bool Es8388_InitPlayback(void);

#endif
