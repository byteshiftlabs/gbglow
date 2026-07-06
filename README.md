# gbglow — Game Boy Emulator

A Game Boy emulator written in C++17.

![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

## Overview

gbglow includes CPU, PPU, audio, cartridge, input, save-state, and debugger components.

The current release target is Ubuntu 24.04. Release builds stay portable by default. Host-specific tuning is available when you explicitly ask for it.

Supported cartridge families:

- ROM-only
- MBC1
- MBC3
- MBC5

## Quick Start

```bash
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow
./build.sh

# Optional: enable host-specific tuning for a personal build on this machine
./build.sh --native-tuning

# 3. Run a ROM
./run.sh path/to/game.gb
```

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | D-pad |
| Z | A button |
| X | B button |
| Enter | Start |
| Shift | Select |
| Ctrl+O | Open ROM dialog |
| Ctrl+R | Reset emulator |
| F1-F9 | Save state |
| Shift+F1-F9 | Load state |
| F11 | Toggle debugger |
| F12 | Capture screenshot |
| ESC | Exit |

## Requirements

Current support target: Ubuntu 24.04.

Dear ImGui is fetched automatically during CMake configure.

Release builds stay portable by default. If you want `-march=native` and `-mtune=native`, opt in explicitly with `./build.sh --native-tuning` or `-DGBGLOW_ENABLE_NATIVE_TUNING=ON` during CMake configure.

Install on Ubuntu 24.04:
```bash
sudo apt install build-essential cmake libsdl2-dev pkg-config cppcheck zenity
```

Required tools:

- GCC or Clang with C++17 support
- CMake 3.14 or newer
- SDL2 development package
- pkg-config, used to detect the SDL2 development package and provide the compiler/linker flags for it
- cppcheck

The debugger UI uses Dear ImGui. It is fetched automatically during CMake configure, so no separate manual install is needed.

Screenshots are supported in the emulator with `F12` and are saved to a local `gbglow` screenshot directory.

## Testing

```bash
./build.sh
```

To run the exact same pinned ``cppcheck`` version that CI uses:

```bash
./build.sh --bootstrap-cppcheck --clean
```

Manual test run:

```bash
cd build && ctest --output-on-failure
```

Current test targets:

- `test_core`
- `test_persistence`
- `test_ppu`

## Documentation

Build the docs with:

```bash
python3 -m venv .docs-venv
source .docs-venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r docs/requirements.txt
make -C docs html

# Open docs/_build/html/index.html in your browser
```

On Ubuntu 24.04, prefer the virtualenv path above instead of installing Sphinx into the system interpreter.

## License

[GPL-3.0](LICENSE)
