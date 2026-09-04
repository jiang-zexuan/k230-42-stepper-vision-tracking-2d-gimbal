#include "es8388.h"

#include "i2c.h"

bool Es8388_Probe(void)
{
  return HAL_I2C_IsDeviceReady(&hi2c1, ES8388_I2C_ADDRESS, 3U, 100U) ==
         HAL_OK;
}

static bool Es8388_WriteRegister(uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(&hi2c1, ES8388_I2C_ADDRESS, reg,
                           I2C_MEMADD_SIZE_8BIT, &value, 1U, 100U) ==
         HAL_OK;
}

bool Es8388_InitPlayback(void)
{
  /* 只建立 DAC 播放路径，录音和输入增益暂不启用。寄存器编号依据
     ES8388 的寄存器定义：先静音、上电和配置，最后再解除静音。 */
  static const uint8_t init_table[][2] = {
      {0x19U, 0x04U}, /* DAC 静音 */
      {0x01U, 0x50U}, /* 模拟电路控制 */
      {0x02U, 0x00U}, /* 芯片电源全部上电 */
      {0x08U, 0x00U}, /* Codec 作为 I2S 从机 */
      {0x04U, 0xC0U}, /* 先关闭 DAC 和模拟输出 */
      {0x00U, 0x12U}, /* 播放模式 */
      {0x17U, 0x18U}, /* DAC：16 bit I2S */
      {0x18U, 0x02U}, /* 单倍速，256 x Fs */
      {0x26U, 0x00U}, /* DAC 输入选择 */
      {0x27U, 0x9CU}, /* 左 DAC -> 左混音器，0 dB */
      {0x2AU, 0x9CU}, /* 右 DAC -> 右混音器，0 dB */
      {0x2BU, 0x80U}, /* DAC 使用内部 LRCK */
      {0x2DU, 0x00U}, /* 输出模式 */
      {0x1AU, 0x00U}, /* 左 DAC 数字音量 */
      {0x1BU, 0x00U}, /* 右 DAC 数字音量 */
      {0x04U, 0x3CU}, /* 打开 DAC、LOUT/ROUT */
      {0x2EU, 0x1EU}, /* 左输出音量 */
      {0x2FU, 0x1EU}, /* 右输出音量 */
      {0x30U, 0x1EU}, /* 板载扬声器左输出 LOUT2 */
      {0x31U, 0x1EU}, /* 板载扬声器右输出 ROUT2 */
      {0x19U, 0x00U}, /* 解除 DAC 静音 */
  };

  for (uint32_t index = 0U;
       index < (sizeof(init_table) / sizeof(init_table[0])); ++index)
  {
    if (!Es8388_WriteRegister(init_table[index][0], init_table[index][1]))
    {
      return false;
    }
    if (index == 0U)
    {
      HAL_Delay(10U);
    }
  }
  return true;
}
