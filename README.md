# gbglow — Game Boy Color Emulator

A cycle-accurate Game Boy Color emulator written in C++17, designed for clarity, correctness, and educational value.

![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

## Overview

gbglow emulates the Nintendo Game Boy Color hardware at the cycle level, running commercial ROMs including Pokémon titles. The codebase is structured as a reference implementation: every component maps directly to a real hardware block, with inline documentation explaining the hardware behaviour behind each implementation decision.

Key components: Sharp LR35902 CPU (~180 opcodes + full CB prefix), Picture Processing Unit (DMG and CGB palette modes), 4-channel APU, MBC1/MBC3/MBC5 cartridge support with battery-backed saves, and a built-in step debugger with ImGui overlay.

The project prioritises readable, well-documented code over aggressive optimisation, making it suitable as a study resource for anyone learning Game Boy emulation.

## Quick Start

```bash
# 1. Clone with submodules (ImGui is a git submodule)
git clone --recurse-submodules https://github.com/byteshiftlabs/gbglow.git
cd gbglow

# 2. Build (requires CMake ≥ 3.10, GCC/Clang with C++17, SDL2)
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
| F1–F9 | Save state (slot 1–9) |
| Shift+F1–F9 | Load state (slot 1–9) |
| F12 | Toggle debugger |
| ESC | Exit |

## How It Works

gbglow follows the classic emulator pipeline each frame:

1. **CPU** executes instructions until the frame cycle budget is consumed, raising interrupts as hardware would (VBlank, Timer, Joypad).
2. **PPU** steps in lockstep with the CPU, progressing through OAM Search → Transfer → HBlank → VBlank modes and writing pixels to a 160×144 framebuffer.
3. **APU** generates PCM samples that are queued to SDL2's audio device; the audio queue backpressure drives frame-rate synchronisation.
4. **Display** (SDL2 + ImGui) blits the framebuffer, handles input, and renders the debugger overlay.

Save states serialise the full machine snapshot (CPU registers, memory, PPU state, timer, audio) in the `GBGLOW_STATE_V2` binary format, stored next to the ROM file.

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
  imgui/                Dear ImGui (git submodule)
  stb/                  stb_image_write (PNG screenshot export)
tests/
  test_basic.cpp        Component unit tests (CPU, Memory, Registers)
docs/                   Sphinx documentation source
config/
  keybindings.conf      Key binding configuration file
```

## Requirements

| Dependency | Version | Purpose |
|---|---|---|
| C++ compiler | GCC ≥ 11 or Clang ≥ 13 | C++17 required |
| CMake | ≥ 3.10 | Build system |
| SDL2 | ≥ 2.0.20 | Video, audio, input |
| pkg-config | any | SDL2 discovery |

Install on Ubuntu/Debian:
```bash
sudo apt install build-essential cmake libsdl2-dev pkg-config
```

## Testing

```bash
cd build && ctest --output-on-failure
```

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
