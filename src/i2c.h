#ifndef I2C_H
#define I2C_H
#include "stdint.h"
#include "stm32l4xx.h"

/*
 * I2C1 bare-metal master driver (I2C1_SCL: PB6, I2C1_SDA: PB7, both AF4,
 * open-drain with pull-ups enabled).
 *
 * Polling-based, matching the style of uart.c/timer.c in this project --
 * every wait loop is bounded by I2C_TIMEOUT_LOOPS so a disconnected/silent
 * slave (e.g. no FPGA wired up yet) can't hang the MCU forever.
 */

typedef enum {
    I2C_OK = 0,
    I2C_ERR_NACK,       // slave didn't ACK its address or a data byte
    I2C_ERR_TIMEOUT,    // bus never went ready / flag never set (no slave present, wiring fault, etc.)
    I2C_ERR_ARG,        // bad argument (e.g. zero-length transfer)
} I2C_Status;

// speed_hz: bus clock, e.g. 100000 for Standard-mode, 400000 for Fast-mode.
// Only 100 kHz is implemented right now (see i2c.c) -- extend I2C1_config's
// TIMINGR table if you need Fast-mode later.
void I2C1_config(uint32_t speed_hz);

// Single transaction: START, addr+W, reg, data[0..len-1], STOP.
// `len` may exceed 255 -- internally chunked via the I2C peripheral's
// NBYTES-reload mechanism, so the whole thing still happens as one
// uninterrupted transaction from the slave's point of view.
I2C_Status I2C1_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len);

// Write the `reg_len`-byte register pointer `reg` (no STOP), then a
// repeated-START read of `len` bytes.
I2C_Status I2C1_read(uint8_t addr, const uint8_t *reg, uint8_t reg_len, uint8_t *data, uint16_t len);

I2C_Status I2C1_write_byte(uint8_t addr, uint8_t reg, uint8_t byte);

#endif
