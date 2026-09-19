#include "stdint.h"
#include "console.h"
#include "uart.h"

#define CONSOLE_LINE_MAX 64

// The table Console_run() is driving, so 'help' can list it.
static const ConsoleCommand *const *active_table;
static uint32_t active_count;

// Reads one line into `buf` (NUL-terminated, capped at maxlen-1 chars),
// echoing input back and handling backspace/DEL. Blocks until Enter.
static void read_line(char *buf, uint16_t maxlen)
{
    uint16_t len = 0;
    while (1)
    {
        uint8_t c = UART_receive_byte_blocking();

        if (c == '\r' || c == '\n')
        {
            UART_send_buffer("\r\n");
            break;
        }
        else if (c == 0x7F || c == 0x08) // DEL or backspace
        {
            if (len > 0)
            {
                len--;
                UART_send_buffer("\b \b"); // erase the character on the terminal too
            }
        }
        else if (len < (uint16_t)(maxlen - 1) && c >= 0x20 && c < 0x7F) // printable only
        {
            buf[len++] = (char) c;
            UART_send_byte((char) c); // local echo
        }
    }
    buf[len] = '\0';
}

static void cmd_help(char *args)
{
    (void) args;
    UART_send_buffer("Commands:\r\n");
    Console_print_help(active_table, active_count);
}

const ConsoleCommand console_cmd_help = { "help", "this text", 0, cmd_help };

void Console_run(const ConsoleCommand *const *table, uint32_t count)
{
    char line[CONSOLE_LINE_MAX];

    active_table = table;
    active_count = count;

    for (uint32_t i = 0; i < count; i++)
    {
        if (table[i]->init) table[i]->init();
    }

    UART_send_buffer("\r\nmipsprobe console -- type 'help' for commands\r\n> ");

    while (1)
    {
        read_line(line, sizeof(line));

        char *cmd = line;
        while (*cmd == ' ') cmd++;

        if (*cmd != '\0' && !Console_dispatch(table, count, cmd))
        {
            // Console_dispatch() has NUL-terminated `cmd` after the command word.
            UART_send_buffer("Unknown command '"); UART_send_buffer(cmd);
            UART_send_buffer("' -- try 'help'\r\n");
        }

        UART_send_buffer("> ");
    }
}
