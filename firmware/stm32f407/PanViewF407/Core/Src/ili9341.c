/* USER CODE BEGIN Header */
/**
  * @file    ili9341.c
  * @brief   ILI9341 initialization and RGB565 fill test.
  */
/* USER CODE END Header */
#include "ili9341.h"
#include "main.h"
#include "spi.h"

enum
{
  TFT_SPI_TIMEOUT_MS = 100U,
  ILI9341_CMD_READ_ID = 0x04U,
  ILI9341_CMD_SWRESET = 0x01U,
  ILI9341_CMD_SLP_OUT = 0x11U,
  ILI9341_CMD_INV_OFF = 0x20U,
  ILI9341_CMD_DISP_ON = 0x29U,
  ILI9341_CMD_CASET = 0x2AU,
  ILI9341_CMD_PASET = 0x2BU,
  ILI9341_CMD_RAMWR = 0x2CU,
  ILI9341_CMD_MADCTL = 0x36U,
  ILI9341_CMD_PIXFMT = 0x3AU
};

static void Tft_Select(void)
{
  HAL_GPIO_WritePin(TFT_LCD_CS_GPIO_Port, TFT_LCD_CS_Pin, GPIO_PIN_RESET);
}

static void Tft_Deselect(void)
{
  HAL_GPIO_WritePin(TFT_LCD_CS_GPIO_Port, TFT_LCD_CS_Pin, GPIO_PIN_SET);
}

static bool Tft_WriteCommand(uint8_t command)
{
  HAL_StatusTypeDef status;

  HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_RESET);
  Tft_Select();
  status = HAL_SPI_Transmit(&hspi1, &command, 1U, TFT_SPI_TIMEOUT_MS);
  Tft_Deselect();
  return status == HAL_OK;
}

static bool Tft_WriteCommandData(uint8_t command,
                                 const uint8_t *data,
                                 uint16_t size)
{
  HAL_StatusTypeDef status;

  /* ILI9341 要求命令和参数处于同一次片选事务中。 */
  HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_RESET);
  Tft_Select();
  status = HAL_SPI_Transmit(&hspi1, &command, 1U, TFT_SPI_TIMEOUT_MS);
  if (status == HAL_OK)
  {
    HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_SET);
    status = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size,
                              TFT_SPI_TIMEOUT_MS);
  }
  Tft_Deselect();
  return status == HAL_OK;
}

static bool Tft_SetWindow(uint16_t x0, uint16_t y0,
                          uint16_t x1, uint16_t y1)
{
  uint8_t column_data[4] = {
      (uint8_t)(x0 >> 8), (uint8_t)x0,
      (uint8_t)(x1 >> 8), (uint8_t)x1};
  uint8_t page_data[4] = {
      (uint8_t)(y0 >> 8), (uint8_t)y0,
      (uint8_t)(y1 >> 8), (uint8_t)y1};

  return Tft_WriteCommandData(ILI9341_CMD_CASET, column_data,
                              sizeof(column_data)) &&
         Tft_WriteCommandData(ILI9341_CMD_PASET, page_data,
                              sizeof(page_data)) &&
         Tft_WriteCommand(ILI9341_CMD_RAMWR);
}

static const uint8_t *Tft_Font5x7(char character)
{
  static const uint8_t glyph_space[] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
  static const uint8_t glyph_a[] = {0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
  static const uint8_t glyph_b[] = {0x1EU, 0x11U, 0x11U, 0x1EU, 0x11U, 0x11U, 0x1EU};
  static const uint8_t glyph_d[] = {0x1EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x1EU};
  static const uint8_t glyph_e[] = {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU};
  static const uint8_t glyph_f[] = {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U};
  static const uint8_t glyph_g[] = {0x0FU, 0x10U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0FU};
  static const uint8_t glyph_i[] = {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x1FU};
  static const uint8_t glyph_k[] = {0x11U, 0x12U, 0x14U, 0x18U, 0x14U, 0x12U, 0x11U};
  static const uint8_t glyph_m[] = {0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U};
  static const uint8_t glyph_n[] = {0x11U, 0x19U, 0x15U, 0x13U, 0x11U, 0x11U, 0x11U};
  static const uint8_t glyph_o[] = {0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
  static const uint8_t glyph_p[] = {0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x10U, 0x10U};
  static const uint8_t glyph_r[] = {0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U};
  static const uint8_t glyph_s[] = {0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU};
  static const uint8_t glyph_t[] = {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U};
  static const uint8_t glyph_v[] = {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U};
  static const uint8_t glyph_w[] = {0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU};
  static const uint8_t glyph_y[] = {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U};
  static const uint8_t glyph_0[] = {0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU};
  static const uint8_t glyph_1[] = {0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
  static const uint8_t glyph_5[] = {0x1FU, 0x10U, 0x10U, 0x1EU, 0x01U, 0x01U, 0x1EU};
  static const uint8_t glyph_6[] = {0x0EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU};

  switch (character)
  {
    case 'A': return glyph_a;
    case 'B': return glyph_b;
    case 'D': return glyph_d;
    case 'E': return glyph_e;
    case 'F': return glyph_f;
    case 'G': return glyph_g;
    case 'I': return glyph_i;
    case 'K': return glyph_k;
    case 'M': return glyph_m;
    case 'N': return glyph_n;
    case 'O': return glyph_o;
    case 'P': return glyph_p;
    case 'R': return glyph_r;
    case 'S': return glyph_s;
    case 'T': return glyph_t;
    case 'V': return glyph_v;
    case 'W': return glyph_w;
    case 'Y': return glyph_y;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '5': return glyph_5;
    case '6': return glyph_6;
    default: return glyph_space;
  }
}

bool ILI9341_Init(void)
{
  static const uint8_t power_on_sequence[] = {0x00U, 0xC1U, 0x30U};
  static const uint8_t power_control_a[] = {0x64U, 0x03U, 0x12U, 0x81U};
  static const uint8_t display_timing[] = {0x85U, 0x00U, 0x78U};
  static const uint8_t power_control_b[] = {0x39U, 0x2CU, 0x00U, 0x34U, 0x02U};
  static const uint8_t charge_pump[] = {0x20U};
  static const uint8_t driver_timing[] = {0x00U, 0x00U};
  static const uint8_t power_control[] = {0x13U};
  static const uint8_t vcom_control[] = {0x22U, 0x35U};
  static const uint8_t vcom_control2[] = {0xBDU};
  /* 屏幕物理倒置：MX+MY+BGR，旋转 180 度且保持当前 240x320 方向。 */
  static const uint8_t memory_access[] = {0xC8U};
  static const uint8_t display_function[] = {0x0AU, 0xA2U};
  static const uint8_t pixel_format[] = {0x55U};
  static const uint8_t interface_control[] = {0x01U, 0x30U};
  static const uint8_t frame_rate[] = {0x00U, 0x1BU};
  static const uint8_t gamma_enable[] = {0x00U};
  static const uint8_t gamma_select[] = {0x01U};
  static const uint8_t positive_gamma[] = {
      0x0FU, 0x35U, 0x31U, 0x0BU, 0x0EU, 0x06U, 0x49U, 0xA7U,
      0x33U, 0x07U, 0x0FU, 0x03U, 0x0CU, 0x0AU, 0x00U};
  static const uint8_t negative_gamma[] = {
      0x00U, 0x0AU, 0x0FU, 0x04U, 0x11U, 0x08U, 0x36U, 0x58U,
      0x4DU, 0x07U, 0x10U, 0x0CU, 0x32U, 0x34U, 0x0FU};

  HAL_GPIO_WritePin(TFT_LCD_BL_GPIO_Port, TFT_LCD_BL_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TFT_LCD_CS_GPIO_Port, TFT_LCD_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TFT_LCD_RST_GPIO_Port, TFT_LCD_RST_Pin, GPIO_PIN_SET);

  HAL_GPIO_WritePin(TFT_LCD_RST_GPIO_Port, TFT_LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20U);
  HAL_GPIO_WritePin(TFT_LCD_RST_GPIO_Port, TFT_LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120U);

  if (!Tft_WriteCommand(ILI9341_CMD_SWRESET))
  {
    return false;
  }
  HAL_Delay(120U);

  /* 按官方 MSP2833/MSP2834 STM32 示例补齐电源、Gamma 和接口初始化。 */
  if (!Tft_WriteCommandData(0xCFU, power_on_sequence,
                            sizeof(power_on_sequence)) ||
      !Tft_WriteCommandData(0xEDU, power_control_a,
                            sizeof(power_control_a)) ||
      !Tft_WriteCommandData(0xE8U, display_timing,
                            sizeof(display_timing)) ||
      !Tft_WriteCommandData(0xCBU, power_control_b,
                            sizeof(power_control_b)) ||
      !Tft_WriteCommandData(0xF7U, charge_pump, sizeof(charge_pump)) ||
      !Tft_WriteCommandData(0xEAU, driver_timing, sizeof(driver_timing)) ||
      !Tft_WriteCommandData(0xC0U, power_control, sizeof(power_control)) ||
      !Tft_WriteCommandData(0xC1U, power_control, sizeof(power_control)) ||
      !Tft_WriteCommandData(0xC5U, vcom_control, sizeof(vcom_control)) ||
      !Tft_WriteCommandData(0xC7U, vcom_control2, sizeof(vcom_control2)) ||
      !Tft_WriteCommand(0x21U) ||
      !Tft_WriteCommandData(ILI9341_CMD_MADCTL, memory_access,
                            sizeof(memory_access)) ||
      !Tft_WriteCommandData(0xB6U, display_function,
                            sizeof(display_function)) ||
      !Tft_WriteCommandData(ILI9341_CMD_PIXFMT, pixel_format,
                            sizeof(pixel_format)) ||
      !Tft_WriteCommandData(0xF6U, interface_control,
                            sizeof(interface_control)) ||
      !Tft_WriteCommandData(0xB1U, frame_rate, sizeof(frame_rate)) ||
      !Tft_WriteCommandData(0xF2U, gamma_enable, sizeof(gamma_enable)) ||
      !Tft_WriteCommandData(0x26U, gamma_select, sizeof(gamma_select)) ||
      !Tft_WriteCommandData(0xE0U, positive_gamma,
                            sizeof(positive_gamma)) ||
      !Tft_WriteCommandData(0xE1U, negative_gamma,
                            sizeof(negative_gamma)) ||
      !Tft_WriteCommand(ILI9341_CMD_SLP_OUT))
  {
    return false;
  }

  HAL_Delay(120U);
  return Tft_WriteCommand(ILI9341_CMD_DISP_ON);
}

bool ILI9341_ReadId(uint8_t id[4])
{
  uint8_t command = ILI9341_CMD_READ_ID;
  uint8_t dummy = 0xFFU;
  uint8_t received = 0U;
  uint8_t index;
  HAL_StatusTypeDef status;

  if (id == NULL)
  {
    return false;
  }

  HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_RESET);
  Tft_Select();
  status = HAL_SPI_Transmit(&hspi1, &command, 1U, TFT_SPI_TIMEOUT_MS);
  if (status == HAL_OK)
  {
    HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_SET);
    /* 读命令后，主机仍需发送 dummy 字节来产生 SCK；
       每个字节同时发送 0xFF 并采样 MISO，避免依赖只接收接口的时序。 */
    for (index = 0U; index < 4U && status == HAL_OK; index++)
    {
      status = HAL_SPI_TransmitReceive(&hspi1, &dummy, &received, 1U,
                                      TFT_SPI_TIMEOUT_MS);
      id[index] = received;
    }
  }
  Tft_Deselect();
  return status == HAL_OK;
}

void ILI9341_FillColor(uint16_t color)
{
  ILI9341_FillRect(0U, 0U, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, uint16_t color)
{
  uint8_t row[ILI9341_WIDTH * 2U];
  uint16_t column;
  uint16_t row_index;

  if ((width == 0U) || (height == 0U) || (x >= ILI9341_WIDTH) ||
      (y >= ILI9341_HEIGHT))
  {
    return;
  }

  if (width > (ILI9341_WIDTH - x))
  {
    width = ILI9341_WIDTH - x;
  }
  if (height > (ILI9341_HEIGHT - y))
  {
    height = ILI9341_HEIGHT - y;
  }

  for (column = 0U; column < width; column++)
  {
    /* 官方 MSP2833/MSP2834 STM32 示例：RGB565 高字节先发。 */
    row[2U * column] = (uint8_t)(color >> 8);
    row[2U * column + 1U] = (uint8_t)color;
  }

  if (!Tft_SetWindow(x, y, (uint16_t)(x + width - 1U),
                     (uint16_t)(y + height - 1U)))
  {
    return;
  }

  HAL_GPIO_WritePin(TFT_LCD_DC_GPIO_Port, TFT_LCD_DC_Pin, GPIO_PIN_SET);
  Tft_Select();
  for (row_index = 0U; row_index < height; row_index++)
  {
    (void)HAL_SPI_Transmit(&hspi1, row, (uint16_t)(width * 2U),
                           TFT_SPI_TIMEOUT_MS);
  }
  Tft_Deselect();
}

void ILI9341_DrawText(uint16_t x, uint16_t y, const char *text,
                      uint16_t foreground, uint16_t background,
                      uint8_t scale)
{
  const uint8_t *glyph;
  uint8_t glyph_row;
  uint8_t glyph_column;

  if ((text == NULL) || (scale == 0U))
  {
    return;
  }

  while (*text != '\0')
  {
    glyph = Tft_Font5x7(*text);
    for (glyph_row = 0U; glyph_row < 7U; glyph_row++)
    {
      for (glyph_column = 0U; glyph_column < 5U; glyph_column++)
      {
        uint16_t color = (glyph[glyph_row] & (uint8_t)(0x10U >> glyph_column))
                             ? foreground
                             : background;
        ILI9341_FillRect((uint16_t)(x + glyph_column * scale),
                         (uint16_t)(y + glyph_row * scale),
                         scale, scale, color);
      }
    }
    x = (uint16_t)(x + 6U * scale);
    text++;
  }
}
