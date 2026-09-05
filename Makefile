TARGET = firmware
BUILD_DIR = build
ST_FREQ = 950

# Which demo in examples/ provides main(). Override on the command line,
# e.g. `make DEMO=usb_demo` or `make DEMO=usb_demo flash`.
DEMO ?= scheduler_demo

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

ifeq ($(wildcard examples/$(DEMO).c),)
$(error Unknown DEMO "$(DEMO)". Available demos: $(patsubst examples/%.c,%,$(wildcard examples/*.c)))
endif

MCUFLAGS = \
-mcpu=cortex-m4 \
-mthumb

CFLAGS = \
$(MCUFLAGS) \
-O0 \
-g3 \
-Wall \
-ffunction-sections \
-fdata-sections \
-DSTM32L432xx \
-Ivendor/CMSIS/CMSIS/Core/Include \
-Ivendor/STM32L4_Device/Include \
-Isrc \
-Isrc/usb

LDFLAGS = \
-Tlinker/STM32L432KC_FLASH.ld \
-Wl,--gc-sections \
-Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
-specs=nano.specs

SRC_C := $(wildcard src/*.c)
SRC_C += $(wildcard src/usb/*.c)
SRC_C += vendor/STM32L4_Device/Source/Templates/system_stm32l4xx.c
SRC_C += examples/$(DEMO).c

SRC_S := startup/startup_stm32l432xx.s

OBJ_C := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_C))
OBJ_S := $(patsubst %.s,$(BUILD_DIR)/%.o,$(SRC_S))

OBJ := $(OBJ_C) $(OBJ_S)

ELF = $(BUILD_DIR)/$(TARGET).elf
BIN = $(BUILD_DIR)/$(TARGET).bin

all: $(ELF)

$(ELF): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) $(MCUFLAGS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(MCUFLAGS) -c $< -o $@

flash: $(BIN)
	st-flash --freq $(ST_FREQ) --reset --connect-under-reset write $(BIN) 0x08000000

list-demos:
	@echo "Available demos (make DEMO=<name> ...):"
	@for f in examples/*.c; do echo "  $$(basename $$f .c)"; done

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all flash clean list-demos