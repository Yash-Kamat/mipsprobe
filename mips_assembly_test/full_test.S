# full_test.S -- self-checking test for the MIPS_proc pipeline.
#
# Build:  make            (regenerates ../src/mips_full_test.h)
# Run:    SW0 on -> off (reset the FPGA), then in the STM32 console:
#           fpga load 0
#           fpga run
#           fpga wait
#
# Results, all readable with `fpga dump dmem <start> <count>`:
#   DM[255]      1 = every check passed, 0xFFFFFFFF = a check failed
#   DM[254]      id of the failing check (only written on failure)
#   DM[33..45]   value computed by check 1..13 (DM[32 + id])
#
# PASS also means DONE is set (LD0 lit) and `fpga status` shows PC == TARGET_PC.
# A failed check spins in `hang` forever, so DONE never sets and `fpga wait`
# times out.
#
# DM[0..12] are preloaded by the STM32 from the .data section below, which
# also proves the I2C DMEM programming path works.

        .include "isa.inc"

        # Records `got` at DM[32+id], then falls through only if got == exp.
        .macro CHECK id, got, exp
        ADDI r23, r0, \id
        SW   \got, 32 + \id, r0
        BEQ  \got, \exp, 1f
        BEQ  r0, r0, fail
1:
        .endm

        .text
start:
        BEQ  r0, r0, main               # skip over the fail handler

fail:
        ADDI r8, r0, -1
        SW   r8, 255, r0                # DM[255] = FAIL
        SW   r23, 254, r0               # DM[254] = failing check id
hang:
        BEQ  r0, r0, hang               # never reaches TARGET_PC
        NOP

main:
# 1: addi, and lw of a word the STM32 preloaded into DMEM (DM[0] = 5)
        ADDI r8, r0, 5
        LW   r9, 0, r0
        CHECK 1, r8, r9

# 2: negative immediates are sign-extended (DM[2] = -3)
        ADDI r10, r0, -3
        LW   r11, 2, r0
        CHECK 2, r10, r11

# 3: add  (5 + -3 = 2)
        ADD  r12, r8, r10
        ADDI r13, r0, 2
        CHECK 3, r12, r13

# 4: sub  (5 - -3 = 8)
        SUB  r12, r8, r10
        ADDI r13, r0, 8
        CHECK 4, r12, r13

# 5: full 32-bit sw/lw round trip through DM[60] (DM[4] = 0x12345678, which
#    no 16-bit immediate can build)
        LW   r14, 4, r0
        NOP
        SW   r14, 60, r0
        NOP
        LW   r15, 60, r0
        CHECK 5, r14, r15

# 6: EX/MEM forwarding -- r8 is used by the very next instruction
        ADDI r8, r0, 10
        ADD  r9, r8, r8
        ADDI r10, r0, 20
        CHECK 6, r9, r10

# 7: MEM/WB forwarding -- one instruction between the write and the use
        ADDI r8, r0, 3
        NOP
        ADD  r9, r8, r8
        ADDI r10, r0, 6
        CHECK 7, r9, r10

# 8: regfile write-before-read bypass -- r8 is written in WB while the ADD
#    reading it sits in ID
        ADDI r8, r0, 4
        NOP
        NOP
        ADD  r9, r8, r8
        ADDI r10, r0, 8
        CHECK 8, r9, r10

# 9: load-use stall -- the loaded value is used by the very next instruction
        LW   r8, 0, r0
        ADD  r9, r8, r8
        ADDI r10, r0, 10
        CHECK 9, r9, r10

# 10: beq not taken -- 1 != 2 must fall through
        ADDI r8, r0, 1
        ADDI r9, r0, 2
        ADDI r23, r0, 10
        BEQ  r8, r9, fail
        SW   r8, 42, r0                 # DM[42] = 1: we got past the branch

# 11: beq taken -- the two instructions behind it must never execute
#     (this CPU has no branch delay slot)
        ADDI r8, r0, 7
        ADDI r9, r0, 7
        BEQ  r8, r9, taken11
        ADDI r8, r0, 99                 # wrong path
        ADDI r8, r0, 98                 # wrong path
taken11:
        CHECK 11, r8, r9

# 12: backward loop -- sum DM[8..12] = 1+2+3+4+5 and compare with DM[3] = 15.
#     Exercises the branch predictor, flushes, and a load-use stall per pass.
        ADDI r8, r0, 0                  # sum
        ADDI r9, r0, 8                  # index
        ADDI r10, r0, 13                # one past the last index
loop12:
        LW   r11, 0, r9
        ADD  r8, r8, r11
        ADDI r9, r9, 1
        BEQ  r9, r10, done12
        BEQ  r0, r0, loop12
done12:
        LW   r12, 3, r0
        CHECK 12, r8, r12

# 13: sw/lw through a base register plus offset (DM[20 + 10] = DM[30])
        ADDI r8, r0, 20
        ADDI r9, r0, 0x55
        SW   r9, 10, r8
        LW   r10, 30, r0
        CHECK 13, r9, r10

# Every check passed. End with two non-branch instructions: the loader sets
# TARGET_PC to the address right after the last one, and the stop drain
# starts the cycle the PC gets there.
pass:
        ADDI r8, r0, 1
        ADDI r31, r0, 6767
        SW   r31, 212, r0
        SW   r8, 255, r0                # DM[255] = PASS
        NOP
        NOP

        .data
        .word 5                         # DM[0]
        .word 0                         # DM[1]
        .word -3                        # DM[2]
        .word 15                        # DM[3]   expected sum for check 12
        .word 0x12345678                # DM[4]
        .word 0, 0, 0                   # DM[5..7]
        .word 1, 2, 3, 4, 5             # DM[8..12]
