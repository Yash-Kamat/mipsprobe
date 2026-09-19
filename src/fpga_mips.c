#include "stdint.h"
#include "fpga_mips.h"

I2C_Status FPGA_MIPS_write(uint16_t addr, uint32_t value)
{
    const uint8_t rest[5] = {
        (uint8_t) addr,
        (uint8_t) (value >> 24), (uint8_t) (value >> 16),
        (uint8_t) (value >> 8),  (uint8_t) value,
    };
    // I2C1_write's "reg" byte carries the high byte of the address.
    return I2C1_write(FPGA_I2C_ADDR, (uint8_t) (addr >> 8), rest, sizeof(rest));
}

I2C_Status FPGA_MIPS_read(uint16_t addr, uint32_t *value)
{
    const uint8_t reg[2] = { (uint8_t) (addr >> 8), (uint8_t) addr };
    uint8_t b[4];

    I2C_Status st = I2C1_read(FPGA_I2C_ADDR, reg, sizeof(reg), b, sizeof(b));
    if (st != I2C_OK) return st;

    *value = ((uint32_t) b[0] << 24) | ((uint32_t) b[1] << 16) | ((uint32_t) b[2] << 8) | b[3];
    return I2C_OK;
}

// Writes `len` big-endian bytes as consecutive words, starting at `base`.
static I2C_Status write_words(uint16_t base, const uint8_t *bytes, uint32_t len)
{
    for (uint32_t i = 0; i < len; i += 4)
    {
        uint32_t word = ((uint32_t) bytes[i] << 24) | ((uint32_t) bytes[i + 1] << 16)
                      | ((uint32_t) bytes[i + 2] << 8) | bytes[i + 3];
        I2C_Status st = FPGA_MIPS_write((uint16_t) (base + i), word); // word i/4 lives at base + i
        if (st != I2C_OK) return st;
    }
    return I2C_OK;
}

I2C_Status FPGA_MIPS_load_program(const MIPS_Program *prog)
{
    if (prog == 0) return I2C_ERR_ARG;
    if ((prog->instr_len % 4) != 0 || prog->instr_len > FPGA_IMEM_WORDS * 4) return I2C_ERR_ARG;
    if ((prog->data_len % 4) != 0 || prog->data_len > FPGA_DMEM_WORDS * 4) return I2C_ERR_ARG;

    I2C_Status st = write_words(FPGA_IMEM_BASE, prog->instr, prog->instr_len);
    if (st != I2C_OK) return st;

    st = write_words(FPGA_DMEM_BASE, prog->data, prog->data_len);
    if (st != I2C_OK) return st;

    return FPGA_MIPS_write(FPGA_TARGET_PC, prog->instr_len);
}

I2C_Status FPGA_MIPS_start(void)
{
    return FPGA_MIPS_write(FPGA_CSR, FPGA_CSR_RUN_BIT);
}

I2C_Status FPGA_MIPS_wait_done(uint32_t max_polls, uint8_t *done)
{
    *done = 0;
    for (uint32_t i = 0; i < max_polls; i++)
    {
        uint32_t csr;
        I2C_Status st = FPGA_MIPS_read(FPGA_CSR, &csr);
        if (st != I2C_OK) return st;

        if (csr & FPGA_CSR_DONE_BIT)
        {
            *done = 1;
            return I2C_OK;
        }
    }
    return I2C_OK; // exhausted max_polls without seeing done -- *done stays 0
}
