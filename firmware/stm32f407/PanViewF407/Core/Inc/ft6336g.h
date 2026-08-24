#ifndef __FT6336G_H__
#define __FT6336G_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint8_t chip_id;
  uint8_t touch_count;
  uint16_t x;
  uint16_t y;
} Ft6336gPoint;

bool FT6336G_Init(void);
bool FT6336G_ReadPoint(Ft6336gPoint *point);
bool FT6336G_ReadChipId(uint8_t *chip_id);
bool FT6336G_ReadRegister(uint8_t address, uint8_t *value);

#endif /* __FT6336G_H__ */
