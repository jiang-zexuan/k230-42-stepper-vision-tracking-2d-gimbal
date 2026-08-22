#ifndef UART_TEXT_COMMAND_H
#define UART_TEXT_COMMAND_H

#include <stdint.h>

/* P03 文本协议的第一批命令。后续二进制协议不会复用这些枚举值。 */
typedef enum {
    UART_TEXT_COMMAND_UNKNOWN = 0,
    UART_TEXT_COMMAND_PING,
    UART_TEXT_COMMAND_STATUS,
    UART_TEXT_COMMAND_PITCH_UP,
    UART_TEXT_COMMAND_PITCH_DOWN,
    UART_TEXT_COMMAND_PITCH_STOP,
    UART_TEXT_COMMAND_DUAL_TEST,
    UART_TEXT_COMMAND_DUAL_TEST_NEG
} UartTextCommand;

/*
 * 解析一段已接收文本。仅忽略末尾的 CR/LF，其他字节必须与命令完全一致。
 * 这样 XCOM 是否自动追加换行，都不会影响 PING 和 STATUS 的识别。
 */
UartTextCommand UartTextCommand_Parse(const uint8_t *data, uint16_t size);

#endif
