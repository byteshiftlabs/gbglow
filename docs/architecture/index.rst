Architecture
============

gbglow follows a modular architecture with clear component separation.

.. toctree::
   :maxdepth: 1

   cpu
   memory
   ppu
   apu
   interrupts
   cartridge

System Overview
---------------

.. code-block:: text

   ┌────────────────────────────────────────────────────┐
   │                    Emulator                        │
   │  ┌────────┐  ┌────────┐  ┌────────┐  ┌──────────┐  │
   │  │  CPU   │  │  PPU   │  │  APU   │  │ Cartridge│  │
   │  │        │  │        │  │        │  │          │  │
   │  │ - Regs │  │ - LCD  │  │ - CH1  │  │ - ROM    │  │
   │  │ - ALU  │  │ - Tile │  │ - CH2  │  │ - RAM    │  │
   │  │ - Ctrl │  │ - BG   │  │ - CH3  │  │ - MBC    │  │
   │  └────┬───┘  └────┬───┘  │ - CH4  │  └────┬─────┘  │
   │       │           │      └────┬───┘       │        │
   │       └───────────┼───────────┼───────────┘        │
   │                   │           │                    │
   │            ┌──────▼───────────▼──┐                 │
   │            │       Memory        │                 │
   │            │                     │                 │
   │            │ - VRAM   - I/O      │                 │
   │            │ - WRAM   - APU Regs │                 │
   │            │ - OAM    - Wave RAM │                 │
   │            └─────────────────────┘                 │
   └────────────────────────────────────────────────────┘

Component Interaction
---------------------

Execution Flow
~~~~~~~~~~~~~~

1. **CPU** fetches and executes instruction
2. **Memory** serves read/write requests
3. **Cartridge** provides ROM data via memory bus
4. **PPU** runs in parallel, consuming cycles
5. **Interrupts** signal events (VBlank, etc.)

Timing Synchronization
~~~~~~~~~~~~~~~~~~~~~~

All components run in lock-step based on M-cycles:

* CPU executes instruction (1-6 M-cycles)
* PPU advances by same number of cycles
* Interrupts checked after each instruction

Memory Access
~~~~~~~~~~~~~

Memory requests route through the central Memory bus:

* CPU reads/writes go through Memory::read/write
* Cartridge handles ROM/RAM banking transparently
* PPU accesses VRAM directly via memory bus
* I/O registers mapped at 0xFF00-0xFF7F

Design Principles
-----------------

Single Responsibility
   Each class has one clear purpose

Clear Interfaces
   Public APIs are minimal and well-documented

Hardware Fidelity
   Implementation matches actual GB hardware behavior

Testability
   Components can be tested in isolation
