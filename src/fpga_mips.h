#ifndef FPGA_MIPS_H
#define FPGA_MIPS_H
#include "stm32l4xx.h"
#include "i2c.h"

/*
 * Host-side driver for the MIPS_proc core's I2C slave (MIPS_proc/rtl_files;
 * full description in MIPS_proc/README.md).
 *
 * The slave exposes a 16-bit memory map of 32-bit words. Every I2C
 * transaction carries exactly one word, big-endian:
 *
 *   write: [0x42 W] [addr15:8] [addr7:0] [data31:24] [data23:16] [data15:8] [data7:0] STOP
 *   read:  [0x42 W] [addr15:8] [addr7:0] Sr [0x42 R] [data31:24] ... [data7:0] NACK STOP
 *
 * Run lifecycle: toggle SW0 (FPGA reset) -> load -> CSR.RUN = 1 -> DONE sets
 * once the PC reaches TARGET_PC. DONE stays set until the next FPGA reset.
 */

#define FPGA_I2C_ADDR       0x42U

// Memory map. Word k of a region lives at base + 4*k.
#define FPGA_IMEM_BASE      0x0000U  // instruction memory; address == CPU PC
#define FPGA_IMEM_WORDS     256U
#define FPGA_REGFILE_BASE   0x1000U  // r0..r31 (writes to r0 are ignored)
#define FPGA_REGFILE_WORDS  32U
#define FPGA_DMEM_BASE      0x2000U  // data memory; lw/sw index it by word
#define FPGA_DMEM_WORDS     256U
#define FPGA_CSR            0x3000U  // bit0 RUN (read/write), bit1 DONE (read-only)
#define FPGA_TARGET_PC      0x3004U  // PC of the first instruction that must not run
#define FPGA_PC             0x3008U  // live CPU PC (read-only)

#define FPGA_CSR_RUN_BIT    (1U << 0)
#define FPGA_CSR_DONE_BIT   (1U << 1)

typedef struct {
    const char *name;
    const uint8_t *instr;  // instruction bytes, big-endian, straight from tools/bin2c.py
    uint32_t instr_len;    // byte count -- multiple of 4, at most FPGA_IMEM_WORDS * 4
    const uint8_t *data;   // initial data memory bytes (DM[0], DM[1], ...), same format
    uint32_t data_len;     // byte count -- multiple of 4, at most FPGA_DMEM_WORDS * 4
} MIPS_Program;

I2C_Status FPGA_MIPS_write(uint16_t addr, uint32_t value);
I2C_Status FPGA_MIPS_read(uint16_t addr, uint32_t *value);

// Writes every instruction and data word, then TARGET_PC = instr_len (the
// address right after the last instruction). Does not start the core.
I2C_Status FPGA_MIPS_load_program(const MIPS_Program *prog);

// Writes CSR.RUN = 1.
I2C_Status FPGA_MIPS_start(void);

// Polls CSR until DONE is set, up to max_polls reads. Returns I2C_OK with
// *done = 1 if it finished, I2C_OK with *done = 0 if it gave up, or the I2C
// error if a read failed.
I2C_Status FPGA_MIPS_wait_done(uint32_t max_polls, uint8_t *done);

#endif
