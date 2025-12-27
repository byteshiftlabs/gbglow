Project Overview
================

Philosophy
----------

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────────────────────────┐
   │                              GBCRUSH DESIGN PRINCIPLES                              │
   └─────────────────────────────────────────────────────────────────────────────────────┘

   ╔═══════════════════════════════════╗    ╔═══════════════════════════════════╗
   ║           📖 CLARITY              ║   ║          🧩 MODULARITY            ║
   ╠═══════════════════════════════════╣    ╠═══════════════════════════════════╣
   ║  • Well-documented components     ║    ║  • Clean component separation     ║
   ║  • Hardware behavior references   ║    ║  • CPU, PPU, APU, Memory isolated ║
   ║  • Code reads like documentation  ║    ║  • Independent understanding      ║
   ╚═══════════════════════════════════╝    ╚═══════════════════════════════════╝

   ╔═══════════════════════════════════╗    ╔═══════════════════════════════════╗
   ║          🧪 TESTABILITY           ║   ║          🎓 EDUCATION             ║
   ╠═══════════════════════════════════╣    ╠═══════════════════════════════════╣
   ║  • Unit tests for each component  ║    ║  • Learn GB/GBC hardware          ║
   ║  • Easy verification & debugging  ║    ║  • Clear explanations of "why"    ║
   ║  • Blargg & Mooneye test suites   ║    ║  • Reference implementations      ║
   ╚═══════════════════════════════════╝    ╚═══════════════════════════════════╝

Technology Stack
----------------

* **Language**: C++17 with modern features, zero-cost abstractions, and clean OOP design
* **Build System**: CMake for cross-platform builds
* **Graphics/Audio**: SDL2 for display and audio output
* **GUI**: Dear ImGui for menu interface
* **Testing**: Blargg's and Mooneye test ROM suites

Hardware Overview
-----------------

The Game Boy uses a custom Sharp LR35902 CPU (Z80-like) running at 4.194304 MHz.

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────────────────────────┐
   │                          SHARP LR35902 CPU                                          │
   ├─────────────────────────────────────────────────────────────────────────────────────┤
   │                                                                                     │
   │  ┌─────────────────────────────────┐     ┌─────────────────────────────────────┐    │
   │  │         REGISTER FILE           │     │         CONTROL UNIT                │    │
   │  │  ┌─────────┬─────────┐          │     │  ┌─────────────────────────────┐    │    │
   │  │  │    A    │    F    │ ◄─ AF    │     │  │    INSTRUCTION DECODER      │    │    │
   │  │  │ (Accum) │ (Flags) │   (16b)  │     │  │  ┌───────────────────────┐  │    │    │
   │  │  │  8-bit  │ ZNHC    │          │     │  │  │ Opcode Fetch (M1)     │  │    │    │
   │  │  ├─────────┼─────────┤          │     │  │  │ Operand Fetch         │  │    │    │
   │  │  │    B    │    C    │ ◄─ BC    │     │  │  │ Execute               │  │    │    │
   │  │  │  8-bit  │  8-bit  │   (16b)  │     │  │  │ Write Back            │  │    │    │
   │  │  ├─────────┼─────────┤          │     │  │  └───────────────────────┘  │    │    │
   │  │  │    D    │    E    │ ◄─ DE    │     │  │                             │    │    │
   │  │  │  8-bit  │  8-bit  │   (16b)  │     │  │  256 Base Instructions      │    │    │
   │  │  ├─────────┼─────────┤          │     │  │  256 CB-Prefixed Instrs     │    │    │
   │  │  │    H    │    L    │ ◄─ HL    │     │  └─────────────────────────────┘    │    │
   │  │  │  8-bit  │  8-bit  │   (16b)  │     │                                     │    │
   │  │  └─────────┴─────────┘          │     │  ┌─────────────────────────────┐    │    │
   │  │  ┌───────────────────┐          │     │  │    TIMING CONTROL           │    │    │
   │  │  │        SP         │ ◄─ Stack │     │  │  ┌───────────────────────┐  │    │    │
   │  │  │    Stack Pointer  │   Ptr    │     │  │  │ M-cycles (4 T-states) │  │    │    │
   │  │  │      16-bit       │          │     │  │  │ T-states @ 4.19 MHz   │  │    │    │
   │  │  ├───────────────────┤          │     │  │  │ ~1µs per M-cycle      │  │    │    │
   │  │  │        PC         │ ◄─ Prog  │     │  │  └───────────────────────┘  │    │    │
   │  │  │  Program Counter  │   Cntr   │     │  └─────────────────────────────┘    │    │
   │  │  │      16-bit       │          │     └─────────────────────────────────────┘    │
   │  │  └───────────────────┘          │                                                │
   │  └─────────────────────────────────┘                                                │
   │                 │                                        │                          │
   │                 ▼                                        ▼                          │
   │  ┌──────────────────────────────────────────────────────────────────────────────┐   │
   │  │                              INTERNAL DATA BUS (8-bit)                       │   │
   │  └──────────────────────────────────────────────────────────────────────────────┘   │
   │         │                    │                    │                    │            │
   │         ▼                    ▼                    ▼                    ▼            │
   │  ┌─────────────┐     ┌─────────────────┐  ┌─────────────────┐  ┌────────────────┐   │
   │  │     ALU     │     │  INTERRUPT      │  │  ADDRESS GEN    │  │  BUS INTERFACE │   │
   │  │  (8-bit)    │     │  CONTROLLER     │  │                 │  │      UNIT      │   │
   │  │ ┌─────────┐ │     │ ┌─────────────┐ │  │ ┌─────────────┐ │  │ ┌────────────┐ │   │
   │  │ │ ADD/SUB │ │     │ │ IME (Master)│ │  │ │ Address     │ │  │ │ Data Latch │ │   │
   │  │ │ AND/OR  │ │     │ │ IE  (FF0F)  │ │  │ │ Incrementer │ │  │ │ (8-bit)    │ │   │
   │  │ │ XOR/CP  │ │     │ │ IF  (FFFF)  │ │  │ │ HL+/HL-     │ │  │ ├────────────┤ │   │
   │  │ │ INC/DEC │ │     │ ├─────────────┤ │  │ │ SP+/SP-     │ │  │ │ Addr Latch │ │   │
   │  │ │ RL/RR   │ │     │ │ VBlank  $40 │ │  │ │ PC+1/PC+2   │ │  │ │ (16-bit)   │ │   │
   │  │ │ SLA/SRA │ │     │ │ LCD     $48 │ │  │ └─────────────┘ │  │ └────────────┘ │   │
   │  │ │ SWAP    │ │     │ │ Timer   $50 │ │  │                 │  │       │        │   │
   │  │ │ BIT/SET │ │     │ │ Serial  $58 │ │  │ Addressing:     │  │       ▼        │   │
   │  │ │ RES     │ │     │ │ Joypad  $60 │ │  │ • Immediate     │  │ ┌────────────┐ │   │
   │  │ └─────────┘ │     │ └─────────────┘ │  │ • Register      │  │ │ RD/WR/MREQ │ │   │
   │  │      │      │     │                 │  │ • (HL)          │  │ │ Control    │ │   │
   │  │      ▼      │     │ Priority:       │  │ • (BC)/(DE)     │  │ └────────────┘ │   │
   │  │ ┌─────────┐ │     │ VBlank>LCD>     │  │ • (nn)          │  └───────┬────────┘   │
   │  │ │  FLAGS  │ │     │ Timer>Serial>   │  │ • FF00+n        │          │            │
   │  │ │ Z N H C │ │     │ Joypad          │  └─────────────────┘          │            │
   │  │ └─────────┘ │     └─────────────────┘                               │            │
   │  └─────────────┘                                                       │            │
   │                                                                        │            │
   ├────────────────────────────────────────────────────────────────────────┼────────────┤
   │                                                                        │            │
   │  ════════════════════════════ ADDRESS BUS (A0-A15) ════════════════════╪══════════  │
   │                                    16-bit                              │            │
   │  ──────────────────────────── DATA BUS (D0-D7) ────────────────────────┼──────────  │
   │                                    8-bit                               │            │
   │  - - - - - - - - - - - CONTROL (RD, WR, MREQ, IORQ, M1, CLK) - - - - - ┼ - - - - -  │
   │                                                                        │            │
   │                                                                        ▼            │
   │                                                              ┌──────────────────┐   │
   │                                                              │  EXTERNAL MEMORY │   │
   │                                                              │  & I/O DEVICES   │   │
   │                                                              └──────────────────┘   │
   └─────────────────────────────────────────────────────────────────────────────────────┘

   FLAG REGISTER (F):
   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
   │ Bit │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
   │     │  Z  │  N  │  H  │  C  │  0  │  0  │  0  │  0  │
   └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
     Z = Zero Flag       (Result is zero)
     N = Subtract Flag   (Subtraction operation)
     H = Half-Carry Flag (Carry from bit 3 to 4)
     C = Carry Flag      (Carry from bit 7 / borrow)

Core Components
~~~~~~~~~~~~~~~

+-------------+------------------------------------------------------------------+
| Component   | Specifications                                                   |
+=============+==================================================================+
| **CPU**     | * Sharp LR35902 (Z80-like) @ 4.194304 MHz                        |
|             | * 8-bit architecture with 16-bit addressing                      |
|             | * 8 general-purpose 8-bit registers (can be paired)              |
|             | * 256 base instructions + 256 CB-prefixed instructions           |
+-------------+------------------------------------------------------------------+
| **PPU**     | * 160×144 pixel LCD display                                      |
|             | * 4-shade grayscale (DMG) or color (CGB)                         |
|             | * Tile-based rendering with 8×8 and 8×16 sprites                 |
|             | * Four rendering modes with precise timing                       |
+-------------+------------------------------------------------------------------+
| **Memory**  | * 64KB address space                                             |
|             | * 8KB VRAM, 8KB WRAM                                             |
|             | * Memory-mapped I/O                                              |
|             | * Cartridge with optional banking (MBC1/3/5)                     |
+-------------+------------------------------------------------------------------+
| **Audio**   | * 4 sound channels                                               |
|             | * 2 pulse waves, 1 wave table, 1 noise                           |
|             | * Real-time audio output via SDL2                                |
+-------------+------------------------------------------------------------------+
| **Input**   | * 8-button controller                                            |
|             | * D-pad, A, B, Start, Select                                     |
|             | * Configurable keyboard mapping                                  |
+-------------+------------------------------------------------------------------+

Technical Specifications
~~~~~~~~~~~~~~~~~~~~~~~~

+-------------------+-------------------------------------------------------+
| Feature           | Details                                               |
+===================+=======================================================+
| Screen Resolution | 160×144 pixels (visible area)                         |
+-------------------+-------------------------------------------------------+
| Color Palette     | DMG: 4 shades of gray-green                           |
|                   | CGB: 32,768 colors (15-bit RGB)                       |
+-------------------+-------------------------------------------------------+
| CPU Clock         | 4.194304 MHz (DMG/CGB)                                |
+-------------------+-------------------------------------------------------+
| Frame Rate        | ~59.73 Hz (VBlank frequency)                          |
+-------------------+-------------------------------------------------------+
| RAM               | 8KB Work RAM (WRAM)                                   |
|                   | 8KB Video RAM (VRAM)                                  |
+-------------------+-------------------------------------------------------+
| Cartridge Support | MBC1, MBC3, MBC5 with battery-backed saves            |
+-------------------+-------------------------------------------------------+
| Memory Map        | See :doc:`architecture/memory` for detailed mapping   |
+-------------------+-------------------------------------------------------+

References
----------

* `Pan Docs <https://gbdev.io/pandocs/>`_ - The definitive Game Boy technical reference
* `Game Boy CPU Manual <http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf>`_
* `The Cycle-Accurate Game Boy Docs <https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf>`_
