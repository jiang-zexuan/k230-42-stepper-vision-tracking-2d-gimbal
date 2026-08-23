#include "ft6336g.h"
#include "i2c.h"
#include "main.h"

enum
{
  FT6336G_I2C_ADDRESS = (0x38U << 1U),
  FT6336G_REG_TD_STATUS = 0x02U,
  FT6336G_REG_P1_XH = 0x03U,
  FT6336G_REG_CHIP_ID = 0xA0U,
  FT6336G_TIMEOUT_MS = 100U
};

bool FT6336G_Init(void)
{
  HAL_GPIO_WritePin(CTP_RST_GPIO_Port, CTP_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(CTP_RST_GPIO_Port, CTP_RST_Pin, GPIO_PIN_SET);
  /* FT6336G 数据手册第 3.5 节：复位释放后约 300 ms 才开始保证报点。 */
  HAL_Delay(300U);
  return HAL_I2C_IsDeviceReady(&hi2c1, FT6336G_I2C_ADDRESS, 2U,
                               FT6336G_TIMEOUT_MS) == HAL_OK;
}

bool FT6336G_ReadChipId(uint8_t *chip_id)
{
  return FT6336G_ReadRegister(FT6336G_REG_CHIP_ID, chip_id);
}

bool FT6336G_ReadRegister(uint8_t address, uint8_t *value)
{
  if (value == NULL)
  {
    return false;
  }
  return HAL_I2C_Mem_Read(&hi2c1, FT6336G_I2C_ADDRESS, address,
                          I2C_MEMADD_SIZE_8BIT, value, 1U,
                          FT6336G_TIMEOUT_MS) == HAL_OK;
}

bool FT6336G_ReadPoint(Ft6336gPoint *point)
{
  uint8_t data[4U];
  if (point == NULL)
  {
    return false;
  }
  point->touch_count = 0U;
  if (HAL_I2C_Mem_Read(&hi2c1, FT6336G_I2C_ADDRESS,
                       FT6336G_REG_TD_STATUS, I2C_MEMADD_SIZE_8BIT,
                       &point->touch_count, 1U,
                       FT6336G_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }
  if (point->touch_count == 0U)
  {
    return true;
  }
  if (point->touch_count > 2U)
  {
    point->touch_count = 2U;
  }
  if (HAL_I2C_Mem_Read(&hi2c1, FT6336G_I2C_ADDRESS,
                       FT6336G_REG_P1_XH, I2C_MEMADD_SIZE_8BIT,
                       data, sizeof(data), FT6336G_TIMEOUT_MS) != HAL_OK)
  {
    return false;
  }
  point->x = (uint16_t)(((uint16_t)(data[0] & 0x0FU) << 8U) | data[1]);
  point->y = (uint16_t)(((uint16_t)(data[2] & 0x0FU) << 8U) | data[3]);
  return true;
}
