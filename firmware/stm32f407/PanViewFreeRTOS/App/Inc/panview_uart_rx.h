#ifndef PANVIEW_UART_RX_H
#define PANVIEW_UART_RX_H

#include <stdint.h>
#include "panview_ring_buffer.h"

/* 一次最多保存 256 字节的 USART2 接收数据。 */
#define PANVIEW_UART_RX_BUFFER_SIZE 256U
#define PANVIEW_UART_RX_FLAG_DATA_READY 0x0001U

typedef struct
{
  /* 连续保存 USART2 字节流的标准环形缓冲区。 */
  PanViewRingBuffer ring_buffer;
  /* 最近一次收到的数据长度，供调试日志显示。 */
  uint16_t last_length;
  /* DMA 回调次数和收到的原始字节总数。 */
  uint32_t chunk_count;
  uint32_t received_byte_count;
  /* 环形缓冲区满时无法保存的字节总数。 */
  uint32_t dropped_byte_count;
  /* UART 硬件错误次数。 */
  uint32_t error_count;
  /* DMA 接收启动的尝试次数和成功次数。 */
  uint32_t start_attempt_count;
  uint32_t start_success_count;
  /* 最近一次 HAL 启动结果：0=成功，1=错误，2=忙，3=超时。 */
  uint32_t last_start_status;
  /* 最近一次记录的 UART 状态和硬件错误码。 */
  uint32_t last_uart_state;
  uint32_t last_uart_error_code;
} PanViewUartRxBuffer;

/* 清空接收统计信息。 */
void PanView_UartRx_Init(void);
/* 启动一次 USART2 的 DMA + 空闲线接收。 */
int PanView_UartRx_Start(void);
/* 收到一段数据后，记录本次长度。 */
void PanView_UartRx_OnReceive(uint16_t length);
/* 记录 USART2 的硬件错误状态。 */
void PanView_UartRx_OnError(uint32_t error_code);
/* 从环形缓冲区取出一段数据；没有数据时返回 0。 */
uint16_t PanView_UartRx_Read(uint8_t *output, uint16_t output_size);
/* 取得接收缓冲区，供接收任务读取。 */
PanViewUartRxBuffer *PanView_UartRx_GetBuffer(void);

#endif
