# tt_test.S -- small test that fits the Tiny Tapeout-sized core
# (IMEM 16 words, DMEM 8 words; uses only r0-r6, so it also runs on the
# 16-register variant). Also runs unchanged on the full-size FPGA core.
#
# Build:  make            (regenerates ../src/mips_tt_test.h)
# Run:    SW0 on -> off (reset the FPGA), then in the STM32 console:
#           fpga list                  (find tt_test's index)
#           fpga load <n>
#           fpga run
#           fpga wait
#           fpga dump dmem 0 8
#           fpga dump r
#
# Expected (not self-checking -- compare by eye):
#   DM[0]=3 DM[1]=10 (preloaded)  DM[2]=6  DM[3]=4  DM[4]=8
#   r1=0  r2=10  r3=6  r4=4  r5=4  r6=8   PC == TARGET_PC (0x34)
#
# Covers: lw of STM32-preloaded data, addi with a negative immediate, add,
# sub, EX/MEM and MEM/WB forwarding, beq taken and not taken (loop), sw,
# and a load-use stall. Does not rely on registers resetting to any value.

        .include "isa.inc"

        .text
        LW   r1, 0, r0          # r1 = DM[0] = 3   (loop count)
        LW   r2, 1, r0          # r2 = DM[1] = 10
        ADDI r3, r0, 0          # r3 = 0           (accumulator)
loop:
        ADD  r3, r3, r1         # r3 += r1         -> 3, 5, 6
        ADDI r1, r1, -1         # r1 -= 1          -> 2, 1, 0
        BEQ  r1, r0, done       # not taken twice, taken once
        BEQ  r0, r0, loop       # taken twice
done:
        SUB  r4, r2, r3         # r4 = 10 - 6 = 4
        SW   r3, 2, r0          # DM[2] = 6
        SW   r4, 3, r0          # DM[3] = 4
        LW   r5, 3, r0          # r5 = 4
        ADD  r6, r5, r5         # load-use stall: r6 = 8
        SW   r6, 4, r0          # DM[4] = 8

        .data
        .word 3                 # DM[0]
        .word 10                # DM[1]
