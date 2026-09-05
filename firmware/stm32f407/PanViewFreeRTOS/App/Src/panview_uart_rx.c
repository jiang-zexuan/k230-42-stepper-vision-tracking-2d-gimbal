#include "panview_uart_rx.h"
#include "usart.h"
#include <string.h>

static uint8_t dma_rx_buffer[PANVIEW_UART_RX_BUFFER_SIZE];

/* 模块内部唯一的接收缓冲区，其他模块通过接口访问。 */
static PanViewUartRxBuffer uart_rx_buffer;

/* 初始化接收长度和统计计数。 */
void PanView_UartRx_Init(void)
{
  PanView_RingBuffer_Init(&uart_rx_buffer.ring_buffer);
  uart_rx_buffer.last_length = 0U;
  uart_rx_buffer.chunk_count = 0U;
  uart_rx_buffer.received_byte_count = 0U;
  uart_rx_buffer.dropped_byte_count = 0U;
  uart_rx_buffer.error_count = 0U;
  uart_rx_buffer.start_attempt_count = 0U;
  uart_rx_buffer.start_success_count = 0U;
  uart_rx_buffer.last_start_status = 0U;
  uart_rx_buffer.last_uart_state = 0U;
  uart_rx_buffer.last_uart_error_code = 0U;
}

/* 启动 USART2 DMA 接收，返回 0 表示成功。 */
int PanView_UartRx_Start(void)
{
  HAL_StatusTypeDef status;

  uart_rx_buffer.start_attempt_count++;
  status = HAL_UARTEx_ReceiveToIdle_DMA(&huart2, dma_rx_buffer,
                                        PANVIEW_UART_RX_BUFFER_SIZE);
  uart_rx_buffer.last_start_status = (uint32_t)status;
  uart_rx_buffer.last_uart_state = (uint32_t)HAL_UART_GetState(&huart2);

  if (status == HAL_OK)
  {
    /* 只使用空闲线或缓冲区写满事件，不处理中途的半满通知。 */
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    uart_rx_buffer.start_success_count++;
    return 0;
  }

  return -1;
}

/* HAL 回调调用此函数，记录本次收到的字节数。 */
void PanView_UartRx_OnReceive(uint16_t length)
{
  uint16_t written;

  if (length > PANVIEW_UART_RX_BUFFER_SIZE)
  {
    length = PANVIEW_UART_RX_BUFFER_SIZE;
  }

  written = PanView_RingBuffer_Write(&uart_rx_buffer.ring_buffer,
                                     dma_rx_buffer, length);
  uart_rx_buffer.last_length = length;
  uart_rx_buffer.chunk_count++;
  uart_rx_buffer.received_byte_count += length;
  uart_rx_buffer.dropped_byte_count += (uint32_t)(length - written);
}

/* 保存 HAL 提供的错误码，便于通过 USART1 日志定位问题。 */
void PanView_UartRx_OnError(uint32_t error_code)
{
  uart_rx_buffer.error_count++;
  uart_rx_buffer.last_uart_error_code = error_code;
  uart_rx_buffer.last_uart_state = (uint32_t)HAL_UART_GetState(&huart2);
}

uint16_t PanView_UartRx_Read(uint8_t *output, uint16_t output_size)
{
  return PanView_RingBuffer_Read(&uart_rx_buffer.ring_buffer,
                                 output, output_size);
}

PanViewUartRxBuffer *PanView_UartRx_GetBuffer(void)
{
  return &uart_rx_buffer;
}
