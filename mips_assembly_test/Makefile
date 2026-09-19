# Assembles full_test.S for the MIPS_proc CPU and regenerates the C header the
# STM32 firmware embeds:
#
#   make          assemble -> raw .bin (instr + data) -> ../src/mips_full_test.h
#   make clean
#
# Needs only clang (built with the MIPS target) and llvm-objcopy -- no MIPS gcc.
# The custom opcodes come from the macros in isa.inc, so the output is exact.

PROG    = full_test
CLANG   ?= clang
OBJCOPY ?= llvm-objcopy-14
PYTHON  ?= python3

BUILD   = build
HEADER  = ../src/mips_$(PROG).h

all: $(HEADER)

$(BUILD)/$(PROG).o: $(PROG).S isa.inc
	@mkdir -p $(BUILD)
	$(CLANG) --target=mips-linux-gnu -x assembler -c $< -o $@

$(BUILD)/$(PROG)_instr.bin: $(BUILD)/$(PROG).o
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/$(PROG)_data.bin: $(BUILD)/$(PROG).o
	$(OBJCOPY) -O binary -j .data $< $@

$(HEADER): $(BUILD)/$(PROG)_instr.bin $(BUILD)/$(PROG)_data.bin
	$(PYTHON) ../tools/bin2c.py $(BUILD)/$(PROG)_instr.bin $(PROG)_instr >  $@
	$(PYTHON) ../tools/bin2c.py $(BUILD)/$(PROG)_data.bin  $(PROG)_data  >> $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean
