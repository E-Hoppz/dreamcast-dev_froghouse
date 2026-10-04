# ================================================================
# WORLD_CLOCK Engine: Island Heat Prototype (CD-ROM Streaming)
# Project: FROGHOUSE Ecosystem (Dreamcast)
# Grade: "Mano de Oro" - Hands On Capability
# ================================================================

TARGET = island_heat.elf
OBJS = main.o

# Toolchain Helper Paths
SCRAMBLE = $(KOS_BASE)/utils/scramble/scramble
CDI4DC   = /Users/elishacaez/dreamcast-dev/img4dc/build_root/cdi4dc/cdi4dc
KOS_LINK_SCRIPT = $(KOS_BASE)/utils/ldscripts/shlelf.xc

# [REPAIRMAN'S SHUNT] Detect local IP.BIN first, fall back to KOS default
IP_BIN = $(wildcard IP.BIN)
ifeq ($(IP_BIN),)
IP_BIN = $(KOS_BASE)/kernel/arch/dreamcast/bootstrap/IP.BIN
endif

# Reach across to your compiled SHARED library structures
KOS_CFLAGS += -I../SHARED/include

# Link the Static Engine Archive (Must precede system libraries)
MY_LIBS = -L../SHARED -lworldclock \
          -L$(KOS_BASE)/lib/dreamcast \
          -L$(KOS_BASE)/addons/lib/dreamcast

# Default build outputs ELF target
all: rm-elf $(TARGET)

# Production Target: Generates bootable CDI
cdi: island_heat.cdi

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) island_heat.bin 1ST_READ.BIN island_heat.cdi island_heat.iso
	-rm -rf cdi_dir

rm-elf:
	-rm -f $(TARGET)

# [REPAIRMAN'S FIX] 
# Removed KOS_START_FILE from manual listing. Under modern KOS, -lkallisti automatically 
# pulls the '_kos_startup.o' object, native exit handlers, and memory registers on its own.
$(TARGET): $(OBJS)
	$(KOS_CC) -ml -m4-single -nostartfiles -nostdlib -Wl,-T$(KOS_LINK_SCRIPT) -Wl,--gc-sections -o $(TARGET) $(OBJS) $(MY_LIBS) $(KOS_LIBS)

# --- [ THE ENGINE PACKAGING RAIL ] ---

# Strip stack references to produce raw SH-4 machine binary
island_heat.bin: $(TARGET)
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET) island_heat.bin

# Scramble the binary in-place to allow direct bootstrap loading from CD-ROM
1ST_READ.BIN: island_heat.bin
	$(SCRAMBLE) island_heat.bin 1ST_READ.BIN

# Build ISO directory, copy cd_root files, bind Sega IP bootstrap, and run CDI converter
island_heat.cdi: 1ST_READ.BIN
	@mkdir -p cdi_dir
	@cp 1ST_READ.BIN cdi_dir/
	@# Copy all custom assets in cd_root/ directly to the ISO root
	@if [ -d cd_root ]; then \
		cp -r cd_root/* cdi_dir/; \
		echo "CD-ROM: Packed files from cd_root/ successfully to ISO root."; \
	else \
		echo "CD-ROM WARNING: cd_root/ directory not found in project folder."; \
	fi
	mkisofs -C 0,11702 -V "ISLAND_HEAT" -G $(IP_BIN) -r -J -graft-points -o island_heat.iso cdi_dir
	$(CDI4DC) island_heat.iso island_heat.cdi
	@rm -rf cdi_dir island_heat.iso
	@echo "--------------------------------------------------------"
	@echo "FROGHOUSE: island_heat.cdi manufactured successfully!"
	@echo "--------------------------------------------------------"

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
