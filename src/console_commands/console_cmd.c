#include "console_cmd.h"
#include "uart.h"

// Column the "- <help>" text starts at, so 'help' output lines up.
#define HELP_NAME_WIDTH 8

uint8_t Console_str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b; // true only if both strings hit '\0' at the same spot
}

// Parses a run of leading decimal digits from `s` (0 if there are none --
// indistinguishable from an actual "0", but every caller here rejects
// out-of-range indices anyway so that's harmless).
uint32_t Console_parse_uint(const char *s)
{
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9')
    {
        v = v * 10 + (uint32_t)(*s - '0');
        s++;
    }
    return v;
}

uint8_t Console_dispatch(const ConsoleCommand *const *table, uint32_t count, char *line)
{
    while (*line == ' ') line++;

    char *args = line;
    while (*args && *args != ' ') args++;
    if (*args == ' ') *args++ = '\0'; // terminate the command word
    while (*args == ' ') args++;      // and skip on to the first real argument

    for (uint32_t i = 0; i < count; i++)
    {
        if (Console_str_eq(line, table[i]->name))
        {
            table[i]->run(args);
            return 1;
        }
    }
    return 0;
}

void Console_print_help(const ConsoleCommand *const *table, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        UART_send_buffer("  ");
        UART_send_buffer(table[i]->name);

        uint32_t width = 0;
        while (table[i]->name[width]) width++;
        while (width++ < HELP_NAME_WIDTH) UART_send_byte(' ');

        UART_send_buffer("- ");
        UART_send_buffer(table[i]->help);
        UART_send_buffer("\r\n");
    }
}
