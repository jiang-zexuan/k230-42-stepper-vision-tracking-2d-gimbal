/* USER CODE BEGIN Header */
/**
  * @file    ili9341.h
  * @brief   ILI9341 driver for PanView TFT diagnostics and status text.
  */
/* USER CODE END Header */
#ifndef __ILI9341_H__
#define __ILI9341_H__

#include <stdbool.h>
#include <stdint.h>

enum
{
  ILI9341_WIDTH = 240U,
  ILI9341_HEIGHT = 320U,
  ILI9341_COLOR_BLACK = 0x0000U,
  ILI9341_COLOR_BLUE = 0x001FU,
  ILI9341_COLOR_RED = 0xF800U,
  ILI9341_COLOR_GREEN = 0x07E0U,
  ILI9341_COLOR_CYAN = 0x07FFU,
  ILI9341_COLOR_YELLOW = 0xFFE0U,
  ILI9341_COLOR_ORANGE = 0xFD20U,
  ILI9341_COLOR_WHITE = 0xFFFFU
};

bool ILI9341_Init(void);
bool ILI9341_ReadId(uint8_t id[4]);
void ILI9341_FillColor(uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, uint16_t color);
void ILI9341_DrawText(uint16_t x, uint16_t y, const char *text,
                      uint16_t foreground, uint16_t background,
                      uint8_t scale);

#endif /* __ILI9341_H__ */
