Project Overview
================

Philosophy
----------

gbglow prioritizes:

**Clarity**
   Every component is well-documented with references to hardware behavior.
   Code reads like documentation.

**Modularity**
   Clean separation between CPU, PPU, memory, and other subsystems.
   Each component can be understood independently.

**Testability**
   Comprehensive test suite using official test ROMs.
   Unit tests for each major component.

**Education**
   Code and documentation help others understand GB/GBC hardware.
   Clear explanations of why things work the way they do.

Technology Stack
----------------

Language
~~~~~~~~
C++17 - Modern C++ with zero-cost abstractions, strong typing, and clean OOP design.

Build System
~~~~~~~~~~~~
CMake - Cross-platform build configuration.

Testing
~~~~~~~
* Custom test suite
* Blargg's test ROMs
* Mooneye test suite compatibility

Game Boy Hardware Overview
---------------------------

The Game Boy uses a custom Sharp LR35902 CPU (Z80-like) running at 4.194304 MHz.

Key Components:

CPU
   * 8-bit architecture with 16-bit addressing
   * 8 general-purpose 8-bit registers (can be paired)
   * 256 base instructions + 256 CB-prefixed instructions

PPU (Picture Processing Unit)
   * 160×144 pixel LCD display
   * 4-shade grayscale (DMG) or color (CGB)
   * Tile-based rendering with sprites
   * Four rendering modes with precise timing

Memory
   * 64KB address space
   * 8KB VRAM, 8KB WRAM
   * Memory-mapped I/O
   * Cartridge with optional banking

Audio
   * 4 sound channels
   * 2 pulse waves, 1 wave table, 1 noise

Input
   * 8-button controller (D-pad, A, B, Start, Select)

Technical Specifications
------------------------

Screen Resolution
   160×144 pixels (visible area)

Color Palette
   * DMG: 4 shades of gray-green
   * CGB: 32,768 colors (15-bit RGB)

Memory Map
   See :doc:`architecture/memory` for detailed mapping

References
----------

* `Pan Docs <https://gbdev.io/pandocs/>`_ - The definitive Game Boy technical reference
* `Game Boy CPU Manual <http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf>`_
* `The Cycle-Accurate Game Boy Docs <https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf>`_
