Design Decisions
================

This page records the key technology and design choices made for gbglow and
the reasoning behind them. The goal is to give contributors and reviewers
enough context to understand why the project is built the way it is, not just
how.

Language: C++17
---------------

**Decision:** C++17 is the required language standard.

**Why not older?**
``std::filesystem`` is used throughout for file I/O, path handling, and atomic
writes. Dropping to C++14 would require replacing it with POSIX calls or a
third-party library on every compiler this project targets, with no portability
benefit in return.

**Why not newer (C++20/23/26)?**
Newer standards add useful tools — ``std::span``, concepts, ranges, static
reflection, pattern matching — but none of them change the current emulator
design in a meaningful way. The emulation loop is single-threaded and
synchronous, the instruction dispatch is table-based, and the subsystem
boundaries are already well-defined. Adopting a newer standard would raise
the compiler baseline for contributors and CI without delivering a concrete
improvement.

Build System: CMake
-------------------

**Decision:** CMake is the build system.

**Why CMake?**
The repository builds an application binary, multiple test targets, and
third-party UI code (Dear ImGui via ``FetchContent``). CMake keeps that
configure-and-build flow compiler-agnostic, integrates naturally with
``ctest``, and aligns local builds with CI without requiring custom glue. It
is also the default dialect most C++ contributors already know, which reduces
setup friction.

**Why not plain Make or Meson?**
Plain Make works for single-target builds but tends to accumulate custom shell
glue once you add test targets, dependency fetching, and build-type flags.
Meson is a reasonable alternative, but the added familiarity cost is not
justified when CMake already covers the project's needs.
