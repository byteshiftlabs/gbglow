# Contributing to gbglow

Thank you for your interest in contributing. This document describes the workflow and standards expected for all contributions.

## Prerequisites

| Tool | Minimum version |
|---|---|
| GCC or Clang | GCC 11 / Clang 13 (C++17 required) |
| CMake | 3.14 |
| SDL2 | 2.0.20 |
| cppcheck | 2.7 |

Install on Ubuntu 24.04:
```bash
sudo apt install build-essential cmake libsdl2-dev pkg-config cppcheck
```

Or use the helper script:
```bash
sudo bash ./install_deps_ubuntu.sh
```

## Building

```bash
git clone https://github.com/byteshiftlabs/gbglow.git
cd gbglow
./build.sh
```

`build.sh` compiles, runs tests, and runs static analysis. Dear ImGui is fetched automatically by CMake during configure. All three stages must be clean before a PR is accepted.

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

This bootstraps the pinned ``cppcheck`` version used in CI into ``.tools/`` when needed, then runs the same static-analysis command as the pipeline. If a finding cannot be cleanly fixed (e.g., an intentional public API method that is not called internally), add an entry to `cppcheck.suppressions` with a justification comment explaining why.

## Tests

Add tests for any new public API in `tests/test_basic.cpp`. Test names follow the pattern `test_<module>_<scenario>_<expected>`.

**Current coverage**: CPU registers, memory read/write, basic instructions, and cartridge loading. PPU rendering, APU audio, and save-state round-trip tests are not yet covered — contributions welcome.

## Commit Messages

Use the past participle, present tense description:

```
Added MBC2 cartridge support
Fixed timer overflow on DIV reset
Removed deprecated save-state V1 loader
```

No `WIP`, `fix`, `update`, or `misc` commit messages.

## License

By contributing, you agree your code is licensed under GPL-3.0-or-later, the same license as the project.
