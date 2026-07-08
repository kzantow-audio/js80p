# Building JS80P (JUCE)

Builds the JUCE plugin (VST3 + Standalone). This is a CMake build added alongside the
legacy `Makefile`.

## Status

- Adds a JUCE VST3 + Standalone target; the legacy `Makefile` build is unchanged.
- Milestone 1 (original GUI running inside JUCE) is complete; verified on Linux.
  Windows and macOS builds are not yet verified.
- Milestone 2 (new simplified GUI, branch `feat/juce-gui`) is in progress. It lives
  under `src/ui/` and is reached via the editor's "Detailed view" toggle; the engine
  and patch format are never modified.

### New GUI — current features

- **Two pages** via SYNTH / EFFECTS tabs centered in the header.
  - *Synth page*: header + macro strip (macros 1-8 with per-macro MIDI-CC source) +
    OSC 1 / MIX / OSC 2 columns + a right-third scrollable Modulators panel.
    - Each oscillator column is one panel that also contains its two compact
      filters at the bottom (a 2-column filter-type glyph grid then CUTOFF / Q /
      GAIN). OSC 2 additionally exposes its distortion TYPE as a knob beside DIST.
      MIX and MODULATORS are title-less transparent sections; the MIX column shows
      two small signal-flow triangles (OSC 1 → mix knobs → OSC 2).
    - The MIX column stacks MIX / PM / FM / AM knobs over **POLY** (note handling),
      a combined **TUNING** selector, and **MODE**. TUNING and POLY apply to both
      oscillators / the whole synth and read their live value on load.
    - Two tiny pie-fill dot controls on each OSC title (oscillator inaccuracy /
      instability) and on each envelope card title (time / level inaccuracy); each
      fills clockwise from the bottom.
  - *Effects page*: recreates the original effects chain as panels of knobs, with
    the macro strip still shown on top. The top line holds Input, Vol 1, Dist 1/2,
    Filter 1/2, Vol 2 and OUT (Vol 3); Tape, Chorus, Echo and Reverb each get their
    own full-width single-line row. Continuous knobs are macro-modulation
    destinations (right-click → Modulate by); the Filter cutoffs use the 1 kHz-
    center scaling and the Dist / Filter type knobs display names, not indices.
    No LG toggles.
- **Modulation** (see `doc/z-gui.md` §5-6): knobs are modulation-aware (rotation =
  base, ring-band / badge drag = amount); envelope/LFO groups sharing a shape are one
  editor card; assigning an existing modulator clones a fresh pool slot; global
  sources (macros / CC / wheels / **Random**) route through an intermediate macro.
  Grouped copies are modulated together; unassigning garbage-collects orphaned pool
  slots. DTN/FIN pitch knobs snap base and amount to semitones (Ctrl = fine).
  Envelope cards expose SCL as a modulatable slider in the card title.

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

   If using Visual Studio community edition, you can also use:
   ```bat
   cmake -B build -G "Visual Studio 17 2022" -A x64 -T ClangCL
   cmake --build build --config Release
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
