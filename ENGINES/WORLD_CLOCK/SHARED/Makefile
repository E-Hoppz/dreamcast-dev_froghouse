# ================================================================
# COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
# WORLD_CLOCK Engine: The Static Library & Standalone Makefile
# Project: FROGHOUSE Ecosystem (Dreamcast)
# Grade: "Mano de Oro" - Hands On Capability
# ================================================================

# Ensure KallistiOS environment is sourced before attempting compilation.
ifndef KOS_BASE
$(error Please source your KallistiOS environ.sh script first!)
endif

TARGET_LIB = libworldclock.a
TARGET_ELF = world_clock.elf

# --- [ THE ENGINE CORE ] ---
CORE_OBJS = src/world_clock.o \
            src/frog_state.o \
            src/frog_time.o \
            src/frog_util.o \
            src/frog_config.o \
            src/frog_pool.o \
            src/frog_loader.o \
            src/frog_data.o \
            src/frog_scene.o \
            src/frog_midi.o \
            src/worldclock_forall.o \
            src/frog_vfs.o         # NEW: Added local virtual file pack reader

# --- [ SYSTEM, STORAGE & PERIPHERALS ] ---
SYS_OBJS  = src/frog_audio.o \
            src/frog_input.o \
            src/frog_input_gun.o \
            src/frog_clockwork_bridge.o \
            src/frog_net.o \
            src/frog_debug.o \
            src/frog_debug_live.o \
            src/frog_vmu.o \
            src/frog_stats.o \
            src/frog_scratchpad.o

# --- [ MATHEMATICAL & PHYSICAL RAILS ] ---
MATH_OBJS = src/frog_collision_3d.o \
            src/frog_physics.o \
            src/frog_math_3d.o \
            src/frog_animate.o \
            src/frog_particle.o \
            src/frog_sq.o \
            src/frog_air_signal.o \
            src/frog_matrix.o \
            src/frog_math.o \
            src/frog_sweep.o       # NEW: Added swept-aabb collision trace calculations

# --- [ GRAPHICS & VIEWPORT ] ---
GFX_OBJS  = src/frog_draw.o \
            src/frog_cam.o \
            src/frog_font.o \
            src/frog_batch.o \
            src/frog_mesh.o \
            src/frog_color.o \
            src/frog_tile.o \
            src/frog_modifier.o \
            src/frog_vmu_visuals.o \
            src/frog_occlude.o \
            src/frog_sh_light.o \
            src/frog_portal.o      # NEW: Added portal graph culling

# Combine static library components
OBJS = $(CORE_OBJS) $(SYS_OBJS) $(MATH_OBJS) $(GFX_OBJS)
OBJS += src/frog_entity.o src/frog_asset.o

KOS_CFLAGS += -I./include -O3 -flto -ffast-math -mfsrra -mfsca -g

# Default target compiles the clean static library (excluding main)
all: $(TARGET_LIB)

# Library Compilation Target
$(TARGET_LIB): $(OBJS)
	$(KOS_AR) rcs $(TARGET_LIB) $(OBJS)
	@echo "------------------------------------------------"
	@echo "FROGHOUSE: [$(TARGET_LIB)] Manufactured Successfully"
	@echo "System: Dreamcast World Clock v1.1"
	@echo "Status: ALL RAILS ENERGIZED - READY FOR THE LAIR"
	@echo "------------------------------------------------"

# Standalone Sovereign Executable Target
# Compiles main and links it against the newly compiled static library
standalone: $(TARGET_LIB) src/frog_main.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET_ELF) $(KOS_START) \
		src/frog_main.o $(TARGET_LIB) $(KOS_LIBS)
	@echo "------------------------------------------------"
	@echo "FROGHOUSE: [$(TARGET_ELF)] Compiled Successfully"
	@echo "Mode: Standalone Sovereign Game Host"
	@echo "Status: BOOT RAILS ENERGIZED"
	@echo "------------------------------------------------"

clean:
	rm -f $(OBJS) src/frog_main.o $(TARGET_LIB) $(TARGET_ELF)

src/%.o: src/%.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -c $< -o $@
