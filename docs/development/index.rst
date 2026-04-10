Development Guide
=================

How to build, test, and contribute to GBCrush.

Building from Source
--------------------

Prerequisites
~~~~~~~~~~~~~

Required:

* C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
* CMake 3.15 or higher
* Git

Optional:

* Sphinx (for documentation)
* Doxygen (for code documentation)

Clone Repository
~~~~~~~~~~~~~~~~

.. code-block:: bash

   git clone https://github.com/cmelnulabs/gbcrush.git
   cd gbcrush

Build Steps
~~~~~~~~~~~

.. code-block:: bash

   # Create build directory
   mkdir build
   cd build
   
   # Configure
   cmake ..
   
   # Build
   cmake --build .
   
   # The executable is at: build/gbcrush

Build Types
~~~~~~~~~~~

**Debug Build** (default):

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=Debug ..
   cmake --build .

Features:

* Debug symbols enabled
* Optimizations disabled
* Assertions enabled
* Slower execution

**Release Build**:

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .

Features:

* Full optimizations (-O3)
* No debug symbols
* Assertions disabled
* Maximum performance

**Release with Debug Info**:

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
   cmake --build .

Features:

* Optimizations enabled
* Debug symbols included
* Good for profiling

Running Tests
-------------

Build Tests
~~~~~~~~~~~

Tests are built by default:

.. code-block:: bash

   cmake --build . --target test_basic

Run Tests
~~~~~~~~~

.. code-block:: bash

   # Run all tests
   ctest
   
   # Run specific test
   ./test_basic
   
   # Verbose output
   ctest --verbose

Test Organization
~~~~~~~~~~~~~~~~~

.. code-block:: text

   tests/
   ├── test_basic.cpp      # Core component tests
   ├── test_cpu.cpp        # CPU instruction tests
   ├── test_memory.cpp     # Memory system tests
   └── test_ppu.cpp        # Graphics tests

Writing Tests
~~~~~~~~~~~~~

EmuGBC uses a simple assertion-based test framework:

.. code-block:: cpp

   #include "test_framework.h"
   #include "core/cpu.h"
   
   void test_ld_instructions() {
       Memory memory;
       CPU cpu(memory);
       
       // Test LD A, 42
       memory.write(0x0000, 0x3E);  // LD A, n
       memory.write(0x0001, 42);
       
       Cycles cycles = cpu.step();
       
       assert(cpu.get_registers().a == 42);
       assert(cycles == 2);
       
       test_pass("LD A, n works correctly");
   }

Development Workflow
--------------------

Branching Strategy
~~~~~~~~~~~~~~~~~~

.. code-block:: text

   main           - Stable releases
   develop        - Development branch
   feature/*      - New features
   bugfix/*       - Bug fixes
   docs/*         - Documentation updates

Creating a Feature
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Create feature branch
   git checkout -b feature/sprite-rendering develop
   
   # Make changes
   # ... edit code ...
   
   # Test
   cmake --build . && ctest
   
   # Commit
   git add .
   git commit -m "Add sprite rendering support"
   
   # Push
   git push origin feature/sprite-rendering

Code Style
----------

General Principles
~~~~~~~~~~~~~~~~~~

* **Clarity over cleverness**
* **Consistent naming**
* **Well-documented code**
* **Minimal dependencies**

Naming Conventions
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Classes: PascalCase
   class PPU { };
   class MemoryBankController { };
   
   // Functions: snake_case
   void render_scanline();
   Cycles execute_instruction();
   
   // Variables: snake_case
   u16 program_counter;
   bool vblank_occurred;
   
   // Constants: UPPER_SNAKE_CASE
   constexpr int SCREEN_WIDTH = 160;
   constexpr Cycles CYCLES_PER_FRAME = 70224;
   
   // Private members: trailing underscore
   class Example {
   private:
       int value_;
       bool enabled_;
   };

File Organization
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Header file: example.h
   #pragma once
   
   #include <vector>
   #include "types.h"
   
   class Example {
   public:
       // Public interface first
       Example();
       void do_something();
       
   private:
       // Implementation details last
       void helper_function();
       int value_;
   };

.. code-block:: cpp

   // Implementation file: example.cpp
   #include "example.h"
   
   Example::Example()
       : value_(0)  // Initialize in declaration order
   {
   }
   
   void Example::do_something() {
       // Implementation
   }

Comments
~~~~~~~~

.. code-block:: cpp

   // Good: Explain WHY, not WHAT
   // Use 16-bit addition to set flags correctly
   regs_.af = add16(regs_.af, value);
   
   // Bad: States the obvious
   // Add value to AF register
   regs_.af += value;

.. code-block:: cpp

   // Good: Document non-obvious behavior
   // Game Boy Color hardware quirk: writes to DIV reset it to 0
   if (address == 0xFF04) {
       div_register_ = 0;
       return;
   }

Formatting
~~~~~~~~~~

Use consistent formatting (enforced by clang-format):

.. code-block:: cpp

   // Indentation: 4 spaces
   if (condition) {
       do_something();
   }
   
   // Braces: Same line for class/function, next line for control flow
   class Example {
       void function() {
           if (test) {
               action();
           }
       }
   };
   
   // Line length: Aim for 80-100 characters

Error Handling
~~~~~~~~~~~~~~

.. code-block:: cpp

   // Use exceptions for unrecoverable errors
   if (!file.is_open()) {
       throw std::runtime_error("Failed to open ROM file");
   }
   
   // Use return values for expected failures
   bool load_save_file() {
       std::ifstream file(save_path_);
       if (!file) {
           return false;  // Save file doesn't exist (OK)
       }
       // Load data...
       return true;
   }

Performance Guidelines
----------------------

Optimization Strategy
~~~~~~~~~~~~~~~~~~~~~

1. **Profile first**: Don't guess what's slow
2. **Algorithm before micro-optimization**: Better algorithm > faster loop
3. **Measure changes**: Verify improvements

Hot Paths
~~~~~~~~~

Focus optimization on:

* CPU instruction dispatch
* Memory read/write
* PPU pixel rendering

.. code-block:: cpp

   // Good: Fast path for common case
   u8 Memory::read(u16 address) const {
       // Fast WRAM access (most common)
       if (address >= 0xC000 && address < 0xE000) {
           return wram_[address - 0xC000];
       }
       
       // Handle other regions...
       return slow_path_read(address);
   }

Avoid Allocations
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Bad: Allocates every frame
   std::vector<Sprite> get_sprites() {
       std::vector<Sprite> sprites;
       // Build sprite list...
       return sprites;
   }
   
   // Good: Reuse storage
   class PPU {
       void render_frame() {
           sprite_buffer_.clear();
           // Fill sprite_buffer_...
       }
       
   private:
       std::vector<Sprite> sprite_buffer_;  // Reused
   };

Cache-Friendly Data
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Good: Tightly packed, cache-friendly
   struct Sprite {
       u8 y, x, tile, flags;  // 4 bytes
   };
   
   // Bad: Scattered data
   struct Sprite {
       u8 y;
       bool visible;
       u16 padding;
       u8 x;
       // ... scattered fields
   };

Debugging Tips
--------------

Enable Debug Logging
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #ifdef DEBUG
   #define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << '\n'
   #else
   #define LOG_DEBUG(msg)
   #endif
   
   // Usage
   LOG_DEBUG("Executing opcode: " << std::hex << opcode);

Use Debugger
~~~~~~~~~~~~

.. code-block:: bash

   # GDB
   gdb ./gbcrush
   (gdb) break CPU::step
   (gdb) run tetris.gb
   
   # LLDB
   lldb ./gbcrush
   (lldb) breakpoint set --name CPU::step
   (lldb) run tetris.gb

Instruction Tracing
~~~~~~~~~~~~~~~~~~~

Add temporary logging in CPU:

.. code-block:: cpp

   Cycles CPU::step() {
       u8 opcode = memory_.read(regs_.pc);
       
       // Trace execution
       std::cout << std::hex 
                 << "PC:" << regs_.pc << " "
                 << "OP:" << static_cast<int>(opcode) << " "
                 << "A:" << static_cast<int>(regs_.a) << " "
                 << "F:" << static_cast<int>(regs_.f) << '\n';
       
       // ... execute instruction
   }

Compare with Other Emulators
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Run same ROM in multiple emulators:

.. code-block:: bash

   # Generate execution trace
   ./emugbc --trace rom.gb > emugbc_trace.txt
   
   # Compare with reference emulator
   ./bgb --trace rom.gb > bgb_trace.txt
   diff emugbc_trace.txt bgb_trace.txt

Contributing
------------

Pull Request Process
~~~~~~~~~~~~~~~~~~~~

1. Fork the repository
2. Create feature branch from ``develop``
3. Make changes with clear commits
4. Add tests for new features
5. Ensure all tests pass
6. Update documentation
7. Submit pull request

Commit Messages
~~~~~~~~~~~~~~~

Use clear, descriptive commit messages:

.. code-block:: text

   Good:
   - "Implement sprite rendering with priority handling"
   - "Fix off-by-one error in OAM search timing"
   - "Add tests for MBC3 RTC functionality"
   
   Bad:
   - "fix bug"
   - "update"
   - "wip"

Code Review Checklist
~~~~~~~~~~~~~~~~~~~~~

Before submitting:

* [ ] Code compiles without warnings
* [ ] All tests pass
* [ ] New tests added for new features
* [ ] Documentation updated
* [ ] Code follows style guidelines
* [ ] No unnecessary dependencies added
* [ ] Performance impact considered

Documentation
-------------

Building Documentation
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cd docs/sphinx
   pip install -r requirements.txt
   make html
   
   # Open in browser
   open _build/html/index.html

Documentation Style
~~~~~~~~~~~~~~~~~~~

* Use clear, concise language
* Include code examples
* Reference Game Boy hardware docs
* Explain design decisions

Resources
---------

Game Boy Documentation
~~~~~~~~~~~~~~~~~~~~~~

* `Pan Docs <https://gbdev.io/pandocs/>`_ - Complete hardware reference
* `Game Boy CPU Manual <http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf>`_
* `The Cycle-Accurate Game Boy Docs <https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf>`_

Emulator Development
~~~~~~~~~~~~~~~~~~~~

* `Emulator Development Guide <https://github.com/gbdev/awesome-gbdev>`_
* `GB Opcodes <https://gbdev.io/gb-opcodes/>`_
* `BGB Emulator <https://bgb.bircd.org/>`_ - Reference for testing

Test ROMs
~~~~~~~~~

* `Blargg's Test ROMs <https://github.com/retrio/gb-test-roms>`_
* `Mooneye Test Suite <https://github.com/Gekkio/mooneye-test-suite>`_
* `dmg-acid2 <https://github.com/mattcurrie/dmg-acid2>`_
