# isa.inc -- instruction set of the MIPS_proc pipeline (MIPS_proc/rtl_files).
#
# The CPU uses MIPS field layouts but its own opcodes, so a stock MIPS
# assembler's output would run wrong on it. These macros emit this CPU's exact
# encodings as raw .word values instead. Full table: MIPS_proc/README.md.
#
# Operand order mirrors standard MIPS syntax:
#
#   ADD  rd, rs, rt       rd = rs + rt
#   SUB  rd, rs, rt       rd = rs - rt
#   ADDI rt, rs, imm      rt = rs + sign_extend(imm)       imm: -32768..32767
#   LW   rt, off, base    rt = DM[base + off]    off is a WORD index, not bytes
#   SW   rt, off, base    DM[base + off] = rt
#   BEQ  rs, rt, label    if (rs == rt) goto label         no delay slot
#   NOP                   0xFC000000 -- the pipeline's own bubble, writes nothing
#
# Conventions of this architecture: r0 is hardwired zero and is never a
# destination; registers reset to their own index (r8 = 8, ...).

        .set r0, 0
        .set r1, 1
        .set r2, 2
        .set r3, 3
        .set r4, 4
        .set r5, 5
        .set r6, 6
        .set r7, 7
        .set r8, 8
        .set r9, 9
        .set r10, 10
        .set r11, 11
        .set r12, 12
        .set r13, 13
        .set r14, 14
        .set r15, 15
        .set r16, 16
        .set r17, 17
        .set r18, 18
        .set r19, 19
        .set r20, 20
        .set r21, 21
        .set r22, 22
        .set r23, 23
        .set r24, 24
        .set r25, 25
        .set r26, 26
        .set r27, 27
        .set r28, 28
        .set r29, 29
        .set r30, 30
        .set r31, 31

        # R-type: opcode | rs | rt | rd | shamt | funct. The RTL decodes the
        # opcode only; funct is filled in with the standard value for readability.
        .macro ADD rd, rs, rt
        .word (0x00 << 26) | ((\rs) << 21) | ((\rt) << 16) | ((\rd) << 11) | 0x20
        .endm

        .macro SUB rd, rs, rt
        .word (0x01 << 26) | ((\rs) << 21) | ((\rt) << 16) | ((\rd) << 11) | 0x22
        .endm

        # I-type: opcode | rs | rt | imm16
        .macro ADDI rt, rs, imm
        .word (0x02 << 26) | ((\rs) << 21) | ((\rt) << 16) | ((\imm) & 0xFFFF)
        .endm

        .macro LW rt, off, base
        .word (0x03 << 26) | ((\base) << 21) | ((\rt) << 16) | ((\off) & 0xFFFF)
        .endm

        .macro SW rt, off, base
        .word (0x04 << 26) | ((\base) << 21) | ((\rt) << 16) | ((\off) & 0xFFFF)
        .endm

        # Offset is counted in instructions from PC+4, exactly like standard beq.
        .macro BEQ rs, rt, label
        .word (0x05 << 26) | ((\rs) << 21) | ((\rt) << 16) | ((((\label) - . - 4) >> 2) & 0xFFFF)
        .endm

        .macro NOP
        .word 0xFC000000
        .endm
