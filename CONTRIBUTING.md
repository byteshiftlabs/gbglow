# Contributing to gbglow

Thank you for your interest in contributing. This document describes the workflow and standards expected for all contributions.

## Prerequisites

| Tool | Version |
|---|---|
| GCC | CI builds with the default GCC on Ubuntu 24.04 (C++17 required) |
| CMake | 3.14 or newer (enforced by `CMakeLists.txt`) |
| SDL2 | Development package; located via pkg-config |
| pkg-config | Required — CMake uses it to find SDL2 |
| cppcheck | Required by `build.sh`; CI pins 2.20.0 |
| zenity or kdialog | Optional, only for the in-app ROM file picker |

Install on Ubuntu 24.04:
```bash
sudo bash ./install_deps_ubuntu.sh
```

Or by hand:
```bash
sudo apt install build-essential cmake cppcheck git libsdl2-dev pkg-config zenity
```

Only GCC on Ubuntu 24.04 is exercised by CI. Other compilers and platforms may well work, but they are not tested — if you build somewhere else and hit problems, an issue or PR is welcome.

## Building

```bash
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow
./build.sh
```

`build.sh` compiles, runs tests, and runs static analysis. Dear ImGui is fetched automatically by CMake during configure. All three stages must be clean before a PR is accepted.

By default `build.sh` uses whatever `cppcheck` is on your `PATH`, which may differ from the version CI pins. To match CI exactly, use `--bootstrap-cppcheck` as described under Static Analysis.

## Workflow

1. **Fork** the repository and create a branch following the pattern `label/brief-description`:
   - `feature/mbc2-support`
   - `fix/timer-overflow`
   - `refactor/ppu-cleanup`
   - `docs/memory-map`

2. **Implement** your change incrementally. Each logical unit of work should be a separate commit.

3. **Verify** the full build pipeline passes:
   ```bash
   ./build.sh --bootstrap-cppcheck --clean
   ```
   Zero warnings, zero cppcheck findings, all tests green.

4. **Open a pull request** against `main` with a clear description of what changed and why.

## Code Standards

- **C++17** — no C++20 features.
- Follow the **C++ Core Guidelines** for naming, const-correctness, and ownership.
- No magic numbers — use `constexpr` named constants.
- No shadow variables — locals must not shadow members, parameters, or outer variables.
- Prefer `std::copy` / `std::fill` over raw loops where intent is clearer.
- Every public function must have a Doxygen-style doc comment in the header.
- Bounds-check before indexing into ROM and RAM buffers. A ROM file is untrusted input and may be shorter than its header claims, so mapper `read`/`write` paths must validate offsets against the actual buffer size rather than assuming a well-formed cartridge.
- Copyright header on every new source file:
  ```cpp
  // SPDX-License-Identifier: GPL-3.0-or-later
  // Copyright (C) 2025-2026 gbglow Contributors
  // This file is part of gbglow. See LICENSE for details.
  ```

## Static Analysis

Run cppcheck against your changes before submitting:

```bash
./build.sh --bootstrap-cppcheck --clean
```

This bootstraps the pinned `cppcheck` version used in CI into `.tools/` when needed, then runs the same static-analysis command as the pipeline. The first bootstrap builds cppcheck from source and takes a while; afterwards it is reused. If a finding cannot be cleanly fixed (e.g., an intentional public API method that is not called internally), add an entry to `cppcheck.suppressions` with a justification comment explaining why.

## Tests

Add tests for any new public API in the relevant test target under `tests/`:

- `tests/test_core.cpp` — CPU registers, memory read/write, instructions, cartridge loading
- `tests/test_persistence.cpp` — save-state round-trip, per-component deserialization hardening, gamepad config, recent-ROM list
- `tests/test_ppu.cpp` — sprite and window rendering, CGB palette and VRAM-bank behavior, PPU state sanitization

Test names follow the pattern `test_<module>_<scenario>_<expected>`.

Save states and `.sav` files are parsed from disk and are treated as untrusted input, so deserialization paths carry hardening tests that feed them malformed data and assert the emulator sanitizes rather than trusts it. New deserialization code should come with equivalent tests.

## Commit Messages

Use past tense, describing what the commit did:

```
Added MBC2 cartridge support
Fixed timer overflow on DIV reset
Removed deprecated save-state V1 loader
```

No `WIP`, `fix`, `update`, or `misc` commit messages.

## License

By contributing, you agree your code is licensed under GPL-3.0-or-later, the same license as the project.
