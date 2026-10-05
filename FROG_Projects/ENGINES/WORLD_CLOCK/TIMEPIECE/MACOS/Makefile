# Target executable name
TARGET = TIMEPIECE_Editor

# --- [ VIRTUAL SEARCH PATHS ] ---
vpath %.c ../../SHARED/src
vpath %.cpp . ./imgui ./imgui/backends

# Include paths targeting your SHARED/include directory and local shims
INCLUDES = -I../../SHARED/include -I./host_compat -I./imgui -I./imgui/backends

# Compiler configurations
CC = clang
CXX = clang++

# Host compilation flags (injects shims globally with C++17 standard)
COMMON_FLAGS = $(INCLUDES) -include ./host_compat/desktop_shim.h -DHOST_BUILD -g -O2
SDL2_FLAGS = $(shell sdl2-config --cflags)
SDL2_LIBS = $(shell sdl2-config --libs)

CFLAGS = $(COMMON_FLAGS) $(SDL2_FLAGS) -std=c11
CXXFLAGS = $(COMMON_FLAGS) $(SDL2_FLAGS) -std=c++17
LIBS = $(SDL2_LIBS) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# Local object files list (Links your three local modular files)
OBJS = frog_math_3d.o \
       frog_matrix.o \
       frog_collision_3d.o \
       frog_physics.o \
       frog_math.o \
       frog_state.o \
       frog_util.o \
       frog_air_signal.o \
       imgui.o \
       imgui_draw.o \
       imgui_tables.o \
       imgui_widgets.o \
       imgui_impl_sdl2.o \
       imgui_impl_opengl2.o \
       timepiece_clock.o \
       timepiece_assets.o \
       main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

# Compiles parent C sources locally
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compiles C++ sources locally
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean
