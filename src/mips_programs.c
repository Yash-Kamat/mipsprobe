#include "mips_programs.h"

/*
 * Programs the console can load onto the FPGA (`fpga list`, `fpga load <n>`).
 *
 * Each one is generated from assembly in mips_assembly_test/ -- run `make`
 * there after editing a .S file to regenerate its header.
 */
#include "mips_full_test.h"
#include "mips_tt_test.h"

const MIPS_Program mips_programs[] = {
    {
        .name = "full_test",
        .instr = full_test_instr,
        .instr_len = sizeof(full_test_instr),
        .data = full_test_data,
        .data_len = sizeof(full_test_data),
    },
    {
        .name = "tt_test",
        .instr = tt_test_instr,
        .instr_len = sizeof(tt_test_instr),
        .data = tt_test_data,
        .data_len = sizeof(tt_test_data),
    },
};

const uint32_t mips_program_count = sizeof(mips_programs) / sizeof(mips_programs[0]);
