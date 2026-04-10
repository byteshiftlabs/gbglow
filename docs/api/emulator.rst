Emulator Class
==============

The main coordinator class that ties all components together.

Class Declaration
-----------------

.. code-block:: cpp

   class Emulator {
   public:
       Emulator();
       ~Emulator();
       
       bool load_rom(const std::string& path);
       void reset();
       
       void run_frame();
       Cycles run_cycles(Cycles cycles);
       
       CPU& get_cpu();
       PPU& get_ppu();
       Memory& get_memory();
       
       const CPU& get_cpu() const;
       const PPU& get_ppu() const;
       const Memory& get_memory() const;
       
   private:
       Memory memory_;
       CPU cpu_;
       PPU ppu_;
   };

Constructor
-----------

.. code-block:: cpp

   Emulator::Emulator()
       : cpu_(memory_),
         ppu_(memory_)
   {
   }

Creates a new emulator instance with:

* 64KB memory system
* Sharp LR35902 CPU
* Picture Processing Unit
* All components initialized to power-on state

ROM Loading
-----------

load_rom()
~~~~~~~~~~

.. code-block:: cpp

   bool load_rom(const std::string& path);

Loads a Game Boy ROM file.

**Parameters**
   * ``path`` - Path to ROM file (.gb or .gbc)

**Returns**
   * ``true`` on success
   * ``false`` on failure

**Example**

.. code-block:: cpp

   Emulator emulator;
   if (!emulator.load_rom("tetris.gb")) {
       std::cerr << "Failed to load ROM\n";
       return 1;
   }

**Supported Formats**
   * ROM-only cartridges
   * MBC1 cartridges
   * Battery-backed RAM

**Throws**
   * ``std::runtime_error`` - Unsupported cartridge type
   * ``std::ios_base::failure`` - File I/O error

Execution Control
-----------------

run_frame()
~~~~~~~~~~~

.. code-block:: cpp

   void run_frame();

Executes exactly one frame (70224 cycles, ~16.67ms).

**Frame Duration**
   * 70224 M-cycles
   * ~59.7 Hz (60 fps)
   * Includes VBlank period

**Example**

.. code-block:: cpp

   // Main emulation loop
   while (running) {
       emulator.run_frame();
       
       // Render frame
       const auto& framebuffer = emulator.get_ppu().get_framebuffer();
       render_to_screen(framebuffer);
       
       // Cap to 60 fps
       std::this_thread::sleep_until(next_frame_time);
       next_frame_time += 16ms;
   }

run_cycles()
~~~~~~~~~~~~

.. code-block:: cpp

   Cycles run_cycles(Cycles cycles);

Executes a specific number of cycles.

**Parameters**
   * ``cycles`` - Number of M-cycles to execute

**Returns**
   * Actual cycles executed (may differ slightly)

**Example**

.. code-block:: cpp

   // Run until next VBlank
   Cycles cycles_until_vblank = calculate_vblank_cycles();
   emulator.run_cycles(cycles_until_vblank);

**Implementation**

.. code-block:: cpp

   Cycles Emulator::run_cycles(Cycles cycles) {
       Cycles executed = 0;
       
       while (executed < cycles) {
           // Execute one instruction
           Cycles instruction_cycles = cpu_.step();
           
           // Update PPU with same timing
           ppu_.step(instruction_cycles);
           
           executed += instruction_cycles;
       }
       
       return executed;
   }

Reset
-----

reset()
~~~~~~~

.. code-block:: cpp

   void reset();

Resets emulator to power-on state.

**Resets**
   * All CPU registers
   * Memory contents
   * PPU state
   * Cartridge state (if applicable)

**Does Not Reset**
   * Loaded ROM
   * Battery-backed RAM

**Example**

.. code-block:: cpp

   // Reset after game over
   if (game_over) {
       emulator.reset();
   }

Component Access
----------------

get_cpu()
~~~~~~~~~

.. code-block:: cpp

   CPU& get_cpu();
   const CPU& get_cpu() const;

Returns reference to CPU instance.

**Use Cases**
   * Read register values
   * Inspect CPU state
   * Debug support

**Example**

.. code-block:: cpp

   const auto& cpu = emulator.get_cpu();
   const auto& regs = cpu.get_registers();
   
   std::cout << "PC: " << std::hex << regs.pc << '\n';
   std::cout << "SP: " << std::hex << regs.sp << '\n';

get_ppu()
~~~~~~~~~

.. code-block:: cpp

   PPU& get_ppu();
   const PPU& get_ppu() const;

Returns reference to PPU instance.

**Use Cases**
   * Access framebuffer
   * Render display
   * Debug graphics

**Example**

.. code-block:: cpp

   const auto& ppu = emulator.get_ppu();
   const auto& fb = ppu.get_framebuffer();
   
   // Render to screen
   for (int y = 0; y < 144; ++y) {
       for (int x = 0; x < 160; ++x) {
           u8 color = fb[y * 160 + x];
           put_pixel(x, y, color);
       }
   }

get_memory()
~~~~~~~~~~~~

.. code-block:: cpp

   Memory& get_memory();
   const Memory& get_memory() const;

Returns reference to Memory instance.

**Use Cases**
   * Direct memory access
   * Save states
   * Memory debugging

**Example**

.. code-block:: cpp

   const auto& memory = emulator.get_memory();
   
   // Read WRAM
   u8 value = memory.read(0xC000);
   
   // Dump memory region
   for (u16 addr = 0xC000; addr < 0xD000; ++addr) {
       std::cout << std::hex << static_cast<int>(memory.read(addr)) << ' ';
   }

State Management
----------------

Save States
~~~~~~~~~~~

Not yet implemented. Future API:

.. code-block:: cpp

   class Emulator {
   public:
       // Save current state
       std::vector<u8> save_state() const;
       
       // Restore saved state
       void load_state(const std::vector<u8>& state);
   };

**Will Include**
   * All CPU registers
   * All memory
   * PPU state
   * Cartridge state

Performance
-----------

Timing
~~~~~~

One frame execution:

* 70224 M-cycles
* ~180,000 instructions
* ~10-20ms on modern CPU

Optimization Tips
~~~~~~~~~~~~~~~~~

1. **Batch Execution**: Use ``run_frame()`` instead of ``run_cycles(1)``
2. **Reduce Rendering**: Don't render every frame if not needed
3. **Release Build**: Use -O3 optimization
4. **Profile**: Focus on CPU instruction dispatch

Example: Frame Skipping
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   constexpr int FRAME_SKIP = 2;  // Render every 3rd frame
   
   for (int frame = 0; frame < 180; ++frame) {
       emulator.run_frame();
       
       if (frame % (FRAME_SKIP + 1) == 0) {
           // Only render some frames
           emulator.get_ppu().render_to_terminal();
       }
   }

Thread Safety
-------------

The ``Emulator`` class is **not thread-safe**.

For multi-threaded use:

.. code-block:: cpp

   // Emulation thread
   std::thread emu_thread([&emulator]() {
       while (running) {
           emulator.run_frame();
       }
   });
   
   // Render thread (needs synchronization!)
   std::mutex frame_mutex;
   std::thread render_thread([&emulator, &frame_mutex]() {
       while (running) {
           std::lock_guard lock(frame_mutex);
           const auto& fb = emulator.get_ppu().get_framebuffer();
           render(fb);
       }
   });

Debugging
---------

Example: Breakpoint Support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class DebuggingEmulator : public Emulator {
   public:
       void add_breakpoint(u16 address) {
           breakpoints_.insert(address);
       }
       
       void run_until_breakpoint() {
           while (true) {
               auto& cpu = get_cpu();
               u16 pc = cpu.get_registers().pc;
               
               if (breakpoints_.count(pc)) {
                   std::cout << "Breakpoint at " << std::hex << pc << '\n';
                   break;
               }
               
               run_cycles(1);
           }
       }
       
   private:
       std::set<u16> breakpoints_;
   };

Example: Instruction Tracing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void trace_execution(Emulator& emulator, int num_instructions) {
       auto& cpu = emulator.get_cpu();
       auto& memory = emulator.get_memory();
       
       for (int i = 0; i < num_instructions; ++i) {
           auto& regs = cpu.get_registers();
           u8 opcode = memory.read(regs.pc);
           
           std::cout << std::hex 
                     << "PC:" << regs.pc << " "
                     << "OP:" << static_cast<int>(opcode) << " "
                     << "A:" << static_cast<int>(regs.a) << '\n';
           
           emulator.run_cycles(1);
       }
   }
