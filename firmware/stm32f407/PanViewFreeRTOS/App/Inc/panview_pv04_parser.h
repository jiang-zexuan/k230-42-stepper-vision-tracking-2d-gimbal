#ifndef PANVIEW_PV04_PARSER_H
#define PANVIEW_PV04_PARSER_H

#include <stdint.h>
#include "panview_messages.h"

/* 成功返回 1；格式错误返回 0。 */
uint8_t PanView_Pv04_Parse(const char *line, VisionResult *result);

#endif
