Memory Architecture
===================

Overview
--------

The Game Boy's memory system is fundamental to understanding how the hardware operates. Unlike modern computers with gigabytes of RAM, the Game Boy works within a constrained 64KB address space—yet manages to run games up to 8MB through clever memory banking techniques.

Understanding memory architecture is essential because:

* **Every instruction involves memory** — Fetching opcodes, reading operands, storing results
* **Hardware is memory-mapped** — Graphics, sound, and input are controlled by writing to specific addresses
* **Timing depends on memory access** — Different memory regions have different access rules and speeds
* **Bank switching enables large games** — The technique that lets 32KB of visible memory access megabytes of ROM

This section explains how the Game Boy's memory works, why it's designed this way, and how to emulate it correctly.

Memory System Architecture
--------------------------

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────────────────────────┐
   │                      GAME BOY MEMORY ARCHITECTURE                                   │
   ├─────────────────────────────────────────────────────────────────────────────────────┤
   │                                                                                     │
   │                           ┌─────────────────────────┐                               │
   │                           │         CPU             │                               │
   │                           │   16-bit Address Bus    │                               │
   │                           │    8-bit Data Bus       │                               │
   │                           └───────────┬─────────────┘                               │
   │                                       │                                             │
   │  ═══════════════════════════════════════════════════════════════════════════════    │
   │  ║                         MEMORY BUS (Active Component)                       ║    │
   │  ═══════════════════════════════════════════════════════════════════════════════    │
   │       │           │           │           │           │           │                 │
   │       ▼           ▼           ▼           ▼           ▼           ▼                 │
   │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐            │
   │  │CARTRIDGE│ │  VRAM   │ │ EXT RAM │ │  WRAM   │ │   OAM   │ │  I/O &  │            │
   │  │   ROM   │ │  8 KB   │ │  8 KB   │ │  8 KB   │ │  160 B  │ │  HRAM   │            │
   │  │  32 KB  │ │ (2 banks│ │(optional│ │(8 banks │ │(sprites)│ │  256 B  │            │
   │  │ visible │ │  CGB)   │ │  save)  │ │  CGB)   │ │         │ │         │            │
   │  └────┬────┘ └─────────┘ └────┬────┘ └─────────┘ └─────────┘ └─────────┘            │
   │       │                       │                                                     │
   │       │    ┌──────────────────┴──────────────────┐                                  │
   │       │    │                                     │                                  │
   │       ▼    ▼                                     ▼                                  │
   │  ┌──────────────────┐                    ┌──────────────────┐                       │
   │  │  MEMORY BANK     │                    │    CARTRIDGE     │                       │
   │  │  CONTROLLER      │                    │      RAM         │                       │
   │  │    (MBC)         │                    │  (Battery-backed │                       │
   │  │                  │                    │   for saves)     │                       │
   │  │  Bank Switching  │                    └──────────────────┘                       │
   │  │  ROM: up to 8MB  │                                                               │
   │  │  RAM: up to 128KB│                                                               │
   │  └──────────────────┘                                                               │
   │                                                                                     │
   └─────────────────────────────────────────────────────────────────────────────────────┘

**Why 64KB?**

The CPU has a 16-bit address bus, meaning it can specify addresses from 0x0000 to 0xFFFF—exactly 65,536 (64K) unique locations. This is a hardware limitation: to address more memory, you'd need more address lines, which means a larger (and more expensive) chip.

The Game Boy engineers worked within this constraint by using **bank switching**—a technique where different chunks of memory can be "swapped in" to the same address range, giving access to much more data than the address space would normally allow.

The 64KB Memory Map
-------------------

The address space is divided into distinct regions, each serving a specific purpose:

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────────────────────────┐
   │                           64KB ADDRESS SPACE MAP                                    │
   ├─────────────────────────────────────────────────────────────────────────────────────┤
   │                                                                                     │
   │   0xFFFF ┌───────────────────────────────────────┐ ◄── Interrupt Enable (IE)        │
   │          │              IE Register              │     1 byte                       │
   │   0xFFFE ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │          High RAM (HRAM)              │ ◄── 127 bytes, fast access       │
   │          │           Stack location              │     Used during DMA              │
   │   0xFF80 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │         I/O Registers                 │ ◄── Hardware control             │
   │          │    LCD, Sound, Timer, Joypad          │     128 bytes                    │
   │   0xFF00 ├───────────────────────────────────────┤                                  │
   │          │          Unusable Area                │ ◄── 96 bytes, returns garbage    │
   │   0xFEA0 ├───────────────────────────────────────┤                                  │
   │          │    Object Attribute Memory (OAM)      │ ◄── 160 bytes (40 sprites)       │
   │          │         Sprite Data                   │     4 bytes per sprite           │
   │   0xFE00 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │           Echo RAM                    │ ◄── Mirror of 0xC000-0xDDFF      │
   │          │      (Do not use - hardware quirk)    │     ~7.5KB                       │
   │   0xE000 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │     Work RAM Bank 1 (WRAM1)           │ ◄── 4KB, switchable (CGB)        │
   │          │                                       │     Banks 1-7 in CGB mode        │
   │   0xD000 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │     Work RAM Bank 0 (WRAM0)           │ ◄── 4KB, always fixed            │
   │          │                                       │                                  │
   │   0xC000 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │       External RAM (SRAM)             │ ◄── 8KB window, cartridge RAM    │
   │          │     Cartridge save data               │     Optional, battery-backed     │
   │   0xA000 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │        Video RAM (VRAM)               │ ◄── 8KB, tiles & maps            │
   │          │     Tile data, Background maps        │     2 banks in CGB mode          │
   │   0x8000 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │      ROM Bank 1-N (Switchable)        │ ◄── 16KB window                  │
   │          │        MBC-controlled                 │     Accesses larger ROMs         │
   │   0x4000 ├───────────────────────────────────────┤                                  │
   │          │                                       │                                  │
   │          │       ROM Bank 0 (Fixed)              │ ◄── 16KB, always present         │
   │          │   Interrupt vectors, Entry point      │     Contains boot code           │
   │   0x0000 └───────────────────────────────────────┘                                  │
   │                                                                                     │
   └─────────────────────────────────────────────────────────────────────────────────────┘

Memory Regions In Detail
------------------------

ROM Banks (0x0000-0x7FFF)
~~~~~~~~~~~~~~~~~~~~~~~~~

The first 32KB of address space maps to the game cartridge's ROM (Read-Only Memory). This is where the game's code and data live.

**Bank 0: Fixed ROM (0x0000-0x3FFF)**

This 16KB region always shows the first 16KB of the cartridge ROM. It contains:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Address Range
     - Contents
   * - 0x0000-0x00FF
     - **Restart Vectors** — Entry points for RST instructions (RST $00, RST $08, etc.). These provide fast subroutine calls to fixed addresses.
   * - 0x0040-0x0060
     - **Interrupt Vectors** — Where the CPU jumps when interrupts occur (VBlank at $0040, LCD STAT at $0048, Timer at $0050, Serial at $0058, Joypad at $0060).
   * - 0x0100-0x014F
     - **Cartridge Header** — Game title, manufacturer, cartridge type, ROM/RAM size, checksums. The boot ROM verifies this before starting the game.
   * - 0x0150+
     - **Game Code** — The actual program starts here.

**Bank N: Switchable ROM (0x4000-0x7FFF)**

This 16KB "window" can show different 16KB chunks of a larger ROM. For a 512KB game:

::

   Bank 0:  Always visible at 0x0000-0x3FFF (first 16KB)
   Bank 1:  Default at 0x4000-0x7FFF
   Bank 2:  Swap in by writing to MBC register
   Bank 3:  Swap in by writing to MBC register
   ...
   Bank 31: Swap in by writing to MBC register (last 16KB)

**Why Bank Switching?**

Consider Pokémon Red, which is 1MB. With only 32KB of ROM visible at once, the game uses bank switching to access all its data:

::

   Total ROM size:    1,048,576 bytes (1 MB)
   Visible at once:   32,768 bytes (32 KB)
   Number of banks:   64 banks × 16KB = 1MB
   
   To load a Pokémon sprite from bank 15:
   1. Write 15 to the MBC bank register (e.g., address 0x2000)
   2. Bank 15's data now appears at 0x4000-0x7FFF
   3. Read the sprite data from that address range

Video RAM (0x8000-0x9FFF)
~~~~~~~~~~~~~~~~~~~~~~~~~

VRAM stores all graphics data—the building blocks the PPU uses to render the screen.

**Understanding Tiles**

The Game Boy doesn't work with pixels directly. Instead, it uses **tiles**—8×8 pixel blocks that are assembled to create the screen. This approach dramatically reduces memory requirements:

::

   Direct pixel storage:  160 × 144 × 2 bits = 5,760 bytes per frame
   Tile-based storage:    256 tiles × 16 bytes = 4,096 bytes
                         + 32×32 tile map = 1,024 bytes
                         = 5,120 bytes (and tiles are reusable!)

**VRAM Layout**

.. code-block:: text

   ┌────────────────────────────────────────────────────────────────────┐
   │                        VRAM ORGANIZATION                           │
   ├────────────────────────────────────────────────────────────────────┤
   │                                                                    │
   │   0x8000 ┌──────────────────────────────────────┐                  │
   │          │         TILE DATA BLOCK 0            │                  │
   │          │      Tiles 0-127 (unsigned mode)     │                  │
   │          │      Tiles 128-255 (signed mode)     │                  │
   │   0x8800 ├──────────────────────────────────────┤                  │
   │          │         TILE DATA BLOCK 1            │                  │
   │          │      Tiles 128-255 (unsigned mode)   │                  │
   │          │      Tiles 0-127 (signed mode)       │                  │
   │   0x9000 ├──────────────────────────────────────┤                  │
   │          │         TILE DATA BLOCK 2            │                  │
   │          │      Tiles 0-127 (both modes)        │                  │
   │          │      Shared between BG and Window    │                  │
   │   0x9800 ├──────────────────────────────────────┤                  │
   │          │         TILE MAP 0                   │                  │
   │          │      32×32 = 1024 tile references    │                  │
   │          │      Background or Window            │                  │
   │   0x9C00 ├──────────────────────────────────────┤                  │
   │          │         TILE MAP 1                   │                  │
   │          │      32×32 = 1024 tile references    │                  │
   │          │      Background or Window            │                  │
   │   0xA000 └──────────────────────────────────────┘                  │
   │                                                                    │
   └────────────────────────────────────────────────────────────────────┘

**Tile Data Format**

Each tile is 8×8 pixels, with each pixel having a 2-bit color value (0-3). Tiles are stored in an interleaved format:

::

   Each row of a tile = 2 bytes (16 bits for 8 pixels)
   
   Byte 0: Low bits  of each pixel  (pixel 7 ... pixel 0)
   Byte 1: High bits of each pixel  (pixel 7 ... pixel 0)
   
   Example: A row where pixels are [3, 2, 1, 0, 0, 1, 2, 3]
   
   Pixel values:  3    2    1    0    0    1    2    3
   Binary:        11   10   01   00   00   01   10   11
   
   Byte 0 (low):  1    0    1    0    0    1    0    1  = 0xA5
   Byte 1 (high): 1    1    0    0    0    0    1    1  = 0xC3

**Tile Addressing Modes**

The LCDC register bit 4 controls which addressing mode is used:

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - LCDC.4
     - Base Address
     - Tile Range
   * - 0
     - 0x8800 (signed)
     - Tiles -128 to 127 (0x8800-0x97FF)
   * - 1
     - 0x8000 (unsigned)
     - Tiles 0 to 255 (0x8000-0x8FFF)

**VRAM Access Restrictions**

VRAM is only accessible during certain PPU modes:

::

   Mode 0 (HBlank):    VRAM accessible ✓
   Mode 1 (VBlank):    VRAM accessible ✓
   Mode 2 (OAM Scan):  VRAM accessible ✓
   Mode 3 (Drawing):   VRAM blocked ✗ (reads return 0xFF)

Games must time their VRAM writes carefully or risk corrupted graphics.

External RAM / Cartridge RAM (0xA000-0xBFFF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This 8KB region maps to RAM on the cartridge itself (if present). It serves several purposes:

**Save Game Data**

Many cartridges include battery-backed RAM that persists when the Game Boy is turned off. This is how games save your progress:

::

   Pokémon saves:     Your party, badges, Pokédex
   Zelda saves:       Game progress, heart containers
   Tetris:            High scores

**Extra Working Memory**

Some games use cartridge RAM as extra work space when the built-in 8KB WRAM isn't enough.

**RAM Banking**

Larger cartridges can have multiple 8KB RAM banks:

::

   Pokémon uses 32KB of save RAM (4 banks):
   Bank 0: Current game state
   Bank 1: Box 1-6 Pokémon storage
   Bank 2: Box 7-12 Pokémon storage  
   Bank 3: Hall of Fame, player info

**RAM Enable**

For power-saving, cartridge RAM is disabled by default. Games must enable it:

::

   ; Enable external RAM
   LD A, $0A
   LD ($0000), A    ; Write to 0x0000-0x1FFF enables RAM
   
   ; ... use RAM at 0xA000-0xBFFF ...
   
   ; Disable RAM (protects against accidental writes)
   LD A, $00
   LD ($0000), A

Work RAM (0xC000-0xDFFF)
~~~~~~~~~~~~~~~~~~~~~~~~

WRAM is the Game Boy's main working memory—where games store variables, arrays, and runtime data.

**DMG (Original Game Boy)**

Simple 8KB block from 0xC000 to 0xDFFF.

**CGB (Game Boy Color)**

32KB total, organized as 8 banks of 4KB each:

::

   0xC000-0xCFFF: Bank 0 (always fixed)
   0xD000-0xDFFF: Banks 1-7 (switchable via SVBK register at 0xFF70)
   
   SVBK register:
   Bits 0-2: Select WRAM bank (1-7)
   Value 0 maps to bank 1 (bank 0 is always at 0xC000)

**Common WRAM Usage**

::

   0xC000-0xC0FF: Often used for sprite attribute table (copied to OAM via DMA)
   0xC100-0xCFFF: Game variables, arrays, state machines
   0xD000-0xDFFF: Additional storage, display buffers

Echo RAM (0xE000-0xFDFF)
~~~~~~~~~~~~~~~~~~~~~~~~

This region is a **hardware quirk**, not a feature. Due to incomplete address decoding, reads/writes to 0xE000-0xFDFF actually access 0xC000-0xDDFF (WRAM).

.. warning::
   **Do not use Echo RAM in real games.** While it works on most hardware, some Game Boy variants handle it differently. Nintendo's official documentation forbids its use.

**For Emulators**

Emulators should implement echo RAM as a mirror:

::

   Read from 0xE123  →  Return value from 0xC123
   Write to 0xE456   →  Write to 0xC456

OAM - Object Attribute Memory (0xFE00-0xFE9F)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OAM stores data for up to 40 sprites (moveable objects that appear on screen).

**Sprite Entry Format**

Each sprite uses 4 bytes:

.. code-block:: text

   ┌──────────────────────────────────────────────────────────────────────┐
   │                    SPRITE ATTRIBUTE FORMAT                           │
   ├──────────────────────────────────────────────────────────────────────┤
   │                                                                      │
   │   Byte 0: Y Position                                                 │
   │   ┌─────────────────────────────────────────────────────────────┐    │
   │   │  Screen Y = (value) - 16                                    │    │
   │   │  Value 0 = Sprite is 16 pixels above screen (hidden)        │    │
   │   │  Value 16 = Sprite Y is at screen top (Y=0)                 │    │
   │   │  Value 160 = Sprite is 16 pixels past bottom (hidden)       │    │
   │   └─────────────────────────────────────────────────────────────┘    │
   │                                                                      │
   │   Byte 1: X Position                                                 │
   │   ┌─────────────────────────────────────────────────────────────┐    │
   │   │  Screen X = (value) - 8                                     │    │
   │   │  Value 0 = Sprite is 8 pixels left of screen (hidden)       │    │
   │   │  Value 8 = Sprite X is at screen left (X=0)                 │    │
   │   │  Value 168 = Sprite is 8 pixels past right (hidden)         │    │
   │   └─────────────────────────────────────────────────────────────┘    │
   │                                                                      │
   │   Byte 2: Tile Number                                                │
   │   ┌─────────────────────────────────────────────────────────────┐    │
   │   │  Index into tile data at 0x8000                             │    │
   │   │  For 8×16 sprites: Bit 0 is ignored (uses tile N and N+1)   │    │
   │   └─────────────────────────────────────────────────────────────┘    │
   │                                                                      │
   │   Byte 3: Attributes/Flags                                           │
   │   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐                  │
   │   │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │                  │
   │   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤                  │
   │   │ Pri │YFlip│XFlip│ Pal │        CGB Only       │                  │
   │   └─────┴─────┴─────┴─────┴─────────────────────────┘                │
   │                                                                      │
   │   Bit 7 (Priority): 0 = Above BG, 1 = Behind BG colors 1-3           │
   │   Bit 6 (Y Flip):   0 = Normal, 1 = Vertically mirrored              │
   │   Bit 5 (X Flip):   0 = Normal, 1 = Horizontally mirrored            │
   │   Bit 4 (Palette):  0 = OBP0, 1 = OBP1 (DMG only)                    │
   │   Bits 0-3:         CGB palette number (CGB only)                    │
   │                                                                      │
   └──────────────────────────────────────────────────────────────────────┘

**Why the Position Offset?**

Sprites are offset by (8, 16) to allow partial visibility at screen edges:

::

   To place a sprite with its top-left at screen position (0, 0):
   Y byte = 0 + 16 = 16
   X byte = 0 + 8 = 8
   
   To hide a sprite completely:
   Set Y = 0 (16 pixels above screen) or Y = 160 (below screen)

**OAM Access Restrictions**

Like VRAM, OAM has access restrictions based on PPU mode:

::

   Mode 0 (HBlank):    OAM accessible ✓
   Mode 1 (VBlank):    OAM accessible ✓
   Mode 2 (OAM Scan):  OAM blocked ✗
   Mode 3 (Drawing):   OAM blocked ✗

**Sprite Limits**

* Maximum 40 sprites total in OAM
* Maximum 10 sprites per scanline (hardware limit)
* Sprites beyond the 10-per-line limit are not drawn

Unusable Region (0xFEA0-0xFEFF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This 96-byte region exists due to the memory map layout but isn't connected to any memory. Behavior varies by hardware revision:

::

   DMG: Returns 0x00
   CGB: Returns semi-random values
   
   Emulator recommendation: Return 0xFF for reads, ignore writes

I/O Registers (0xFF00-0xFF7F)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This 128-byte region contains **memory-mapped I/O registers**—special addresses that control hardware or provide status information.

**What is Memory-Mapped I/O?**

Instead of separate instructions for hardware control (like some CPUs have), the Game Boy uses regular memory reads/writes to specific addresses:

::

   ; Turn off the LCD
   LD A, $00
   LDH ($40), A     ; Write to 0xFF40 (LCDC register)
   
   ; Read joypad state
   LDH A, ($00)     ; Read from 0xFF00 (P1 register)

**I/O Register Map**

.. code-block:: text

   ┌──────────┬──────────────┬────────────────────────────────────────────┐
   │ Address  │    Name      │              Description                   │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** INPUT ***                              │
   │ 0xFF00   │ P1/JOYP      │ Joypad input                               │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** SERIAL ***                             │
   │ 0xFF01   │ SB           │ Serial transfer data byte                  │
   │ 0xFF02   │ SC           │ Serial transfer control                    │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** TIMER ***                              │
   │ 0xFF04   │ DIV          │ Divider register (increments at 16384 Hz)  │
   │ 0xFF05   │ TIMA         │ Timer counter                              │
   │ 0xFF06   │ TMA          │ Timer modulo (reload value)                │
   │ 0xFF07   │ TAC          │ Timer control (enable, frequency)          │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** INTERRUPTS ***                         │
   │ 0xFF0F   │ IF           │ Interrupt flags (pending interrupts)       │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** SOUND ***                              │
   │ 0xFF10   │ NR10         │ Channel 1 sweep                            │
   │ 0xFF11   │ NR11         │ Channel 1 length/duty                      │
   │ 0xFF12   │ NR12         │ Channel 1 envelope                         │
   │ 0xFF13   │ NR13         │ Channel 1 frequency low                    │
   │ 0xFF14   │ NR14         │ Channel 1 frequency high                   │
   │ 0xFF16   │ NR21         │ Channel 2 length/duty                      │
   │ 0xFF17   │ NR22         │ Channel 2 envelope                         │
   │ 0xFF18   │ NR23         │ Channel 2 frequency low                    │
   │ 0xFF19   │ NR24         │ Channel 2 frequency high                   │
   │ 0xFF1A   │ NR30         │ Channel 3 enable                           │
   │ 0xFF1B   │ NR31         │ Channel 3 length                           │
   │ 0xFF1C   │ NR32         │ Channel 3 volume                           │
   │ 0xFF1D   │ NR33         │ Channel 3 frequency low                    │
   │ 0xFF1E   │ NR34         │ Channel 3 frequency high                   │
   │ 0xFF20   │ NR41         │ Channel 4 length                           │
   │ 0xFF21   │ NR42         │ Channel 4 envelope                         │
   │ 0xFF22   │ NR43         │ Channel 4 polynomial counter               │
   │ 0xFF23   │ NR44         │ Channel 4 control                          │
   │ 0xFF24   │ NR50         │ Master volume                              │
   │ 0xFF25   │ NR51         │ Sound panning                              │
   │ 0xFF26   │ NR52         │ Sound on/off                               │
   │ 0xFF30-3F│ Wave RAM     │ Channel 3 waveform data (16 bytes)         │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** LCD ***                                │
   │ 0xFF40   │ LCDC         │ LCD control                                │
   │ 0xFF41   │ STAT         │ LCD status                                 │
   │ 0xFF42   │ SCY          │ Background scroll Y                        │
   │ 0xFF43   │ SCX          │ Background scroll X                        │
   │ 0xFF44   │ LY           │ Current scanline (0-153, read-only)        │
   │ 0xFF45   │ LYC          │ Scanline compare                           │
   │ 0xFF46   │ DMA          │ OAM DMA transfer start                     │
   │ 0xFF47   │ BGP          │ Background palette (DMG)                   │
   │ 0xFF48   │ OBP0         │ Sprite palette 0 (DMG)                     │
   │ 0xFF49   │ OBP1         │ Sprite palette 1 (DMG)                     │
   │ 0xFF4A   │ WY           │ Window Y position                          │
   │ 0xFF4B   │ WX           │ Window X position                          │
   ├──────────┼──────────────┼────────────────────────────────────────────┤
   │          │              │ *** CGB ONLY ***                           │
   │ 0xFF4D   │ KEY1         │ Speed switch                               │
   │ 0xFF4F   │ VBK          │ VRAM bank select                           │
   │ 0xFF51-55│ HDMA1-5      │ VRAM DMA source/dest/length                │
   │ 0xFF68   │ BCPS         │ Background palette index                   │
   │ 0xFF69   │ BCPD         │ Background palette data                    │
   │ 0xFF6A   │ OCPS         │ Sprite palette index                       │
   │ 0xFF6B   │ OCPD         │ Sprite palette data                        │
   │ 0xFF70   │ SVBK         │ WRAM bank select                           │
   └──────────┴──────────────┴────────────────────────────────────────────┘

High RAM (0xFF80-0xFFFE)
~~~~~~~~~~~~~~~~~~~~~~~~

HRAM is 127 bytes of fast internal RAM with special properties:

**Why HRAM Exists**

During OAM DMA transfer (copying sprite data to OAM), most memory becomes inaccessible—the DMA controller is using the bus. Only HRAM remains accessible during this time, so:

::

   ; This code must run from HRAM during DMA
   DMA_Routine:
       LDH (DMA), A      ; Start DMA transfer
       LD A, $28         ; Wait counter (40 iterations)
   .wait:
       DEC A             ; 1 cycle
       JR NZ, .wait      ; 3 cycles (taken) = 4 cycles × 40 = 160 cycles
       RET               ; DMA complete, safe to access memory again

**LDH Instructions**

The CPU has special instructions optimized for accessing 0xFF00-0xFFFF:

::

   LDH A, ($XX)      ; Load from 0xFF00 + XX into A (2 bytes, 3 cycles)
   LDH ($XX), A      ; Store A to 0xFF00 + XX (2 bytes, 3 cycles)
   LD A, (C)         ; Load from 0xFF00 + C into A (1 byte, 2 cycles)
   LD (C), A         ; Store A to 0xFF00 + C (1 byte, 2 cycles)
   
   vs. regular memory access:
   LD A, ($FFXX)     ; Would be 3 bytes, 4 cycles

**Common HRAM Usage**

* Stack placement (SP often initialized to 0xFFFE)
* DMA wait routine (must be in HRAM)
* Frequently accessed variables (saves cycles)

Interrupt Enable Register (0xFFFF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This single byte controls which interrupts the CPU will respond to:

.. code-block:: text

   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
   │ Bit │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
   │     │  -  │  -  │  -  │ Joy │ Ser │ Tim │ LCD │VBlnk│
   └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
   
   Bit 4: Joypad    - Button press detected
   Bit 3: Serial    - Serial transfer complete
   Bit 2: Timer     - Timer overflow
   Bit 1: LCD STAT  - LCD status condition met
   Bit 0: VBlank    - Entered VBlank period

See :doc:`interrupts` for detailed interrupt handling.

DMA Transfer System
-------------------

Direct Memory Access (DMA) allows fast data copying without CPU involvement. The Game Boy has two DMA systems.

OAM DMA (All Models)
~~~~~~~~~~~~~~~~~~~~

Copies 160 bytes from ROM/RAM to OAM (sprite memory) for fast sprite updates.

**How It Works**

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────────────────┐
   │                          OAM DMA TRANSFER                               │
   ├─────────────────────────────────────────────────────────────────────────┤
   │                                                                         │
   │   1. Write source address high byte to 0xFF46                           │
   │      Example: Write $C0 → Source = $C000                                │
   │                                                                         │
   │   ┌─────────────┐                      ┌─────────────┐                  │
   │   │   Source    │   160 bytes          │     OAM     │                  │
   │   │  $C000-     │ ===================> │   $FE00-    │                  │
   │   │   $C09F     │   (160 M-cycles)     │    $FE9F    │                  │
   │   └─────────────┘                      └─────────────┘                  │
   │                                                                         │
   │   2. During transfer (160 M-cycles):                                    │
   │      - CPU can only access HRAM ($FF80-$FFFE)                           │
   │      - All other memory reads return $FF                                │
   │      - Code MUST run from HRAM                                          │
   │                                                                         │
   │   3. Transfer completes, normal memory access resumes                   │
   │                                                                         │
   └─────────────────────────────────────────────────────────────────────────┘

**Standard DMA Routine**

Every Game Boy game needs this routine copied to HRAM:

::

   ; Call this to start DMA. A = source address high byte (e.g., $C0 for $C000)
   OAM_DMA:
       LDH ($46), A      ; Write source to DMA register, starts transfer
       LD A, $28         ; 40 iterations × 4 cycles = 160 cycles
   .wait:
       DEC A
       JR NZ, .wait
       RET
   
   ; The routine itself is 10 bytes. Copy it to HRAM at startup:
   ; LD HL, OAM_DMA_Code    ; Source
   ; LD DE, $FF80            ; Destination (HRAM)
   ; LD BC, 10               ; Length
   ; ... copy loop ...

HDMA/GDMA (CGB Only)
~~~~~~~~~~~~~~~~~~~~

The Game Boy Color adds a more flexible VRAM DMA system:

**GDMA - General Purpose DMA**

Copies up to 2KB to VRAM instantly (halts CPU during transfer):

::

   HDMA1 ($FF51): Source high byte
   HDMA2 ($FF52): Source low byte (lower 4 bits ignored)
   HDMA3 ($FF53): Destination high byte (only $80-$9F valid)
   HDMA4 ($FF54): Destination low byte (lower 4 bits ignored)
   HDMA5 ($FF55): Length and mode
                  Bit 7 = 0: GDMA mode
                  Bits 0-6: (Length / 16) - 1

**HDMA - HBlank DMA**

Copies 16 bytes per HBlank (doesn't halt CPU):

::

   HDMA5 ($FF55): Length and mode
                  Bit 7 = 1: HDMA mode
                  Bits 0-6: (Length / 16) - 1
   
   Each HBlank: 16 bytes transferred
   Useful for smooth scrolling effects, large sprite animations

Memory Access Timing
--------------------

Understanding when memory can be accessed is crucial for correct emulation.

Access Restrictions by PPU Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 15 15 15 15 40

   * - PPU Mode
     - VRAM
     - OAM
     - Duration
     - Notes
   * - **Mode 0** (HBlank)
     - ✓ Yes
     - ✓ Yes
     - 87-204 cycles
     - Safe to update graphics
   * - **Mode 1** (VBlank)
     - ✓ Yes
     - ✓ Yes
     - 4560 cycles
     - Primary window for updates
   * - **Mode 2** (OAM Scan)
     - ✓ Yes
     - ✗ No
     - 80 cycles
     - PPU reading OAM
   * - **Mode 3** (Drawing)
     - ✗ No
     - ✗ No
     - 172-289 cycles
     - PPU using both VRAM and OAM

**Blocked Access Behavior**

When memory is blocked:

* **Reads** return 0xFF
* **Writes** are ignored

**Frame Timing**

::

   One frame = 70,224 cycles = ~16.74 ms (~59.7 Hz)
   
   Lines 0-143:   Visible scanlines
     Mode 2: 80 cycles  (OAM scan)
     Mode 3: 172-289 cycles (rendering)
     Mode 0: remainder of 456 cycles (HBlank)
   
   Lines 144-153: VBlank period
     Mode 1: 4560 cycles total
     Primary time for game logic and graphics updates

Testing Memory Implementation
-----------------------------

Memory tests should verify:

**Address Decoding**
   Each memory region returns correct data and accepts writes appropriately.

**Echo RAM Mirroring**
   Writes to 0xE000-0xFDFF should appear at 0xC000-0xDDFF.

**Read-Only Regions**
   ROM should not be writable (except through MBC registers).

**16-bit Operations**
   Little-endian read/write: ``read16(0xC000)`` should return ``(read(0xC001) << 8) | read(0xC000)``.

**OAM DMA**
   Writing to 0xFF46 should copy 160 bytes from source to OAM.

**I/O Register Behavior**
   Special behaviors: LY is read-only, DIV resets on write, etc.

References
----------

.. [PanDocs_Memory] Pan Docs - Memory Map. https://gbdev.io/pandocs/Memory_Map.html
.. [PanDocs_OAM] Pan Docs - OAM (Sprites). https://gbdev.io/pandocs/OAM.html
.. [PanDocs_DMA] Pan Docs - OAM DMA Transfer. https://gbdev.io/pandocs/OAM_DMA_Transfer.html

.. important::
   **Implementation Accuracy**
   
   Memory timing and access restrictions are critical for accurate emulation. Many games work without perfect timing, but some (especially demos and test ROMs) require cycle-accurate memory access emulation. Start with a simple implementation, then add accuracy as needed for compatibility.

