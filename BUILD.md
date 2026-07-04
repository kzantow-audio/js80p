# Building JS80P (JUCE)

Builds the JUCE plugin (VST3 + Standalone). This is a CMake build added alongside the
legacy `Makefile`.

## Status

- Adds a JUCE VST3 + Standalone target; the legacy `Makefile` build is unchanged.
- Milestone 1 (original GUI running inside JUCE) is complete; verified on Linux.
  Windows and macOS builds are not yet verified.

## Requirements

- **CMake ≥ 3.22** and **Git**. The first configure downloads **JUCE 8.0.14** (internet
  required, ~2–4 min, one time).
- A compiler:
  - **Linux:** GCC or Clang.
  - **macOS:** Apple Clang (Xcode).
  - **Windows:** **`clang-cl`**. MSVC `cl.exe` and MinGW/GCC are not supported.

---

## Linux

1. Install the toolchain and JUCE dependencies:

   ```bash
   sudo apt install build-essential cmake ninja-build \
     libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
     libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
     libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev libcurl4-openssl-dev
   ```

2. Configure and build:

   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

Output:
- `build/JS80P_artefacts/Release/VST3/JS80P.vst3`
- `build/JS80P_artefacts/Release/Standalone/JS80P`

---

## Windows

1. Install **Build Tools for Visual Studio 2022**
   (<https://aka.ms/vs/17/release/vs_BuildTools.exe>). In the installer, select the
   **Desktop development with C++** workload, then on the **Individual components** tab
   ensure these are checked:
   - MSVC v143 - VS 2022 C++ x64/x86 build tools
   - Windows 11 SDK (or Windows 10 SDK)
   - C++ Clang Compiler for Windows
   - C++ CMake tools for Windows

2. Open **x64 Native Tools Command Prompt for VS 2022** (Start menu).

3. Configure and build (run from the repository root):

   ```bat
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
     -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
   cmake --build build
   ```

Output:
- `build\JS80P_artefacts\Release\VST3\JS80P.vst3`
- `build\JS80P_artefacts\Release\Standalone\JS80P.exe`

> Reconfiguring with a different compiler requires a clean build directory: delete
> `build\` first (`rmdir /s /q build`).

---

## macOS

1. Install the toolchain:

   ```bash
   xcode-select --install
   brew install cmake ninja
   ```

2. Configure and build:

   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

Output:
- `build/JS80P_artefacts/Release/VST3/JS80P.vst3`
- `build/JS80P_artefacts/Release/Standalone/JS80P.app`

---

## Installing the VST3

Copy `JS80P.vst3` to the system VST3 directory:

- Linux: `~/.vst3/`
- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/`

Or configure with `-DCOPY_PLUGIN_AFTER_BUILD=TRUE` to install on build.

---

## Verifying

Run [pluginval](https://github.com/Tracktion/pluginval) against the built VST3:

```bash
pluginval --strictness-level 5 --skip-gui-tests --validate <path>/JS80P.vst3
```

`--skip-gui-tests` is required in headless environments (pluginval's editor-embedding
test crashes without a display server).
