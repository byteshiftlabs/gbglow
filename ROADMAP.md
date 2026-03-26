# gbglow Development Roadmap

## Purpose

This document tracks feature areas and follow-up work for gbglow. It is a planning document, not a verification report.

## Current Scope

The current codebase includes work across these areas:

- Core CPU, memory, timer, and interrupt handling
- PPU rendering and DMA-related behavior
- APU and audio output plumbing
- Cartridge support for ROM-only, MBC1, MBC3, and MBC5
- Input handling, debugger UI, save states, and recent-ROM support
- Project documentation and automated test targets

## Ongoing Work

- Expand behavioral validation against additional ROM-based test suites
- Close documented accuracy gaps, especially around timing-sensitive behavior
- Continue tightening documentation so it stays aligned with the current implementation
- Keep build, test, and static-analysis workflows current as the code changes

## Deferred Work

- Serial and link-related behavior
- Additional cartridge variants beyond the currently supported set
- Remaining Game Boy Color edge cases and timing details
- Broader manual compatibility testing across ROM libraries

## Working Notes

- Prefer small, reviewable changes over large mixed refactors
- Treat README and Sphinx docs as user-facing documentation that must stay factual
- Record open gaps as follow-up work instead of treating them as completed milestones
