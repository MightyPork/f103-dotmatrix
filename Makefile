################################################################

ARCH_FLAGS     = -msoft-float -mfloat-abi=soft
ARCH_FLAGS    += -mthumb -mcpu=cortex-m3

# Clock speed constants
DEFS          += -DF_CPU=72000000UL

DEFS          += -DSTM32F10X_MD
DEFS          += -DARM_MATH_CM3
DEFS          += -DUSE_STDPERIPH_DRIVER

LDSCRIPT       = lib/cmsis/stm32_flash.ld
STARTUP_SCRIPT = lib/cmsis/startup_stm32f10x_md.s


################################################################

# Main file
BINARY      = main

# Project include dirs
INCL_DIRS  := project

# C files
SOURCES   := $(shell find project -type f -name "*.c")
#SOURCES   += $(shell find lib/sbmp/library -type f -name "*.c")

HEADERS   := $(shell find project -type f -name "*.h")
#HEADERS   += $(shell find lib/sbmp/library -type f -name "*.h")

#INCL_DIRS += lib/sbmp/library

################################################################

# Add library sources and include paths
LIB_INCL_DIRS =
LIB_SOURCES =
include lib/CMSIS.mk
include lib/SPL.mk
include lib/SBMP.mk

INCL_DIRS += $(LIB_INCL_DIRS)

LIB_OBJS_ = $(LIB_SOURCES:.c=.o)
LIB_OBJS = $(LIB_OBJS_:.S=.o)

# .c -> .o

OBJS = $(SOURCES:%.c=%.o)
OBJS += $(STARTUP_SCRIPT:.s=.o)

LIBNAME = stm32f103spl
LIBFILE = lib/lib$(LIBNAME).a


################################################################

SUBDIR_ROOTS = .
GARBAGE_PATTERNS := *.o *.d *.elf *.bin *.hex *.srec *.list *.map *.dis *.disasm .depend

DIRS := . $(shell find $(SUBDIR_ROOTS) -type d)
GARBAGE := $(foreach DIR,$(DIRS),$(addprefix $(DIR)/,$(GARBAGE_PATTERNS)))

###############################################################################
###############################################################################
###############################################################################

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
  Q     := @
  NULL  := 2>/dev/null
endif

###############################################################################
# Executables

PREFIX  ?= arm-none-eabi

CC      := $(PREFIX)-gcc
CXX     := $(PREFIX)-g++
LD      := $(PREFIX)-gcc
AR      := $(PREFIX)-ar
AS      := $(PREFIX)-as
OBJCOPY := $(PREFIX)-objcopy
SIZE    := $(PREFIX)-size
OBJDUMP := $(PREFIX)-objdump
GDB     := $(PREFIX)-gdb
STFLASH := $(shell which st-flash)

###############################################################################

# For CMSIS compatibility
DEFS          += -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -D__STATIC_INLINE="static inline"
DEFS          += -DVERBOSE_LOGGING=1

###############################################################################
# C flags

CFLAGS      += -Os -ggdb -std=gnu99 -Wfatal-errors
CFLAGS      += -Wall -Wextra -Wshadow
CFLAGS      += -Wwrite-strings -Wold-style-definition -Winline -Wmissing-noreturn -Wstrict-prototypes
CFLAGS      += -Wredundant-decls -Wfloat-equal -Wsign-compare
CFLAGS      += -fno-common -ffunction-sections -fdata-sections -Wunused-function
CFLAGS      += -MD -Wno-format-zero-length
CFLAGS      += $(INCL_DIRS:%=-I%) $(DEFS)
#-flto

# Special flags to hide warnings in CMSIS
LIB_CFLAGS   = -Wno-shadow -Wno-float-equal -Wno-inline -Wno-unused-parameter -Wno-unused
#-Wno-old-style-definition -Wno-strict-prototypes

###############################################################################
# Linker flags

LDFLAGS     += --static -lm -lc -nostartfiles  
LDFLAGS     += -Llib
LDFLAGS     += -T$(LDSCRIPT)
LDFLAGS     += -Wl,-Map=$(*).map
LDFLAGS     += -Wl,--gc-sections
#-specs=nano.specs

###############################################################################
# Used libraries

LDLIBS		+= -lm -lc -l$(LIBNAME)
LDLIBS		+= -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

###############################################################################
###############################################################################
###############################################################################

.PHONY: all images clean elf bin hex srec list dis ttyusb ttyacm lib libcheck size

.SUFFIXES: .elf .bin .hex .srec .list .map .images
.SECONDEXPANSION:
.SECONDARY:

all: elf size

elf: $(BINARY).elf
bin: $(BINARY).bin
hex: $(BINARY).hex
srec: $(BINARY).srec
list: $(BINARY).list

images: $(BINARY).images
flash: $(BINARY).flash

dis: $(BINARY).elf
	$(Q)$(OBJDUMP) -dS $(BINARY).elf -j .text -j .isr_vector > $(BINARY).dis

lib: CFLAGS += $(LIB_CFLAGS)
lib: $(LIBFILE)

$(LIBFILE): $(LIB_OBJS)
	$(Q)$(AR) rcsv $@ $(LIB_OBJS)

libcheck:
	$(Q)if [ ! -e $(LIBFILE) ]; then \
	  echo "--- You must build the lib first! ---"; \
	  exit 1; \
	fi

size: $(BINARY).elf
	$(Q)$(SIZE) -A $(BINARY).elf | grep -v "    0"

%.images: %.bin %.hex %.srec %.list %.map

%.bin: %.elf
	$(Q)$(OBJCOPY) -Obinary $(*).elf $(*).bin

%.hex: %.elf
	$(Q)$(OBJCOPY) -Oihex $(*).elf $(*).hex

%.srec: %.elf
	$(Q)$(OBJCOPY) -Osrec $(*).elf $(*).srec

%.list: %.elf
	$(Q)$(OBJDUMP) -S $(*).elf > $(*).list

%.elf %.map: libcheck $(OBJS) $(HEADERS)
	$(Q)$(LD) $(LDFLAGS) $(ARCH_FLAGS) $(OBJS) $(LDLIBS) -o $(*).elf
	$(Q)./parsemap.php
	$(Q)$(SIZE) $(*).elf

%.o: %.c
	$(Q)echo "CC $(*).c"
	$(Q)$(CC) $(CFLAGS) $(ARCH_FLAGS) -o $(*).o -c $(*).c

%.o: %.s
	$(Q)echo "CC $(*).s"
	$(Q)$(CC) $(CFLAGS) $(ARCH_FLAGS) -o $(*).o -c $(*).s

%.o: %.S
	$(Q)echo "CC $(*).S"
	$(Q)$(CC) $(CFLAGS) $(ARCH_FLAGS) -o $(*).o -c $(*).S

clean:
	$(Q)$(RM) -r $(GARBAGE)

%.flash: %.bin
	@printf "  FLASH  $<\n"
	$(Q)$(STFLASH) write $(*).bin 0x8000000

-include $(OBJS:.o=.d)
