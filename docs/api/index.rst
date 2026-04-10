API Reference
=============

This section documents the public API of GBCrush's core components.

.. toctree::
   :maxdepth: 2
   :caption: Core Components

   emulator
   cpu
   memory
   registers

.. toctree::
   :maxdepth: 2
   :caption: Video System

   ppu
   lcd

.. toctree::
   :maxdepth: 2
   :caption: Cartridge System

   cartridge
   mbc

Core Types
----------

Common type definitions used throughout the codebase.

Basic Types
~~~~~~~~~~~

.. code-block:: cpp

   // Unsigned integer types
   using u8  = std::uint8_t;    // 8-bit unsigned
   using u16 = std::uint16_t;   // 16-bit unsigned
   using u32 = std::uint32_t;   // 32-bit unsigned
   using u64 = std::uint64_t;   // 64-bit unsigned
   
   // Signed integer types
   using i8  = std::int8_t;     // 8-bit signed
   using i16 = std::int16_t;    // 16-bit signed
   using i32 = std::int32_t;    // 32-bit signed
   using i64 = std::int64_t;    // 64-bit signed
   
   // Cycle counting
   using Cycles = u32;           // CPU cycle count

File: ``src/core/types.h``

Constants
~~~~~~~~~

.. code-block:: cpp

   // Memory sizes
   constexpr size_t MEMORY_SIZE = 0x10000;  // 64KB
   constexpr size_t VRAM_SIZE = 0x2000;     // 8KB
   constexpr size_t WRAM_SIZE = 0x2000;     // 8KB
   constexpr size_t OAM_SIZE = 0xA0;        // 160 bytes
   constexpr size_t HRAM_SIZE = 0x7F;       // 127 bytes
   
   // Screen dimensions
   constexpr int SCREEN_WIDTH = 160;
   constexpr int SCREEN_HEIGHT = 144;
   
   // Timing
   constexpr Cycles CYCLES_PER_FRAME = 70224;  // ~59.7 Hz
   constexpr Cycles CYCLES_PER_SCANLINE = 456;

Quick Start
-----------

Basic usage example:

.. code-block:: cpp

   #include "core/emulator.h"
   
   int main(int argc, char* argv[]) {
       if (argc < 2) {
           std::cerr << "Usage: " << argv[0] << " <rom_file>\n";
           return 1;
       }
       
       // Create emulator
       Emulator emulator;
       
       // Load ROM
       if (!emulator.load_rom(argv[1])) {
           std::cerr << "Failed to load ROM\n";
           return 1;
       }
       
       // Run for 60 frames (1 second at 60fps)
       for (int frame = 0; frame < 60; ++frame) {
           emulator.run_frame();
           
           // Render every frame
           emulator.get_ppu().render_to_terminal();
       }
       
       return 0;
   }

Error Handling
--------------

GBCrush uses exceptions for error handling:

.. code-block:: cpp

   try {
       Emulator emulator;
       emulator.load_rom("game.gb");
       emulator.run_frame();
   } catch (const std::exception& e) {
       std::cerr << "Error: " << e.what() << '\n';
       return 1;
   }

Common exceptions:

* ``std::runtime_error`` - General runtime errors
* ``std::invalid_argument`` - Invalid parameters
* ``std::ios_base::failure`` - File I/O errors

Thread Safety
-------------

GBCrush is **not thread-safe**. All API calls should be made from a single thread.

For multi-threaded applications:

* Create separate ``Emulator`` instances per thread
* Use external synchronization if sharing state
* Render calls must be synchronized with emulation

Performance Considerations
--------------------------

Cycle Accuracy
~~~~~~~~~~~~~~

EmuGBC aims for cycle-accurate emulation:

* Each instruction returns its exact cycle count
* PPU operates at correct timing
* Memory access delays are modeled

For best performance:

* Batch frame execution with ``run_frame()``
* Minimize rendering calls
* Use Release builds (-O3 optimization)

Memory Usage
~~~~~~~~~~~~

Typical memory footprint:

* Base emulator: ~100KB
* Cartridge ROM: 32KB - 8MB
* Save RAM: 8KB - 128KB
* Video framebuffer: 23KB (160×144)

Total: ~150KB + ROM size

Building
--------

See :doc:`../development/index` for build instructions.

Examples
--------

See ``examples/`` directory for complete examples:

* Basic emulator usage
* Custom input handling
* Debugging tools
* Performance testing
