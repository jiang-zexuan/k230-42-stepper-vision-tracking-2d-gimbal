#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/uart_text_command.h"

static void test_parse_recognizes_ping(void)
{
    const uint8_t input[] = "PING";

    assert(UartTextCommand_Parse(input, sizeof(input) - 1U) ==
           UART_TEXT_COMMAND_PING);
}

static void test_parse_ignores_xcom_line_ending_for_status(void)
{
    const uint8_t input[] = "STATUS\r\n";

    assert(UartTextCommand_Parse(input, sizeof(input) - 1U) ==
           UART_TEXT_COMMAND_STATUS);
}

static void test_parse_rejects_unknown_text(void)
{
    const uint8_t input[] = "MOVE";

    assert(UartTextCommand_Parse(input, sizeof(input) - 1U) ==
           UART_TEXT_COMMAND_UNKNOWN);
}

int main(void)
{
    test_parse_recognizes_ping();
    test_parse_ignores_xcom_line_ending_for_status();
    test_parse_rejects_unknown_text();
    puts("uart_text_command_test: PASS");
    return 0;
}
