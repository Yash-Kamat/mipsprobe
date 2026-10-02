# Assembles every program in PROGS for the MIPS_proc CPU and regenerates the
# C headers the STM32 firmware embeds:
#
#   make          assemble -> raw .bin (instr + data) -> ../src/mips_<prog>.h
#   make clean
#
# Needs only clang (built with the MIPS target) and llvm-objcopy -- no MIPS gcc.
# The custom opcodes come from the macros in isa.inc, so the output is exact.

PROGS   = full_test tt_test
CLANG   ?= clang
OBJCOPY ?= llvm-objcopy-14
PYTHON  ?= python3

BUILD   = build

all: $(PROGS:%=../src/mips_%.h)

$(BUILD)/%.o: %.S isa.inc
	@mkdir -p $(BUILD)
	$(CLANG) --target=mips-linux-gnu -x assembler -c $< -o $@

$(BUILD)/%_instr.bin: $(BUILD)/%.o
	$(OBJCOPY) -O binary -j .text $< $@

$(BUILD)/%_data.bin: $(BUILD)/%.o
	$(OBJCOPY) -O binary -j .data $< $@

../src/mips_%.h: $(BUILD)/%_instr.bin $(BUILD)/%_data.bin
	$(PYTHON) ../tools/bin2c.py $(BUILD)/$*_instr.bin $*_instr >  $@
	$(PYTHON) ../tools/bin2c.py $(BUILD)/$*_data.bin  $*_data  >> $@

.SECONDARY:

clean:
	rm -rf $(BUILD)

.PHONY: all clean
