#ifndef PANVIEW_FONT_16_H
#define PANVIEW_FONT_16_H

#include <stdint.h>

/* 16x16、逐行、高位在前的点阵。返回值为 32 字节字模。 */
const uint8_t *PanViewFont16_Find(uint16_t codepoint);

#endif
