# Software Build Setup

This folder now contains a Dear ImGui application using SDL2 (platform layer) and Vulkan (renderer).

## Project Layout

- `application/` → ImGui application source (`main.cpp`) and app target CMake file
- `cmake/toolchains/rpi4-aarch64.cmake` → Raspberry Pi 4 cross-compilation toolchain file
- `CMakeLists.txt` → Software root CMake entrypoint
- `CMakePresets.json` → Ready-to-use build presets

## Prerequisites

### Windows build (native)

- CMake 3.24+
- Ninja
- MSVC toolchain (Visual Studio Build Tools)
- Vulkan SDK installed (for Vulkan headers/libraries)

### Raspberry Pi 4 build (aarch64 cross-compile)

- CMake 3.24+
- Ninja
- `aarch64-linux-gnu-gcc` and `aarch64-linux-gnu-g++` (cross-compiler toolchain)
- A Raspberry Pi sysroot containing target SDL2/Vulkan dependencies

**Installing the aarch64 cross-compiler toolchain:**

On **Windows** (using MSYS2/MinGW):
```powershell
pacman -S mingw-w64-x86_64-arm-none-eabi-binutils mingw-w64-x86_64-aarch64-linux-gnu-gcc
```

Or download pre-built toolchains:
- https://github.com/abhiTronix/raspberry-pi-cross-compiler/releases
- https://releases.linaro.org/components/toolchain/binaries/

Ensure the toolchain `bin/` directory is in your PATH.

Once installed, configure with optional sysroot:

```powershell
cmake --preset rpi4-aarch64 -DRPI_SYSROOT="C:/path/to/rpi/sysroot"
```

Or use the VS Code task (will prompt for sysroot path).

## Build Commands

From `software/`:

### Configure + build for Windows

```powershell
cmake --preset windows-msvc
cmake --build --preset build-windows-msvc
```

### Configure + build for Raspberry Pi 4 (aarch64)

```powershell
cmake --preset rpi4-aarch64 -DRPI_SYSROOT="C:/path/to/rpi/sysroot"
cmake --build --preset build-rpi4-aarch64
```

## Notes

- Dependencies are fetched automatically via CMake FetchContent (`HAI_FETCH_DEPS=ON`).
- The target executable is `home_automation_interface`.
- `aarch46` appears to be a typo; Raspberry Pi 4 64-bit target is `aarch64`.
