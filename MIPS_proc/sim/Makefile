# Icarus Verilog regression for the MIPS core.
#
#   make          build and run; prints "PASS" or "FAIL" and exits non-zero on failure
#   make clean
#
# The testbench drives scl/sda as an I2C master exactly the way src/fpga_mips.c
# does, loads mips_assembly_test/full_test, runs it to DONE and checks every
# result documented in ../README.md.

RTL     := $(wildcard ../rtl_files/*.v)
PROGDIR := ../../mips_assembly_test/build
HEX     := full_test_instr.hex full_test_data.hex

all: run

# The .bin files are raw big-endian 32-bit words; od emits them byte by byte so
# the hex stays big-endian regardless of host endianness.
%.hex: $(PROGDIR)/%.bin
	od -An -tx1 -w4 -v $< | tr -d ' ' > $@

$(PROGDIR)/%.bin:
	$(MAKE) -C ../../mips_assembly_test

# Word counts come from the program itself, so this never goes stale when
# full_test.S grows. TARGET_PC is 4 x the instruction count, as fpga_mips.c does.
program.vh: $(HEX)
	@printf '`define INSTR_WORDS %s\n' "$$(wc -l < full_test_instr.hex)"  > $@
	@printf '`define DATA_WORDS %s\n'  "$$(wc -l < full_test_data.hex)"  >> $@

sim.vvp: tb_top.v $(RTL) program.vh
	iverilog -g2005 -Wall -o $@ -I. tb_top.v $(RTL)

run: sim.vvp $(HEX)
	@vvp $<

clean:
	rm -f sim.vvp *.hex program.vh *.vcd

.PHONY: all run clean
