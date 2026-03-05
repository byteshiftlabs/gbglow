Memory Architecture
===================

The Game Boy has a 64KB address space with memory-mapped I/O.

Memory Map
----------

.. code-block:: text

   ┌──────────────┬──────────┬─────────────────────────────┐
   │   Address    │   Size   │         Description         │
   ├──────────────┼──────────┼─────────────────────────────┤
   │ 0x0000-0x3FFF│   16KB   │ ROM Bank 0 (fixed)          │
   │ 0x4000-0x7FFF│   16KB   │ ROM Bank 1-N (switchable)   │
   │ 0x8000-0x9FFF│    8KB   │ Video RAM (VRAM)            │
   │ 0xA000-0xBFFF│    8KB   │ External RAM (cartridge)    │
   │ 0xC000-0xCFFF│    4KB   │ Work RAM Bank 0 (WRAM)      │
   │ 0xD000-0xDFFF│    4KB   │ Work RAM Bank 1             │
   │ 0xE000-0xFDFF│   ~8KB   │ Echo RAM (mirror C000-DDFF) │
   │ 0xFE00-0xFE9F│   160B   │ Object Attribute Memory     │
   │ 0xFEA0-0xFEFF│    96B   │ Unusable                    │
   │ 0xFF00-0xFF7F│   128B   │ I/O Registers               │
   │ 0xFF80-0xFFFE│   127B   │ High RAM (HRAM)             │
   │ 0xFFFF       │     1B   │ Interrupt Enable Register   │
   └──────────────┴──────────┴─────────────────────────────┘

ROM (0x0000-0x7FFF)
-------------------

Cartridge ROM is mapped to the first 32KB of address space:

**Bank 0 (0x0000-0x3FFF)**
   Fixed bank, always contains the first 16KB of ROM.
   Includes interrupt vectors and game initialization code.

**Bank N (0x4000-0x7FFF)**
   Switchable bank controlled by Memory Bank Controller (MBC).
   Allows games larger than 32KB by bank switching.

VRAM (0x8000-0x9FFF)
--------------------

Video RAM stores tile data and tile maps:

**Tile Data (0x8000-0x97FF)**
   * 384 tiles, 16 bytes each
   * Two addressing modes: signed (0x8800) and unsigned (0x8000)

**Tile Maps (0x9800-0x9FFF)**
   * Two 32×32 tile maps
   * Background map: 0x9800 or 0x9C00 (LCDC bit 3)

External RAM (0xA000-0xBFFF)
-----------------------------

Cartridge RAM for save data:

* Optional, depends on cartridge type
* Battery-backed on some cartridges
* Can be banked (multiple 8KB banks)
* Enabled/disabled by MBC

WRAM (0xC000-0xDFFF)
--------------------

8KB of working RAM:

* Bank 0: 0xC000-0xCFFF (fixed)
* Bank 1: 0xD000-0xDFFF (switchable in CGB mode)

Echo RAM (0xE000-0xFDFF)
------------------------

Mirror of WRAM (0xC000-0xDDFF). Writes to echo RAM also write to WRAM.

**Note:** Using echo RAM is discouraged and may not work on all hardware.

OAM (0xFE00-0xFE9F)
-------------------

Object Attribute Memory stores sprite data:

* 40 sprites, 4 bytes each (160 bytes total)
* Sprite format: Y pos, X pos, Tile #, Attributes
* Inaccessible during PPU modes 2 & 3

**OAM Structure**

.. code-block:: text

   Each sprite entry (4 bytes):
   
   Byte 0: Y Position (screen Y + 16)
   Byte 1: X Position (screen X + 8)
   Byte 2: Tile Number
   Byte 3: Attributes
      Bit 7: Priority (0=above BG, 1=behind BG)
      Bit 6: Y-flip
      Bit 5: X-flip
      Bit 4: Palette (0=OBP0, 1=OBP1)
      Bits 3-0: Unused (DMG) / Palette # (CGB)

**OAM DMA (0xFF46)**

Games use DMA to quickly copy sprite data from RAM to OAM:

.. code-block:: text

   Register: 0xFF46
   
   Write: Triggers DMA transfer
   Value: High byte of source address (XX → XX00)
   
   Transfer:
   - Source: (value << 8) to (value << 8) + 159
   - Destination: 0xFE00 to 0xFE9F
   - Size: 160 bytes (40 sprites × 4 bytes)
   - Time: 160 machine cycles (real hardware)
   
   Example:
      LD A, $C3      ; Source = 0xC300
      LDH ($46), A   ; Trigger DMA

**Why DMA?**

* OAM is not accessible during PPU modes 2 & 3
* Games need to update all sprites quickly during VBlank
* DMA allows fast transfer without CPU overhead
* Common pattern: Copy from Work RAM (0xC000-0xDF00)

**Implementation**

.. code-block:: cpp

   // In Memory::write()
   if (address == OAM_DMA_REG) {
       u16 source = static_cast<u16>(value) << OAM_DMA_SOURCE_SHIFT;
       
       for (u16 i = 0; i < OAM_SIZE; i++) {
           oam_[i] = read(source + i);
       }
       io_regs_[address - IO_REGISTERS_START] = value;
       return;
   }

**Constants**

.. code-block:: cpp

   constexpr u16 OAM_DMA_REG = 0xFF46;
   constexpr u16 OAM_SIZE = 160;  // 40 sprites × 4 bytes
   constexpr u8 OAM_DMA_SOURCE_SHIFT = 8;

I/O Registers (0xFF00-0xFF7F)
-----------------------------

Memory-mapped hardware control:

.. list-table::
   :header-rows: 1
   :widths: 15 15 70

   * - Address
     - Name
     - Description
   * - 0xFF00
     - P1/JOYP
     - Joypad input
   * - 0xFF01
     - SB
     - Serial transfer data
   * - 0xFF02
     - SC
     - Serial transfer control
   * - 0xFF04
     - DIV
     - Divider register
   * - 0xFF05
     - TIMA
     - Timer counter
   * - 0xFF06
     - TMA
     - Timer modulo
   * - 0xFF07
     - TAC
     - Timer control
   * - 0xFF0F
     - IF
     - Interrupt flags
   * - 0xFF10-0xFF26
     - NRxx
     - Audio registers
   * - 0xFF40
     - LCDC
     - LCD control
   * - 0xFF41
     - STAT
     - LCD status
   * - 0xFF42
     - SCY
     - Scroll Y
   * - 0xFF43
     - SCX
     - Scroll X
   * - 0xFF44
     - LY
     - Current scanline
   * - 0xFF45
     - LYC
     - Scanline compare
   * - 0xFF46
     - DMA
     - OAM DMA transfer
   * - 0xFF47
     - BGP
     - Background palette
   * - 0xFF48-0xFF49
     - OBP0/1
     - Object palettes
   * - 0xFF4A
     - WY
     - Window Y
   * - 0xFF4B
     - WX
     - Window X

HRAM (0xFF80-0xFFFE)
--------------------

High RAM - 127 bytes of fast internal RAM:

* Accessible even when WRAM is disabled
* Used for time-critical code
* Stack often placed here

Interrupt Enable (0xFFFF)
--------------------------

Controls which interrupts are enabled:

.. code-block:: text

   Bit 0: VBlank
   Bit 1: LCD STAT
   Bit 2: Timer
   Bit 3: Serial
   Bit 4: Joypad

Implementation
--------------

Memory Class
~~~~~~~~~~~~

.. code-block:: cpp

   class Memory {
   public:
       Memory();
       ~Memory();
       
       void load_cartridge(std::unique_ptr<Cartridge> cart);
       
       u8 read(u16 address) const;
       void write(u16 address, u8 value);
       
       u16 read16(u16 address) const;
       void write16(u16 address, u16 value);
       
   private:
       std::array<u8, 0x2000> vram_;      // 8KB Video RAM
       std::array<u8, 0x2000> wram_;      // 8KB Work RAM
       std::array<u8, 0x00A0> oam_;       // Object Attribute Memory
       std::array<u8, 0x0080> hram_;      // High RAM
       std::array<u8, 0x0080> io_regs_;   // I/O registers
       u8 interrupt_enable_;
       
       std::unique_ptr<Cartridge> cartridge_;
   };

Address Decoding
~~~~~~~~~~~~~~~~

Memory access is decoded by address range:

.. code-block:: cpp

   u8 Memory::read(u16 address) const {
       if (address < 0x8000) {
           // ROM: delegate to cartridge
           return cartridge_ ? cartridge_->read(address) : 0xFF;
       } else if (address < 0xA000) {
           // VRAM
           return vram_[address - 0x8000];
       } else if (address < 0xC000) {
           // External RAM: delegate to cartridge
           return cartridge_ ? cartridge_->read(address) : 0xFF;
       } else if (address < 0xE000) {
           // WRAM
           return wram_[address - 0xC000];
       } else if (address < 0xFE00) {
           // Echo RAM
           return wram_[address - 0xE000];
       } else if (address < 0xFEA0) {
           // OAM
           return oam_[address - 0xFE00];
       } else if (address < 0xFF00) {
           // Unusable
           return 0xFF;
       } else if (address < 0xFF80) {
           // I/O Registers
           return io_regs_[address - 0xFF00];
       } else if (address < 0xFFFF) {
           // HRAM
           return hram_[address - 0xFF80];
       } else {
           // Interrupt Enable
           return interrupt_enable_;
       }
   }
   * - WRAM
     - 1
     - Always accessible
   * - OAM
     - 1
     - Blocked during PPU modes 2-3
   * - HRAM
     - 1
     - Always accessible

DMA Transfers
-------------

OAM DMA allows fast copy from ROM/RAM to OAM:

.. code-block:: cpp

   // Write to 0xFF46 starts DMA
   void Memory::write(u16 address, u8 value) {
       if (address == 0xFF46) {
           // DMA transfer: copy 160 bytes to OAM
           u16 source = value * 0x100;
           for (int i = 0; i < 0xA0; i++) {
               oam_[i] = read(source + i);
           }
       }
       // ... other write handling
   }

Testing
-------

Memory tests verify:

* Correct address decoding
* Echo RAM mirroring
* I/O register access
* Cartridge integration
* 16-bit read/write operations

