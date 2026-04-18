Implementation Notes
====================

Design decisions, implementation details, and technical notes.

Project Philosophy
------------------

Code Quality
~~~~~~~~~~~~

gbglow prioritizes **code clarity** above all else:

* Self-documenting code with meaningful names
* Comments that explain intent where needed
* Documentation for subsystem behavior and interfaces
* Simple, straightforward implementations

Modular Architecture
~~~~~~~~~~~~~~~~~~~~

Components are cleanly separated:

.. code-block:: text

   CPU ──> Memory <──┬── PPU
                     ├── Cartridge
                     └── Joypad

Benefits:

* Easy to understand each component
* Simple to test in isolation
* Clean interfaces between systems
* Straightforward to add features

Architecture Decisions
----------------------

Why C++17?
~~~~~~~~~~

**Advantages:**

* Control over low-level data structures
* Standard toolchain support
* Modern language features such as smart pointers, constexpr, and std::optional

**Alternatives Considered:**

* **Rust**: Different ownership model and toolchain tradeoffs
* **C**: Lower-level language with fewer standard abstractions
* **Python**: Different runtime model than the current implementation

Why CMake?
~~~~~~~~~~

* Cross-platform build system
* Industry standard
* Good IDE integration
* Simple for small projects

Why Not Use External Libraries?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Philosophy**: Minimize dependencies for clarity.

Current dependencies:

* Standard Library only
* No SDL, OpenGL, etc.

**Rationale**:

* Easier to understand
* Simpler to build
* Focus on emulation core
* Rendering is pluggable

CPU Implementation
------------------

Instruction Dispatch
~~~~~~~~~~~~~~~~~~~~

Switch-based dispatch for clarity:

.. code-block:: cpp

   Cycles CPU::execute_instruction(u8 opcode) {
       switch (opcode) {
           case 0x00: return nop();
           case 0x01: return ld_bc_nn();
           case 0x02: return ld_bcp_a();
           // ... all 256 opcodes
       }
   }

**Why not jump table?**

* Switch is more readable
* Compiler optimizes to jump table anyway
* Easier to add per-instruction debugging

**Why not interpreter pattern?**

* More complex class hierarchy
* Less clear instruction flow

Instruction Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~

Each instruction is a clear, self-contained function:

.. code-block:: cpp

   Cycles CPU::ld_hl_nn() {
       u16 value = read_word_pc();
       regs_.hl(value);
       return 3;
   }

Pattern:

1. Read operands
2. Perform operation
3. Update flags (if needed)
4. Return cycle count

Flag Handling
~~~~~~~~~~~~~

Flags updated explicitly for clarity:

.. code-block:: cpp

   void CPU::add_a(u8 value) {
       u16 result = regs_.a + value;
       
       regs_.set_flag_z(result & 0xFF == 0);
       regs_.set_flag_n(false);
       regs_.set_flag_h((regs_.a & 0x0F) + (value & 0x0F) > 0x0F);
       regs_.set_flag_c(result > 0xFF);
       
       regs_.a = result & 0xFF;
   }

**Why explicit?** Clear flag behavior, easier to debug.

Memory System
-------------

Address Space Layout
~~~~~~~~~~~~~~~~~~~~

Memory is memory-mapped, not banked internally:

.. code-block:: cpp

   u8 Memory::read(u16 address) const {
       if (address < 0x8000) {
           return cartridge_->read(address);  // ROM
       } else if (address < 0xA000) {
           return vram_[address - 0x8000];     // VRAM
       } else if (address < 0xC000) {
           return cartridge_->read(address);   // External RAM
       }
       // ... etc
   }

**Design**: Centralized address decoding.

**Benefits**:

* Single source of truth
* Easy to trace memory access
* Clean cartridge integration

Why Not Memory Callbacks?
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Alternative**: Register read/write callbacks for each region.

**Why not used**:

* More complex
* Harder to trace
* Less clear control flow

PPU Implementation
------------------

Mode-Based State Machine
~~~~~~~~~~~~~~~~~~~~~~~~~

PPU operates in 4 modes per scanline:

.. code-block:: cpp

   void PPU::step(Cycles cycles) {
       cycles_ += cycles;
       
       switch (mode_) {
           case Mode::OAMSearch:
               if (cycles_ >= 80) {
                   mode_ = Mode::Transfer;
                   cycles_ -= 80;
               }
               break;
           
           case Mode::Transfer:
               if (cycles_ >= 172) {
                   mode_ = Mode::HBlank;
                   render_scanline();
                   cycles_ -= 172;
               }
               break;
           
           // ... VBlank handling
       }
   }

**Design**: Accumulate cycles, transition when threshold reached.

**Benefits**:

* Matches hardware behavior
* Easy to understand timing
* Clear mode transitions

Rendering Strategy
~~~~~~~~~~~~~~~~~~

Currently: Render entire scanline when entering HBlank.

**Future**: Pixel-by-pixel FIFO for accurate mid-scanline effects.

**Tradeoff**: Current approach is simpler but less accurate for edge cases.

Tile Decoding
~~~~~~~~~~~~~

Game Boy tiles are 2-bit planar format:

.. code-block:: cpp

   u8 PPU::get_tile_pixel(u8 tile_index, u8 x, u8 y) const {
       u16 tile_addr = 0x8000 + (tile_index * 16) + (y * 2);
       u8 low = memory_.read(tile_addr);
       u8 high = memory_.read(tile_addr + 1);
       
       u8 bit = 7 - x;
       u8 color = ((high >> bit) & 1) << 1 | ((low >> bit) & 1);
       
       return color;  // 0-3
   }

**Format**: Two bytes per row, one bit per pixel per byte.

Testing Strategy
----------------

Test Levels
~~~~~~~~~~~

1. **Unit Tests**: Individual component behavior
2. **ROM Tests**: Run actual Game Boy ROMs

Current Coverage
~~~~~~~~~~~~~~~~

.. code-block:: text

   ✅ CPU registers and flags
   ✅ Memory read/write
   ✅ Basic CPU instructions
   ✅ ALU operations
   ⏳ PPU rendering (partial)
   ⏳ Interrupts (partial)
   ❌ Cartridge types (needs more)
   ❌ Full ROM tests

Test Philosophy
~~~~~~~~~~~~~~~

* **Simple is better**: Basic assertions, clear failures
* **Test behavior, not implementation**: Public API only
* **One concept per test**: Focused tests
* **Descriptive names**: ``test_add_sets_carry_flag()``

Known Limitations
-----------------

Not Yet Implemented
~~~~~~~~~~~~~~~~~~~

* Serial communication
* Game Boy Color features (CGB palette rendering, VRAM banking)
* Pixel FIFO (currently renders entire scanline at once)

Accuracy Issues
~~~~~~~~~~~~~~~

* No instruction timing delays (all instant)
* No memory access delays
* PPU FIFO not implemented (render entire scanline)
* No DMA transfer timing
* HALT bug not emulated

Will Not Implement
~~~~~~~~~~~~~~~~~~

* Super Game Boy features
* Game Boy Player features
* Link cable hardware timing
* Exact power-on state randomness

Future Roadmap
--------------

Phase A: CGB Support
~~~~~~~~~~~~~~~~~~~~

* CGB palette rendering pipeline
* VRAM banking (bank 0/1)
* Double-speed mode

Phase B: Pixel FIFO
~~~~~~~~~~~~~~~~~~~~

* Pixel-by-pixel rendering
* Mid-scanline effects
* Accurate mode 3 length variation

Design Patterns Used
--------------------

Strategy Pattern
~~~~~~~~~~~~~~~~

Cartridge types use strategy pattern:

.. code-block:: cpp

   class Cartridge {
   public:
       virtual u8 read(u16 address) const = 0;
       virtual void write(u16 address, u8 value) = 0;
   };
   
   class MBC1 : public Cartridge { /* ... */ };
   class MBC3 : public Cartridge { /* ... */ };

Dependency Injection
~~~~~~~~~~~~~~~~~~~~

Components receive dependencies via constructor:

.. code-block:: cpp

   class CPU {
   public:
       explicit CPU(Memory& memory) : memory_(memory) {}
       
   private:
       Memory& memory_;  // Injected dependency
   };

**Benefits**: Easy to test, clear dependencies.

RAII
~~~~

Resources managed via RAII:

.. code-block:: cpp

   class Cartridge {
   public:
       ~Cartridge() {
           if (has_battery_) {
               save_ram();  // Automatic save on destruction
           }
       }
   };

Sprite Rendering Implementation
--------------------------------

Overview
~~~~~~~~

The Game Boy supports up to 40 sprites (8x8 or 8x16 pixels) with up to 10 visible per scanline. Sprite data is stored in Object Attribute Memory (OAM) at 0xFE00-0xFE9F.

OAM DMA Transfer
~~~~~~~~~~~~~~~~

**Problem**: Games need to quickly copy sprite data from RAM to OAM each frame.

**Solution**: OAM DMA (Direct Memory Access) register at 0xFF46.

Implementation Details
^^^^^^^^^^^^^^^^^^^^^^

When a game writes to register 0xFF46:

.. code-block:: cpp

   // Writing to 0xFF46 triggers DMA
   if (address == OAM_DMA_REG) {
       u16 source = static_cast<u16>(value) << OAM_DMA_SOURCE_SHIFT;
       
       for (u16 i = 0; i < OAM_SIZE; i++) {
           oam_[i] = read(source + i);
       }
       io_regs_[address - IO_REGISTERS_START] = value;
       return;
   }

**How it works**:

1. Source address = value × 0x100 (value is high byte only)
2. Copies 160 bytes (40 sprites × 4 bytes each) from source to OAM
3. Common source addresses: 0xC000-0xDF00 (Work RAM)

**Constants Used**:

.. code-block:: cpp

   constexpr u16 OAM_DMA_REG = 0xFF46;
   constexpr u16 OAM_SIZE = 160;  // 40 sprites × 4 bytes each
   constexpr u8 OAM_DMA_SOURCE_SHIFT = 8;  // Source address = value << 8

**Example Game Usage**:

.. code-block:: asm

   ; Copy sprite data from 0xC300 to OAM
   ld a, $C3      ; High byte of source address
   ldh ($46), a   ; Write to DMA register
   ; Hardware copies 160 bytes automatically

Sprite Visibility Logic
~~~~~~~~~~~~~~~~~~~~~~~

**Challenge**: Determining which sprites are visible on the current scanline.

**Solution**: Hardware-accurate visibility checks using raw Y coordinates.

Implementation
^^^^^^^^^^^^^^

.. code-block:: cpp

   void PPU::search_oam() {
       // Check all 40 sprites in OAM
       for (u16 i = 0; i < OAM_SPRITE_COUNT; i++) {
           Sprite sprite = read_sprite_from_oam(i);
           
           // Hardware logic: Compare against RAW Y value (not screen-adjusted)
           // Skip if: scanline >= Y OR scanline + 16 < Y
           if (ly_ >= sprite.y || ly_ + SPRITE_Y_VISIBILITY_OFFSET < sprite.y) {
               continue;
           }
           
           // For 8x8 mode: additional check
           // Skip if: scanline + 8 >= Y
           if (sprite_height == SPRITE_HEIGHT_8X8 && 
               ly_ + SPRITE_8X8_HEIGHT_CHECK >= sprite.y) {
               continue;
           }
           
           scanline_sprites_.push_back(sprite);
           
           // Hardware limit: max 10 sprites per scanline
           if (scanline_sprites_.size() >= MAX_SPRITES_PER_SCANLINE) {
               break;
           }
       }
   }

**Key Constants**:

.. code-block:: cpp

   constexpr u8 SPRITE_Y_VISIBILITY_OFFSET = 16;  // Sprites use Y+16 coordinate system
   constexpr u8 SPRITE_8X8_HEIGHT_CHECK = 8;      // For 8x8 sprite visibility
   constexpr u8 MAX_SPRITES_PER_SCANLINE = 10;    // Hardware limitation

**Why Y+16?**

Game Boy sprites use an offset coordinate system:

* Screen Y position 0 corresponds to sprite.y = 16
* Allows sprites to be partially off-screen at top
* Visible range: Y ∈ [0, 160] → sprite.y ∈ [16, 176]

8x16 Sprite Rendering
~~~~~~~~~~~~~~~~~~~~~

**Feature**: Sprites can be 8x8 or 8x16 pixels (LCDC bit 2).

**Challenge**: 8x16 sprites use two consecutive tiles and handle Y-flip differently.

Implementation
^^^^^^^^^^^^^^

.. code-block:: cpp

   void PPU::render_sprites() {
       for (const Sprite& sprite : scanline_sprites_) {
           // Calculate sprite row within the sprite
           int sprite_row = ly_ - static_cast<int>(sprite.y) + 
                           SPRITE_Y_VISIBILITY_OFFSET;
           
           u8 tile_num = sprite.tile;
           
           // Handle 8x16 mode
           if (sprite_height == SPRITE_HEIGHT_8X16) {
               tile_num &= SPRITE_8X16_TILE_MASK;  // Clear bit 0
               
               if (sprite_row >= SPRITE_8X16_ROW_THRESHOLD) {
                   sprite_row -= SPRITE_8X16_ROW_THRESHOLD;
                   tile_num++;
               }
               
               // Y-flip swaps tiles in 8x16 mode
               if (sprite.flags & (BIT_1 << SPRITE_FLAG_Y_FLIP_BIT)) {
                   tile_num ^= 1;
               }
           }
           
           // Apply Y-flip to row (for both 8x8 and within 8-pixel row for 8x16)
           if (sprite.flags & (BIT_1 << SPRITE_FLAG_Y_FLIP_BIT)) {
               sprite_row = SPRITE_ROW_MASK - sprite_row;
           }
           
           render_sprite_scanline(sprite, tile_num, sprite_row);
       }
   }

**8x16 Tile Selection**:

* Bit 0 of tile number is ignored (always even)
* Top tile: sprite.tile & 0xFE
* Bottom tile: (sprite.tile & 0xFE) + 1
* Y-flip: XOR tile number with 1 (swaps top/bottom)

**Constants**:

.. code-block:: cpp

   constexpr u8 SPRITE_8X16_TILE_MASK = 0xFE;      // Mask to clear bit 0
   constexpr u8 SPRITE_8X16_ROW_THRESHOLD = 8;     // Switch to bottom tile at row 8
   constexpr u8 SPRITE_ROW_MASK = 7;               // Row within 8-pixel tile (0-7)

Sprite Pixel Rendering
~~~~~~~~~~~~~~~~~~~~~~

**Function Signature Change**:

.. code-block:: cpp

   // Old (incorrect for 8x16):
   u8 get_sprite_pixel(const Sprite& sprite, u8 pixel_x, u8 pixel_y);
   
   // New (correct):
   u8 get_sprite_pixel(u8 tile_num, u8 sprite_flags, u8 pixel_x, u8 pixel_y);

**Why the change?**

* 8x16 sprites need to use calculated tile number, not sprite.tile
* Prevents using wrong tile for bottom half of 8x16 sprites
* Makes function more reusable (doesn't need full sprite structure)

**Implementation**:

.. code-block:: cpp

   u8 PPU::get_sprite_pixel(u8 tile_num, u8 sprite_flags, 
                            u8 pixel_x, u8 pixel_y) const {
       // Apply X-flip if needed
       if (sprite_flags & (BIT_1 << SPRITE_FLAG_X_FLIP_BIT)) {
           pixel_x = (TILE_SIZE - BIT_1) - pixel_x;
       }
       
       // Sprites always use tile data at 0x8000-0x8FFF
       u16 tile_addr = TILE_DATA_UNSIGNED_BASE + 
                      (tile_num * BYTES_PER_TILE) + 
                      (pixel_y * BYTES_PER_TILE_ROW);
       
       u8 byte1 = memory_.read(tile_addr);
       u8 byte2 = memory_.read(tile_addr + BIT_1);
       
       // Extract 2-bit pixel value
       u8 bit_pos = PIXEL_MSB_BIT - pixel_x;
       u8 pixel = ((byte2 >> bit_pos) & BIT_1) << PIXEL_BIT_SHIFT | 
                  ((byte1 >> bit_pos) & BIT_1);
       
       return pixel;
   }

Testing Sprite Rendering
~~~~~~~~~~~~~~~~~~~~~~~~~

**Expected Results**:

1. Boot-time sprites appear after DMA populates OAM
2. Scene sprites are visible with the expected priority rules
3. Player sprites render with the expected tile and flip handling

**Before Fix**: All sprites invisible (OAM remained zeros)

**After Fix**: All sprites render correctly

Debugging Tips
^^^^^^^^^^^^^^

If sprites don't appear:

1. **Check LCDC register**: Bit 1 must be set (sprites enabled)
2. **Verify OAM data**: Should not be all zeros after DMA
3. **Check visibility logic**: Ensure Y coordinate checks are correct
4. **Verify tile numbers**: 8x16 mode requires special handling
5. **Test with debug output**: Log first few OAM entries

Additional note: scanline sprites are stored contiguously.

Code Quality Improvements
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Magic Number Elimination**:

All magic numbers replaced with named constants:

.. code-block:: cpp

   // Before:
   if (ly_ >= sprite.y || ly_ + 16 < sprite.y)
   
   // After:
   if (ly_ >= sprite.y || ly_ + SPRITE_Y_VISIBILITY_OFFSET < sprite.y)

**Benefits**:

* Self-documenting code
* Easy to modify sprite behavior
* Prevents typos and errors
* Makes hardware constants explicit

**Complete Constant List**:

.. code-block:: cpp

   // Memory (memory.cpp)
   constexpr u16 OAM_DMA_REG = 0xFF46;
   constexpr u16 OAM_SIZE = 160;
   constexpr u8 OAM_DMA_SOURCE_SHIFT = 8;
   
   // PPU (ppu.cpp)
   constexpr u8 SPRITE_Y_VISIBILITY_OFFSET = 16;
   constexpr u8 SPRITE_8X8_HEIGHT_CHECK = 8;
   constexpr u8 SPRITE_8X16_TILE_MASK = 0xFE;
   constexpr u8 SPRITE_8X16_ROW_THRESHOLD = 8;
   constexpr u8 SPRITE_ROW_MASK = 7;

Lessons Learned
---------------

What Went Well
~~~~~~~~~~~~~~

* Clear architecture from start
* Modular design paid off
* Test coverage helped catch bugs
* Documentation-first approach
* **Incremental debugging**: Added logging to find OAM DMA issue

What Could Be Better
~~~~~~~~~~~~~~~~~~~~

* Should have implemented sprites earlier
* PPU could be more modular
* Need better test ROM integration
* **OAM DMA was missing**: Should have been implemented from start

Common Pitfalls
~~~~~~~~~~~~~~~

**Sprite Rendering Issues**:

1. **Forgetting OAM DMA**: Games don't write directly to OAM
2. **Wrong Y coordinates**: Must use raw Y values, not screen-adjusted
3. **8x16 tile calculation**: Must calculate tile_num, not use sprite.tile
4. **Y-flip in 8x16 mode**: Swaps tiles, not just pixels

Advice for Contributors
~~~~~~~~~~~~~~~~~~~~~~~

1. **Read the docs**: Architecture docs first
2. **Start small**: Pick one feature
3. **Test thoroughly**: Add tests with code
4. **Ask questions**: GitHub discussions
5. **Be patient**: Emulation is complex
6. **Debug with logging**: Temporary debug output catches issues

References
----------

* Pan Docs: https://gbdev.io/pandocs/
* Game Boy CPU Manual: http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
* Awesome GB Dev: https://github.com/gbdev/awesome-gbdev
* The Cycle-Accurate GB Docs: https://github.com/AntonioND/giibiiadvance

