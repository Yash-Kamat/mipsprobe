#include "stdint.h"
#include "i2c.h"
#include "gpio.h"

// Bounds every wait-for-flag loop below. Not a calibrated timeout (no timer
// backing it), just a large enough spin count that a missing/silent slave
// (e.g. no FPGA wired up yet) faults out instead of hanging the MCU forever.
#define I2C_TIMEOUT_LOOPS 100000U

// Spin until every bit in `mask` is set in I2C1->ISR, or bail out on NACK/timeout.
static I2C_Status i2c_wait(uint32_t mask)
{
    uint32_t timeout = I2C_TIMEOUT_LOOPS;
    while ((I2C1->ISR & mask) != mask)
    {
        if (I2C1->ISR & I2C_ISR_NACKF) return I2C_ERR_NACK;
        if (--timeout == 0) return I2C_ERR_TIMEOUT;
    }
    return I2C_OK;
}

// Force the bus back to idle after an error mid-transaction.
static void i2c_abort(void)
{
    I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
    I2C1->CR2 |= I2C_CR2_STOP;

    uint32_t timeout = I2C_TIMEOUT_LOOPS;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && --timeout);

    I2C1->ICR = I2C_ICR_STOPCF;
}

void I2C1_config(uint32_t speed_hz)
{
    (void) speed_hz; // only the 100 kHz Standard-mode TIMINGR preset below is implemented right now

    // I2C1_SCL: PB6, I2C1_SDA: PB7, both AF4, open-drain with pull-ups
    // (STM32 I2C lines are always open-drain -- the bus is wired-AND).
    GPIO_manual_Config(GPIOB, GPIO_PIN_6, GPIO_MODER_Alternate, GPIO_OTYPER_Open_Drain,
                        GPIO_OSPEEDR_High, GPIO_PUPDR_Pull_Up, GPIO_AFR_AF4);
    GPIO_manual_Config(GPIOB, GPIO_PIN_7, GPIO_MODER_Alternate, GPIO_OTYPER_Open_Drain,
                        GPIO_OSPEEDR_High, GPIO_PUPDR_Pull_Up, GPIO_AFR_AF4);

    // Route I2C1's kernel clock from HSI16 (10b) rather than PCLK1, so the
    // TIMINGR value below stays correct no matter what SysClockConfig() does
    // with the PLL. HSI16 is already running -- SysClockConfig() enables it
    // as the PLL source and never turns it back off.
    RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_I2C1SEL_Msk) | RCC_CCIPR_I2C1SEL_1;

    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    I2C1->CR1 &= ~I2C_CR1_PE; // TIMINGR is only writable while PE=0

    // fI2CCLK = 16 MHz, Standard-mode (100 kHz): PRESC=0x3, SCLDEL=0x4,
    // SDADEL=0x2, SCLH=0xF, SCLL=0x13 -- from RM0394 Table 203 ("Timing
    // settings for fI2CCLK of 16 MHz"), not guessed.
    I2C1->TIMINGR = (0x3U << 28) | (0x4U << 20) | (0x2U << 16) | (0xFU << 8) | 0x13U;

    I2C1->CR1 |= I2C_CR1_PE;
}

I2C_Status I2C1_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (data == 0 && len != 0) return I2C_ERR_ARG;

    uint32_t total = (uint32_t) len + 1; // +1 for the register-address byte
    uint32_t remaining = total;
    uint32_t idx = 0; // 0 = the reg byte itself, 1..len = data[idx-1]
    uint32_t chunk = (remaining > 255) ? 255 : remaining;

    I2C1->CR2 = ((uint32_t) addr << 1)
              | (chunk << I2C_CR2_NBYTES_Pos)
              | ((remaining > 255) ? I2C_CR2_RELOAD : 0)
              | I2C_CR2_START; // RD_WRN=0 (write), AUTOEND=0 -- we STOP manually below

    while (remaining > 0)
    {
        I2C_Status st = i2c_wait(I2C_ISR_TXIS);
        if (st != I2C_OK) { i2c_abort(); return st; }

        I2C1->TXDR = (idx == 0) ? reg : data[idx - 1];
        idx++;
        remaining--;
        chunk--;

        if (chunk == 0 && remaining > 0)
        {
            // Current 255-byte chunk is queued; reload NBYTES for the rest
            // without releasing the bus (I2C v2 peripheral's RELOAD mechanism).
            st = i2c_wait(I2C_ISR_TCR);
            if (st != I2C_OK) { i2c_abort(); return st; }

            chunk = (remaining > 255) ? 255 : remaining;
            I2C1->CR2 = (I2C1->CR2 & ~(I2C_CR2_NBYTES | I2C_CR2_RELOAD))
                      | (chunk << I2C_CR2_NBYTES_Pos)
                      | ((remaining > 255) ? I2C_CR2_RELOAD : 0);
        }
    }

    I2C_Status st = i2c_wait(I2C_ISR_TC);
    if (st != I2C_OK) { i2c_abort(); return st; }

    I2C1->CR2 |= I2C_CR2_STOP;
    st = i2c_wait(I2C_ISR_STOPF);
    if (st != I2C_OK) return st;

    I2C1->ICR = I2C_ICR_STOPCF;
    return I2C_OK;
}

I2C_Status I2C1_read(uint8_t addr, const uint8_t *reg, uint8_t reg_len, uint8_t *data, uint16_t len)
{
    if (reg == 0 || reg_len == 0 || data == 0 || len == 0) return I2C_ERR_ARG;

    // Phase 1: write the reg_len-byte register pointer, no STOP (AUTOEND=0)
    // so the bus stays ours for the repeated START below.
    I2C1->CR2 = ((uint32_t) addr << 1)
              | ((uint32_t) reg_len << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_START;

    I2C_Status st;
    for (uint8_t i = 0; i < reg_len; i++)
    {
        st = i2c_wait(I2C_ISR_TXIS);
        if (st != I2C_OK) { i2c_abort(); return st; }

        I2C1->TXDR = reg[i];
    }

    st = i2c_wait(I2C_ISR_TC);
    if (st != I2C_OK) { i2c_abort(); return st; }

    // Phase 2: repeated START into read mode, chunked the same way as
    // I2C1_write, with AUTOEND on the final chunk so hardware generates the
    // STOP itself right after the last byte's ACK.
    uint32_t remaining = len;
    uint32_t idx = 0;
    uint32_t chunk = (remaining > 255) ? 255 : remaining;
    uint32_t autoend = (remaining <= 255) ? I2C_CR2_AUTOEND : 0;

    I2C1->CR2 = ((uint32_t) addr << 1)
              | I2C_CR2_RD_WRN
              | (chunk << I2C_CR2_NBYTES_Pos)
              | ((remaining > 255) ? I2C_CR2_RELOAD : 0)
              | autoend
              | I2C_CR2_START;

    while (remaining > 0)
    {
        st = i2c_wait(I2C_ISR_RXNE);
        if (st != I2C_OK) { i2c_abort(); return st; }

        data[idx++] = (uint8_t) I2C1->RXDR;
        remaining--;
        chunk--;

        if (chunk == 0 && remaining > 0)
        {
            st = i2c_wait(I2C_ISR_TCR);
            if (st != I2C_OK) { i2c_abort(); return st; }

            chunk = (remaining > 255) ? 255 : remaining;
            uint32_t next_autoend = (remaining <= 255) ? I2C_CR2_AUTOEND : 0;
            I2C1->CR2 = (I2C1->CR2 & ~(I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND))
                      | (chunk << I2C_CR2_NBYTES_Pos)
                      | ((remaining > 255) ? I2C_CR2_RELOAD : 0)
                      | next_autoend;
        }
    }

    st = i2c_wait(I2C_ISR_STOPF); // AUTOEND already triggered the STOP; just wait for it
    if (st != I2C_OK) return st;

    I2C1->ICR = I2C_ICR_STOPCF;
    return I2C_OK;
}

I2C_Status I2C1_write_byte(uint8_t addr, uint8_t reg, uint8_t byte)
{
    return I2C1_write(addr, reg, &byte, 1);
}
