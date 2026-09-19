#include "stdint.h"
#include "fpga.h"
#include "uart.h"
#include "i2c.h"
#include "fpga_mips.h"
#include "mips_programs.h"

#define FPGA_I2C_SPEED_HZ 100000

// Each CSR poll is one I2C read (~1 ms at 100 kHz), so this gives a program a
// few seconds to reach TARGET_PC. Not a calibrated time bound.
#define FPGA_WAIT_MAX_POLLS 5000

static void fpga_init(void)
{
    I2C1_config(FPGA_I2C_SPEED_HZ);
}

static void print_i2c_status(I2C_Status st)
{
    switch (st)
    {
        case I2C_OK:          UART_send_buffer("OK\r\n"); break;
        case I2C_ERR_NACK:    UART_send_buffer("ERROR: I2C NACK (no slave ack -- check wiring/address)\r\n"); break;
        case I2C_ERR_TIMEOUT: UART_send_buffer("ERROR: I2C timeout (no response on the bus)\r\n"); break;
        case I2C_ERR_ARG:     UART_send_buffer("ERROR: bad argument\r\n"); break;
        default:              UART_send_buffer("ERROR: unknown\r\n"); break;
    }
}

// Skips the current argument and the spaces after it; returns the next one ("" if none).
static char *next_arg(char *s)
{
    while (*s && *s != ' ') s++;
    while (*s == ' ') s++;
    return s;
}

static void fpga_list(char *args)
{
    (void) args;
    for (uint32_t i = 0; i < mips_program_count; i++)
    {
        const MIPS_Program *p = &mips_programs[i];
        UART_send_buffer("  [");        UART_send_uint32(i);
        UART_send_buffer("] ");         UART_send_buffer(p->name);
        UART_send_buffer(" - ");        UART_send_uint32(p->instr_len / 4);
        UART_send_buffer(" instr, ");   UART_send_uint32(p->data_len / 4);
        UART_send_buffer(" data words\r\n");
    }
}

static void fpga_load(char *args)
{
    if (*args == '\0')
    {
        UART_send_buffer("usage: fpga load <n> -- see 'fpga list'\r\n");
        return;
    }

    uint32_t idx = Console_parse_uint(args);
    if (idx >= mips_program_count)
    {
        UART_send_buffer("ERROR: no such program -- see 'fpga list'\r\n");
        return;
    }

    // DONE only clears on an FPGA reset, and a core with RUN set would carry
    // on from its current PC -- either way the load has to start from reset.
    uint32_t csr;
    I2C_Status st = FPGA_MIPS_read(FPGA_CSR, &csr);
    if (st != I2C_OK) { print_i2c_status(st); return; }
    if (csr & (FPGA_CSR_RUN_BIT | FPGA_CSR_DONE_BIT))
    {
        UART_send_buffer("ERROR: core has already been started -- toggle SW0 to reset the FPGA first\r\n");
        return;
    }

    UART_send_buffer("Loading '"); UART_send_buffer(mips_programs[idx].name); UART_send_buffer("'... ");
    print_i2c_status(FPGA_MIPS_load_program(&mips_programs[idx]));
}

static void fpga_start(char *args)
{
    (void) args;
    UART_send_buffer("Starting core... ");
    print_i2c_status(FPGA_MIPS_start());
}

static void fpga_status(char *args)
{
    (void) args;
    uint32_t csr, pc, target;
    I2C_Status st = FPGA_MIPS_read(FPGA_CSR, &csr);
    if (st == I2C_OK) st = FPGA_MIPS_read(FPGA_PC, &pc);
    if (st == I2C_OK) st = FPGA_MIPS_read(FPGA_TARGET_PC, &target);
    if (st != I2C_OK) { print_i2c_status(st); return; }

    UART_send_buffer("running=");     UART_send_buffer((csr & FPGA_CSR_RUN_BIT)  ? "1" : "0");
    UART_send_buffer("  done=");      UART_send_buffer((csr & FPGA_CSR_DONE_BIT) ? "1" : "0");
    UART_send_buffer("  PC=");        UART_send_hex32(pc);
    UART_send_buffer("  TARGET_PC="); UART_send_hex32(target);
    UART_send_buffer("\r\n");
}

static void fpga_wait(char *args)
{
    uint8_t done;
    I2C_Status st = FPGA_MIPS_wait_done(FPGA_WAIT_MAX_POLLS, &done);
    if (st != I2C_OK) { print_i2c_status(st); return; }

    UART_send_buffer(done ? "done: " : "gave up waiting: ");
    fpga_status(args);
}

// Prints words of one region, one per line: "  dmem[5] @0x2014 = 0x0000002A".
// args: "[start] [count]" in decimal; count is clamped to the end of the region.
static void dump_region(const char *name, uint16_t base, uint32_t words,
                        uint32_t default_count, char *args)
{
    uint32_t start = Console_parse_uint(args);
    char *count_arg = next_arg(args);
    uint32_t count = (*count_arg != '\0') ? Console_parse_uint(count_arg) : default_count;

    if (start >= words)
    {
        UART_send_buffer("ERROR: start is past the end of the region\r\n");
        return;
    }
    if (count > words - start) count = words - start;

    for (uint32_t k = start; k < start + count; k++)
    {
        uint16_t addr = (uint16_t) (base + 4 * k);
        uint32_t value;
        I2C_Status st = FPGA_MIPS_read(addr, &value);
        if (st != I2C_OK) { print_i2c_status(st); return; }

        UART_send_buffer("  ");  UART_send_buffer(name);
        UART_send_buffer("[");   UART_send_uint32(k);
        UART_send_buffer("] @"); UART_send_hex16(addr);
        UART_send_buffer(" = "); UART_send_hex32(value);
        UART_send_buffer("\r\n");
    }
}

static void dump_imem(char *args) { dump_region("imem", FPGA_IMEM_BASE,    FPGA_IMEM_WORDS,    16,                 args); }
static void dump_reg(char *args)  { dump_region("r",    FPGA_REGFILE_BASE, FPGA_REGFILE_WORDS, FPGA_REGFILE_WORDS, args); }
static void dump_dmem(char *args) { dump_region("dmem", FPGA_DMEM_BASE,    FPGA_DMEM_WORDS,    16,                 args); }

static const ConsoleCommand dump_sub_imem = { "imem", "instruction memory words (default 16)", 0, dump_imem };
static const ConsoleCommand dump_sub_reg  = { "reg",  "registers r0-r31 (default all 32)",     0, dump_reg  };
static const ConsoleCommand dump_sub_dmem = { "dmem", "data memory words (default 16)",        0, dump_dmem };

static const ConsoleCommand *const dump_regions[] = { &dump_sub_imem, &dump_sub_reg, &dump_sub_dmem };
#define DUMP_REGION_COUNT (sizeof(dump_regions) / sizeof(dump_regions[0]))

static void fpga_dump(char *args)
{
    if (!Console_dispatch(dump_regions, DUMP_REGION_COUNT, args))
    {
        UART_send_buffer("usage: fpga dump <region> [start] [count]\r\n");
        Console_print_help(dump_regions, DUMP_REGION_COUNT);
    }
}

static const ConsoleCommand sub_list   = { "list",   "list the embedded MIPS programs",                0, fpga_list   };
static const ConsoleCommand sub_load   = { "load",   "load program <n> over I2C (fpga load <n>)",      0, fpga_load   };
static const ConsoleCommand sub_run    = { "run",    "start the MIPS core",                            0, fpga_start  };
static const ConsoleCommand sub_status = { "status", "show running/done, PC and TARGET_PC",            0, fpga_status };
static const ConsoleCommand sub_wait   = { "wait",   "poll until done (gives up after a few seconds)", 0, fpga_wait   };
static const ConsoleCommand sub_dump   = { "dump",   "read memory back (fpga dump <imem|reg|dmem> [start] [count])", 0, fpga_dump };

static const ConsoleCommand *const subcommands[] = {
    &sub_list, &sub_load, &sub_run, &sub_status, &sub_wait, &sub_dump,
};
#define SUBCOMMAND_COUNT (sizeof(subcommands) / sizeof(subcommands[0]))

static void fpga_run(char *args)
{
    if (!Console_dispatch(subcommands, SUBCOMMAND_COUNT, args))
    {
        UART_send_buffer("usage: fpga <sub-command>\r\n");
        Console_print_help(subcommands, SUBCOMMAND_COUNT);
    }
}

const ConsoleCommand console_cmd_fpga = {
    "fpga", "MIPS core over I2C -- type 'fpga' for sub-commands", fpga_init, fpga_run
};
