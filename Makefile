PROJECT = wolf3d_stm32

BUILD_DIR = build


SRC_DIR = src
SRCS = $(shell find $(SRC_DIR)/* -path $(SRC_DIR)/wolf3d/scripts -prune -type f -o -name '*.s' -o -name '*.c')
OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o,  $(basename $(SRCS))))
SUS  = $(addprefix $(BUILD_DIR)/, $(addsuffix .su, $(basename $(SRCS))))
TARGET = $(BUILD_DIR)/$(PROJECT)

VSWAP = src/wolf3d/wl6/vswap.wl6
VSWAP_LEN = $(shell du -bs $(VSWAP) | cut -f1)

# placing the vswap file at 4MiB into the 8MiB QSPI flash
VSWAP_ADDR = 0x90400000

CROSS_COMPILE = arm-none-eabi-
DEBUGGER = openocd
OOCDCNF = openocd/devebox_stm32h743.cfg

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
GDB = $(CROSS_COMPILE)gdb

OPTFLAGS += -O3

CFLAGS += -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -mabi=aapcs
CFLAGS += -Wall -Wextra -Wl,--gc-sections
CFLAGS += -fno-common -ffunction-sections -fdata-sections -static
CFLAGS += -DSTM32H743xx -DHSE_VALUE=25000000 -DCOMPILE_DATE=`date +%s`UL
CFLAGS += -I$(SRC_DIR)/cmsis/inc -I$(SRC_DIR)/drivers -I$(SRC_DIR)/FatFS -I$(SRC_DIR)/wolf3d -I$(SRC_DIR)/
CFLAGS += -DVSWAP_LEN=$(VSWAP_LEN) -DVSWAP_ADDR=$(VSWAP_ADDR)

LDFLAGS += -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -mabi=aapcs
LDFLAGS += -lm -lgcc
LDFLAGS += -Lld

USE_QSPI = NO

ifeq ($(USE_QSPI), YES)
  CFLAGS += -DQSPI_FLASH_XIP
  LDFLAGS += -TSTM32H743XI_QSPI.ld
else
  LDFLAGS += -TSTM32H743XI.ld
endif

.PHONY: all build_debug build output info size flash flash_debug debug clean

all: build size

build: CFLAGS += $(OPTFLAGS)
build: $(TARGET).elf

build_debug: CFLAGS += -fstack-usage -ggdb3 $(OPTFLAGS)
#build_debug: CFLAGS += -finstrument-functions
#build_debug: CFLAGS += -finstrument-functions-exclude-file-list=$(SRC_DIR)/cmsis/,$(SRC_DIR)/main.c,$(SRC_DIR)/syscalls.c,$(SRC_DIR)/systick.c
build_debug: LDFLAGS += -Xlinker -Map=$(TARGET).map
build_debug: $(TARGET).elf
	$(OBJDUMP) -x -S $(TARGET).elf > $(TARGET).lst
	$(OBJDUMP) -D $(TARGET).elf > $(TARGET).dis
	$(SIZE) $(TARGET).elf > $(TARGET).size

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $+ -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(@D)
	$(AS) $+ -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $+ -o $@ $(LDFLAGS)

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

flash: build
	$(DEBUGGER) -f $(OOCDCNF) -c "program $(TARGET).elf verify reset exit"

flash_debug: build_debug
	$(DEBUGGER) -f $(OOCDCNF) -c "program $(TARGET).elf verify reset exit"

flash_vswap:
	$(DEBUGGER) -f $(OOCDCNF) -c "program $(VSWAP) $(VSWAP_ADDR) verify reset exit"

debug: build_debug flash_debug
	$(DEBUGGER) -f $(OOCDCNF) &
	$(GDB) $(TARGET).elf -ex "target remote localhost:3333" -ex "monitor reset halt"

clean:
	rm -f $(TARGET).elf
	rm -f $(TARGET).size
	rm -f $(TARGET).lst
	rm -f $(TARGET).dis
	rm -f $(TARGET).map
	rm -f $(OBJS)
	rm -f $(SUS)
