#include "uart_text_command.h"

#include <stddef.h>
#include <string.h>

static uint16_t UartTextCommand_TrimLineEnding(const uint8_t *data, uint16_t size)
{
    while ((size > 0U) &&
           ((data[size - 1U] == '\r') || (data[size - 1U] == '\n')))
    {
        size--;
    }

    return size;
}

UartTextCommand UartTextCommand_Parse(const uint8_t *data, uint16_t size)
{
    if (data == NULL)
    {
        return UART_TEXT_COMMAND_UNKNOWN;
    }

    size = UartTextCommand_TrimLineEnding(data, size);

    if ((size == 4U) && (memcmp(data, "PING", size) == 0))
    {
        return UART_TEXT_COMMAND_PING;
    }

    if ((size == 6U) && (memcmp(data, "STATUS", size) == 0))
    {
        return UART_TEXT_COMMAND_STATUS;
    }

    if ((size == 8U) && (memcmp(data, "PITCH_UP", size) == 0))
    {
        return UART_TEXT_COMMAND_PITCH_UP;
    }

    if ((size == 10U) && (memcmp(data, "PITCH_DOWN", size) == 0))
    {
        return UART_TEXT_COMMAND_PITCH_DOWN;
    }

    if ((size == 10U) && (memcmp(data, "PITCH_STOP", size) == 0))
    {
        return UART_TEXT_COMMAND_PITCH_STOP;
    }

    if ((size == 9U) && (memcmp(data, "DUAL_TEST", size) == 0))
    {
        return UART_TEXT_COMMAND_DUAL_TEST;
    }

    if ((size == 13U) && (memcmp(data, "DUAL_TEST_NEG", size) == 0))
    {
        return UART_TEXT_COMMAND_DUAL_TEST_NEG;
    }

    return UART_TEXT_COMMAND_UNKNOWN;
}
