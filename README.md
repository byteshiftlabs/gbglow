# gbglow — Game Boy Emulator

![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)
![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![CI](https://github.com/byteshiftlabs/gbglow/actions/workflows/ci.yml/badge.svg)

A Game Boy emulator written in C++17.

## Quick start

Clone, build, run:

```bash
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow
sudo bash ./install_deps_ubuntu.sh
./build.sh
./run.sh path/to/game.gb
```

`build.sh` compiles, runs the test suite, and runs static analysis. `run.sh` builds first if `build/gbglow` is missing, then launches the emulator with the ROM you pass it.

## Requirements

- C++17 compiler (GCC; CI builds with the default GCC on Ubuntu 24.04)
- CMake 3.14+
- SDL2 development package
- pkg-config — CMake locates SDL2 through it
- cppcheck — `build.sh` runs static analysis on every build and will not proceed without it
- zenity or kdialog — optional, only for the in-app "open ROM" file picker

On Debian/Ubuntu, `install_deps_ubuntu.sh` installs all of the above. To do it by hand:

```bash
sudo apt install build-essential cmake cppcheck git libsdl2-dev pkg-config zenity
```

Dear ImGui is fetched automatically by CMake during configure, so it does not need installing.

## Tests

`./build.sh` already runs the tests. To re-run them on their own against an existing build:

```bash
cd build
ctest --output-on-failure
```

To build against the same pinned cppcheck version CI uses, rather than whatever your distro ships:

```bash
./build.sh --bootstrap-cppcheck --clean
```

This downloads and builds cppcheck into `.tools/` the first time, which is slow, then reuses it.

## Controls

- D-pad: Arrow keys
- A: `Z` — B: `X` — Start: `Enter` — Select: `Shift`
- Save state: `F1`–`F9` — Load state: `Shift+F1`–`Shift+F9`
- Open ROM: `Ctrl+O` — Reset: `Ctrl+R`
- Pause: `P` — Mute: `M` — Fast-forward: hold `Space`
- Debugger: `F11` — Screenshot: `F12` — Exit: `Esc`

Game Boy button bindings can be remapped in `config/keybindings.conf`. The debugger adds `F5` (continue/pause) and `F10` (step over) while it is open. See `docs/` for developer notes.

## Supported cartridges

ROM-only, MBC1, MBC3, MBC5. Any other cartridge type is rejected at load time with an error — MBC2 in particular is not supported.

## Documentation

The Sphinx sources live in `docs/`. To build them locally:

```bash
python3 -m venv .docs-venv
. .docs-venv/bin/activate
python -m pip install -r docs/requirements.txt
make -C docs html
```

Output lands in `docs/_build/html`.

See [ROADMAP.md](ROADMAP.md) for what is and is not currently in scope, and [CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow.

## License

GPL-3.0 — see LICENSE
