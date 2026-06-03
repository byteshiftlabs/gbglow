Project Overview
================

Philosophy
----------

gbglow prioritizes:

**Clarity**
   Documentation focuses on component boundaries and hardware-facing behavior.

**Modularity**
   Clean separation between CPU, PPU, memory, and other subsystems.
   Each component can be understood independently.

**Testability**
   The project includes unit tests for core components.

**Education**
   Documentation explains implementation choices where they affect behavior.

Technology Stack
----------------

Language
~~~~~~~~
C++17 - Modern C++ with strong typing and standard library support.

Build System
~~~~~~~~~~~~
CMake - Cross-platform build configuration.

Testing
~~~~~~~
* Custom unit test suite (CPU registers, memory, basic instructions)

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

* `Pan Docs <https://gbdev.io/pandocs/>`_ - Game Boy technical reference
* `Game Boy CPU Manual <http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf>`_
* `The Cycle-Accurate Game Boy Docs <https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf>`_
