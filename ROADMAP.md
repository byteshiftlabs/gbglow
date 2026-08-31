# gbglow Development Roadmap

## Purpose

This document tracks feature areas and follow-up work for gbglow. It is a planning document, not a verification report: items listed as implemented describe code that exists, not code whose accuracy has been formally validated.

## Implemented

- CPU, memory, timer, and interrupt handling
- PPU rendering, including DMA-related behavior, plus CGB palettes and VRAM
  banking for color-only cartridges
- APU and audio output
- Cartridge support for ROM-only, MBC1, MBC3 (including RTC), and MBC5
- Battery-backed `.sav` files and numbered save states across nine slots
- Keyboard and gamepad input, with remappable Game Boy buttons
- Dear ImGui debugger UI, screenshots, and a recent-ROM list
- Sphinx documentation and a CMake/CTest test suite run by CI on every push and PR

## Ongoing Work

- Close documented accuracy gaps, especially around timing-sensitive behavior
- Extend hardening tests over untrusted input paths — ROM headers, `.sav` files, and save states are all parsed from disk
- Keep build, test, and static-analysis workflows current as the code changes

## Not Currently Supported

- Serial port and link-cable behavior — not implemented
- Cartridge types outside ROM-only/MBC1/MBC3/MBC5 — rejected at load time
- Color output is unverified. gbglow emulates original hardware, so only a
  cartridge whose header marks it color-only takes the color path; cartridges
  that support color but also run in monochrome are emulated in monochrome. No
  color ROM has been run through the emulator, so the color path is written and
  unit-tested but never exercised end to end.
- Remaining Game Boy Color edge cases and timing details
- Broader manual compatibility testing across ROM libraries

## Working Notes

- Prefer small, reviewable changes over large mixed refactors
- Treat README and Sphinx docs as user-facing documentation that must stay factual
- Record open gaps as follow-up work instead of treating them as completed milestones
- Do not describe untested changes as verified; say what was actually run
