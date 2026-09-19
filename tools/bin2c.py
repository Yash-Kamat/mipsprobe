#!/usr/bin/env python3
"""
Converts a raw binary file (e.g. a MIPS assembler's raw .bin output) into a C
byte array, for dropping into src/mips_programs.c as a MIPS_Program's
`instr`/`data` field.

Usage:
    python3 tools/bin2c.py <input.bin> <c_identifier> > snippet.h

Byte order is NOT touched here -- whatever order your assembler wrote the
.bin in is what ends up in the array, in file order. The MIPS_proc core is
big-endian (first byte = MSB of each 32-bit word), so assemble for a
big-endian target, as mips_assembly_test/Makefile does.
"""
import sys


def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <input.bin> <c_identifier>", file=sys.stderr)
        sys.exit(1)

    in_path, ident = sys.argv[1], sys.argv[2]
    with open(in_path, "rb") as f:
        data = f.read()

    if len(data) % 4 != 0:
        print(f"warning: {in_path} is {len(data)} bytes, not a multiple of 4 -- "
              f"FPGA_MIPS_load_program() will reject this", file=sys.stderr)

    print(f"// Generated from {in_path} ({len(data)} bytes, {len(data) // 4} words) by tools/bin2c.py")
    print(f"static const uint8_t {ident}[] = {{")
    for i in range(0, len(data), 12):
        row = data[i:i + 12]
        print("    " + ", ".join(f"0x{b:02X}" for b in row) + ",")
    print("};")


if __name__ == "__main__":
    main()
