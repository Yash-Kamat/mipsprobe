#ifndef CONSOLE_CMD_FPGA_H
#define CONSOLE_CMD_FPGA_H

#include "console_cmd.h"

// 'fpga <list|load|run|status|wait>' -- drives the FPGA-hosted MIPS core over
// I2C1 (PB6=SCL, PB7=SDA). Its init() configures I2C1.
extern const ConsoleCommand console_cmd_fpga;

#endif
