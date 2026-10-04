# Froghouse Core: WORLD_CLOCK

TARGET = world_clock.elf
OBJS = src/main.o \
       ../SHARED/src/controls.o \
       ../SHARED/src/world_clock.o \
       ../SHARED/src/frog_math.o \
       ../SHARED/src/frog_math_3d.o \
       ../SHARED/src/frog_input.o \
       ../SHARED/src/frog_debug.o \
       ../SHARED/src/frog_debug_live.o \
       ../SHARED/src/frog_physics.o \
       ../SHARED/src/frog_config.o \
       ../SHARED/src/frog_stats.o \
       ../SHARED/src/frog_util.o \
       ../SHARED/src/frog_time.o \
       ../SHARED/src/frog_audio.o \
       
# NEW: This tells the compiler to look in your 'include' cabinet for blueprints
KOS_CFLAGS += -Iinclude -I../SHARED/include

# Point to KOS (The Dreamcast Operating System)
include $(KOS_BASE)/Makefile.rules

all: rm-elf $(TARGET)

# This packs everything in your 'assets' folder onto the Dreamcast disc
KOS_ROMDISK_DIR = assets

include $(KOS_BASE)/Makefile.prefab

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lkallisti

clean:
	rm -f $(TARGET) $(OBJS)

rm-elf:
	rm -f $(TARGET)
