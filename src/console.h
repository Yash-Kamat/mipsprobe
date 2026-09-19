#ifndef CONSOLE_H
#define CONSOLE_H

#include "console_cmd.h"

// The built-in 'help' command -- lists whichever table Console_run() was given.
extern const ConsoleCommand console_cmd_help;

// Blocking UART REPL: calls every command's init() once, then prints a prompt,
// reads a line (with echo/backspace), dispatches it against `table`, repeats.
// Never returns.
void Console_run(const ConsoleCommand *const *table, uint32_t count);

#endif
