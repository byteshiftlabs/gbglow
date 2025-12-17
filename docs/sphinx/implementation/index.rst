Implementation Notes
====================

Design decisions, implementation details, and technical notes.

Project Philosophy
------------------

Crystal-Clear Code
~~~~~~~~~~~~~~~~~~

EmuGBC prioritizes **code clarity** above all else:

* Self-documenting code with meaningful names
* Comprehensive comments explaining WHY, not WHAT
* Extensive documentation at every level
* Simple, straightforward implementations

**Why?** Existing emulators (gnuboy, etc.) are functional but hard to understand. EmuGBC aims to be the go-to reference implementation for learning Game Boy emulation.

Accuracy vs Performance
~~~~~~~~~~~~~~~~~~~~~~~

EmuGBC targets **cycle-accurate** emulation:

* Each instruction returns exact cycle count
* PPU timing matches hardware
* Memory access delays are modeled

Performance is important but secondary to accuracy and clarity.

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

* Performance: Zero-cost abstractions, inline-able code
* Control: Manual memory management where needed
* Industry Standard: Proven for emulation (Dolphin, PCSX2, etc.)
* Modern Features: Smart pointers, constexpr, std::optional

**Alternatives Considered:**

* **Rust**: Great safety, but less industry adoption for emulation
* **C**: More control, but lacking modern abstractions
* **Python**: Too slow for cycle-accurate emulation

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

Future: Optional SDL integration for real-time rendering.

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
* No significant performance benefit

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
* No performance benefit
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

Performance Optimizations
-------------------------

Applied Optimizations
~~~~~~~~~~~~~~~~~~~~~

1. **Inline Small Functions**: Compiler inlines getters/setters
2. **Reuse Buffers**: PPU framebuffer allocated once
3. **Fast Paths**: Common memory regions checked first
4. **Constexpr**: Constants evaluated at compile time

Not Yet Optimized
~~~~~~~~~~~~~~~~~

1. **Instruction Dispatch**: Could use computed goto
2. **Memory Access**: Could cache last access region
3. **PPU Rendering**: Could use SIMD for pixel processing

**Philosophy**: Optimize only when profiling shows need.

Testing Strategy
----------------

Test Levels
~~~~~~~~~~~

1. **Unit Tests**: Individual component behavior
2. **Integration Tests**: Components working together  
3. **ROM Tests**: Run actual Game Boy ROMs

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

* Sprite rendering
* Window layer
* LCD STAT interrupt modes
* Timer interrupt
* Serial communication
* Audio Processing Unit
* Game Boy Color features
* MBC3/MBC5 cartridges
* Save states

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

Phase 3: Sprite Rendering
~~~~~~~~~~~~~~~~~~~~~~~~~~

* OAM search implementation
* Sprite priority handling
* 8x8 and 8x16 sprite modes
* Sprite-background priority

Phase 4: Complete PPU
~~~~~~~~~~~~~~~~~~~~~~

* Window layer rendering
* Pixel FIFO implementation
* Mid-scanline effects
* LCD STAT interrupt modes

Phase 5: Complete Interrupts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Timer interrupt
* Serial interrupt
* Joypad interrupt
* Proper interrupt timing

Phase 6: Audio
~~~~~~~~~~~~~~

* Sound channels 1-4
* Audio mixer
* Sample generation
* Volume/envelope control

Phase 7: Advanced Features
~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Game Boy Color support
* MBC3/MBC5 cartridges
* Save states
* Rewind feature
* Debugger interface

Phase 8: Optimization
~~~~~~~~~~~~~~~~~~~~~

* Profile-guided optimization
* SIMD for rendering
* JIT compilation (maybe)

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

Performance Notes
-----------------

Profiling Results
~~~~~~~~~~~~~~~~~

Typical frame breakdown (Release build, modern CPU):

.. code-block:: text

   CPU execution:     60%
   PPU rendering:     30%
   Memory access:      8%
   Other:              2%

Hot Spots
~~~~~~~~~

1. **CPU::execute_instruction()**: Switch dispatch
2. **Memory::read()**: Address decoding
3. **PPU::render_scanline()**: Tile lookups

Target Performance
~~~~~~~~~~~~~~~~~~

* Single frame: < 1ms (1000+ fps possible)
* Real-time: 16.67ms per frame (60 fps)
* Headroom: 16× for future features

Lessons Learned
---------------

What Went Well
~~~~~~~~~~~~~~

* Clear architecture from start
* Modular design paid off
* Comprehensive testing helped catch bugs
* Documentation-first approach

What Could Be Better
~~~~~~~~~~~~~~~~~~~~

* Should have implemented sprites earlier
* PPU could be more modular
* Need better test ROM integration
* Should profile earlier

Advice for Contributors
~~~~~~~~~~~~~~~~~~~~~~~

1. **Read the docs**: Architecture docs first
2. **Start small**: Pick one feature
3. **Test thoroughly**: Add tests with code
4. **Ask questions**: GitHub discussions
5. **Be patient**: Emulation is complex

References
----------

* Pan Docs: https://gbdev.io/pandocs/
* Game Boy CPU Manual: http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
* Awesome GB Dev: https://github.com/gbdev/awesome-gbdev
* The Cycle-Accurate GB Docs: https://github.com/AntonioND/giibiiadvance
