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

    return UART_TEXT_COMMAND_UNKNOWN;
}
