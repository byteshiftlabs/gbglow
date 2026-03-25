# gbglow — Game Boy Color Emulator

A Game Boy Color emulator written in C++17.

![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

## Overview

gbglow provides a Game Boy and Game Boy Color emulator with CPU, PPU, audio, cartridge, input, save-state, and debugger components.

Key components: Sharp LR35902 CPU instruction handling, Picture Processing Unit (DMG and CGB palette modes), 4-channel APU, MBC1/MBC3/MBC5 cartridge support with battery-backed saves, and a built-in step debugger with ImGui overlay.

The repository includes implementation notes and Sphinx documentation for contributors working on emulator behavior and subsystem boundaries.

## Quick Start

```bash
# 1. Clone the repository
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow

# 2. Build (requires CMake ≥ 3.14, GCC/Clang with C++17, SDL2)
#    Dear ImGui is fetched automatically by CMake during configure
./build.sh

# 3. Run a ROM
./run.sh path/to/game.gb
```

### Controls

| Key | Action |
|-----|--------|
| Arrow keys | D-pad |
| Z | A button |
| X | B button |
| Enter | Start |
| Shift | Select |
| Ctrl+O | Open ROM dialog |
| Ctrl+R | Reset emulator |
| F1–F9 | Save state (slot 1–9) |
| Shift+F1–F9 | Load state (slot 1–9) |
| F11 | Toggle debugger |
| F12 | Capture screenshot |
| ESC | Exit |

## How It Works

gbglow follows the classic emulator pipeline each frame:

1. **CPU** executes instructions until the frame cycle budget is consumed, raising interrupts as hardware would (VBlank, Timer, Joypad).
2. **PPU** steps in lockstep with the CPU, progressing through OAM Search → Transfer → HBlank → VBlank modes and writing pixels to a 160×144 framebuffer.
3. **APU** generates PCM samples that are queued to SDL2's audio device; the audio queue backpressure drives frame-rate synchronisation.
4. **Display** (SDL2 + ImGui) blits the framebuffer, handles input, and renders the debugger overlay.

Save states serialise the full machine snapshot (CPU registers, memory, PPU state, timer, audio, cartridge MBC registers) in the ``GBGLOW_STATE`` binary format, stored next to the ROM file.

## Project Structure

```
src/
  main.cpp              Entry point
  core/                 CPU, Memory (MMU), Emulator, save-state I/O, Timer
  video/                PPU, SDL2 display layer
  audio/                APU (4-channel: Square×2, Wave, Noise)
  cartridge/            ROM loader, MBC1, MBC3, MBC5
  input/                Joypad, gamepad mapping
  debug/                Debugger back-end and ImGui front-end
  ui/                   Recent ROMs list, screenshot capture
external/
  stb/                  stb_image_write (PNG screenshot export)
tests/
  test_core.cpp         CPU, memory, timer, and cartridge tests
  test_persistence.cpp  Save-state and persistence tests
  test_ppu.cpp          PPU behavior tests
docs/                   Sphinx documentation source
config/
  keybindings.conf      Key binding configuration file
```

## Requirements

Current support target: Ubuntu 24.04.

| Dependency | Version | Purpose |
|---|---|---|
| C++ compiler | GCC ≥ 11 or Clang ≥ 13 | C++17 required |
| CMake | ≥ 3.14 | Build system and FetchContent support |
| SDL2 | ≥ 2.0.20 | Video, audio, input |
| pkg-config | any | SDL2 discovery |
| cppcheck | recent | Static analysis used by `build.sh` |

Dear ImGui is fetched automatically during CMake configure.

Install on Ubuntu 24.04:
```bash
sudo apt install build-essential cmake libsdl2-dev pkg-config cppcheck zenity
```

Other platforms may work, but they are not part of the current supported build target.

`zenity` is optional but enables the `Browse` button in the Open ROM dialog on Linux. Without it, you can still paste a ROM path manually.

## Testing

```bash
cd build && ctest --output-on-failure
```

Current test targets:

- `test_core`
- `test_persistence`
- `test_ppu`

## Documentation

Full Sphinx documentation in `docs/`:

```bash
cd docs && pip install -r requirements.txt && make html
# Open docs/_build/html/index.html
```

Covers CPU architecture, memory map, PPU modes, interrupt handling, and the save-state format.

## License

[GPL-3.0](LICENSE)

## References

- [Pan Docs](https://gbdev.io/pandocs/) — Complete Game Boy hardware reference
- [GB Opcodes](https://gbdev.io/gb-opcodes/) — Opcode table with flags
- [Game Boy CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
