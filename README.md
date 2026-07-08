# gbglow — Game Boy Emulator

A Game Boy emulator written in C++17.

![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)

## Quick start

Clone, build, run:

```bash
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow
./build.sh
./run.sh path/to/game.gb
```

## Requirements

- C++17 compiler (GCC)
- CMake 3.14+
- SDL2 development package
- cppcheck

On Debian/Ubuntu:

```bash
sudo apt install build-essential cmake libsdl2-dev
```

## Tests

Build and run tests:

```bash
./build.sh
cd build
ctest --output-on-failure
```

Optional — run the pinned `cppcheck` used by CI (developer step):

```bash
# Optional: bootstrap and run the pinned cppcheck used by CI
./build.sh --bootstrap-cppcheck --clean
```

## Controls (summary)

- D-pad: Arrow keys
- A: `Z` — B: `X` — Start: `Enter` — Select: `Shift`
- Save/load states: `F1-F9` / `Shift+F1-F9`
- Debugger: `F11` — Screenshot: `F12` — Exit: `Esc`

Full key mapping and developer notes are in `docs/`.

## Supported cartridges

ROM-only, MBC1, MBC3, MBC5

On Ubuntu 24.04, prefer the virtualenv path above instead of installing Sphinx into the system interpreter.

See [ROADMAP.md](ROADMAP.md) for the current validation, documentation, and follow-up work priorities.

## License

GPL-3.0 — see LICENSE
