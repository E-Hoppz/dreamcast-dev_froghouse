# Xash3D FWGS Engine Dreamcast port <img align="right" width="128" height="128" src="https://github.com/FWGS/xash3d-fwgs/raw/master/game_launch/icon-xash-material.png" alt="Xash3D FWGS icon" />

Xash3D (pronounced `[ksɑʂ]`) FWGS is a game engine, aimed to provide compatibility with Half-Life Engine and extend it. This is a Dreamcast port of the engine.

## Prerequisites

1. KallistiOS (KOS) development environment
2. GLDC
3. FatFs
4. libbz2

## Repository Setup

1. Create a development directory (e.g., `Xash3D_DC`)
2. Clone the following repositories in the same directory:
   ```
   git clone https://github.com/maximqaxd/xash3d-fwgs_dc
   ```

## Required Modifications

1. Modify KOS `include/kos/opts.h`:
   ```c
   // Change this line
   #define FS_CD_MAX_FILES 48
   ```
2. Modify KOS environ.sh:
   ```c
   # Optimization Level
   #
   # Controls the baseline optimization level to use when building.
   # Typically this is -Og (debugging), -O0, -O1, -O2, or -O3.
   # NOTE: For our target, -O4 is a valid optimization level that has
   # been seen to have some performance impact as well.
   #
   //set 
   export KOS_CFLAGS="${KOS_CFLAGS} -Os"
   # Stack Protector
   #
   # Controls whether GCC emits extra code to check for buffer overflows or stack
   # smashing, which can be very useful for debugging. -fstack-protector only
   # covers vulnerable objects, while -fstack-protector-strong provides medium
   # coverage, and -fstack-protector-all provides full coverage. You may also
   # override the default stack excepton handler by providing your own
   # implementation of "void __stack_chk_fail(void)."
   #
   // disable stack protector
   #export KOS_CFLAGS="${KOS_CFLAGS} -fstack-protector-all"
   ```
3. Rebuild KOS

## Building Dependencies

1. Build GLDC:
    ```c
   # Modify texture.c line 1700:
   Replace FASTCPY(targetData, conversionBuffer, destBytes);
   With memcpy(targetData, conversionBuffer, destBytes);
   # In include/GL/glext.h
   Remove or comment #define GL_TEXTURE_LOD_BIAS 0x8501
    # In GL/config.h
   replace #define MAX_TEXTURE_COUNT to 1536
   ```

## Building Xash3D

This will build:
- Filesystem
- GL renderer
- Main engine (1ST_READ.BIN)
- HLSDK

## Installation & Running

1. Copy engine binaries to a directory
2. Copy Half-Life game data (contents of valve folder) to the build/valve directory don't overwrite when prompted.
3. Build the engine and all components (make all)
4. Run on your Dreamcast

## Hardware Requirements

Currently we support multiplayer. It requires BBA. Modem tested, but seems it won't work for now.

## Support Development

You can support the port development for acquiring dev tools at https://boosty.to/maximqad

## Notes

- This is a work in progress
- Some features may be unstable or missing
- Performance optimizations are ongoing

## Credits

- Original Xash3D Engine by Uncle Mike
- FWGS team for Xash3D FWGS fork
- Kazade for GLdc
- KallistiOS team
