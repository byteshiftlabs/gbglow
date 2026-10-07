# gbglow — Game Boy Emulator

![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)
![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![CI](https://github.com/byteshiftlabs/gbglow/actions/workflows/ci.yml/badge.svg)

A Game Boy emulator written in C++17, with a built-in debugger.

## Quick start

On Ubuntu 22.04:

```bash
sudo apt install build-essential cmake libsdl2-dev
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow
./build.sh
./run.sh path/to/game.gb
```

`build.sh` builds and runs the tests, then runs cppcheck if it is installed (`sudo apt install cppcheck`). `zenity` (or `kdialog`) is only needed for the `Ctrl+O` file picker.

## Controls

| Action | Key |
|---|---|
| D-pad | Arrow keys |
| A / B | `Z` / `X` |
| Start / Select | `Enter` / `Shift` |
| Save / load state | `F1`–`F9` / `Shift+F1`–`Shift+F9` |
| Open ROM / Reset | `Ctrl+O` / `Ctrl+R` |
| Pause / Mute / Fast-forward | `P` / `M` / hold `Space` |
| Debugger / Screenshot / Exit | `F11` / `F12` / `Esc` |

Button bindings live in `config/keybindings.conf`.

## Supported cartridges

ROM-only, MBC1, MBC3, MBC5.

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow and static analysis, [ROADMAP.md](ROADMAP.md) for scope, and `docs/` for the Sphinx developer docs (`make -C docs html` after `pip install -r docs/requirements.txt`).

## Acknowledgments

- [Pan Docs](https://gbdev.io/pandocs/), [GB Opcodes](https://gbdev.io/gb-opcodes/), [Game Boy CPU Manual](http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf), [awesome-gbdev](https://github.com/gbdev/awesome-gbdev)
- [SDL2](https://www.libsdl.org/), [Dear ImGui](https://github.com/ocornut/imgui) v1.91.8 (fetched by CMake), [stb_image_write](https://github.com/nothings/stb) v1.16 (vendored in `src/vendor/`)

## License

GPL-3.0 — see LICENSE
