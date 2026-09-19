#ifndef CONSOLE_CMD_H
#define CONSOLE_CMD_H

#include "stdint.h"

/*
 * Console command table.
 *
 * Each command lives in its own file under src/console_commands/ and exports a
 * single `const ConsoleCommand` describing itself. A demo builds an array of
 * pointers to the ones it wants and hands that to Console_run(), so a demo only
 * links the hardware it actually has wired up.
 *
 * To add a command: create foo.c/foo.h here, export `console_cmd_foo`, and add
 * `&console_cmd_foo` to the table in whichever demo should have it.
 */
typedef struct ConsoleCommand
{
    const char *name;        // the word typed at the prompt -- no spaces
    const char *help;        // one-line summary shown by 'help'
    void (*init)(void);      // one-time hardware setup, or 0 if none is needed
    void (*run)(char *args); // whatever was typed after `name` ("" if nothing)
} ConsoleCommand;

/*
 * Splits the leading word off `line` (in place) and runs the matching command.
 * Returns 0 if nothing matched, in which case `line` has been NUL-terminated
 * after that word so the caller can print it back.
 *
 * A command with sub-commands just calls this again on its own args -- that is
 * all "led on" is: Console_dispatch() on led.c's private sub-command table.
 *
 * `init` is NOT called from here; Console_run() calls it once at startup for
 * the top-level table, so sub-command entries leave it 0.
 */
uint8_t Console_dispatch(const ConsoleCommand *const *table, uint32_t count, char *line);

// Prints one "  name  - help" line per entry. Used by the 'help' command and by
// the usage message a command prints when its sub-command doesn't match.
void Console_print_help(const ConsoleCommand *const *table, uint32_t count);

// Small helpers shared by command implementations (no libc in this build).
uint8_t  Console_str_eq(const char *a, const char *b);
uint32_t Console_parse_uint(const char *s);

#endif
