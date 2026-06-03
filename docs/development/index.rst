Development Guide
=================

How to build, test, and contribute to gbglow from the current repository state.

Prerequisites
-------------

Required tools:

* C++17 compiler
* CMake 3.14 or newer
* ``pkg-config``
* SDL2 development package discoverable as ``sdl2`` via ``pkg-config``
* ``cppcheck``
* Git

Ubuntu 24.04 example:

.. code-block:: bash

   sudo apt install build-essential cmake pkg-config libsdl2-dev cppcheck

Or use the helper script from the repository root:

.. code-block:: bash

   sudo bash ./install_deps_ubuntu.sh

Repository layout
-----------------

Main project areas:

* ``src/core``: CPU, memory, timer, emulator loop, save-state I/O
* ``src/video``: PPU and SDL2 display integration
* ``src/audio``: APU implementation
* ``src/cartridge``: cartridge loading and MBC implementations
* ``src/input``: joypad and gamepad input
* ``src/debug``: debugger and ImGui debugger UI
* ``src/ui``: recent ROMs list and screenshot support
* ``tests/test_core.cpp``: CPU, memory, timer, and cartridge tests
* ``tests/test_persistence.cpp``: save-state and persistence tests
* ``tests/test_ppu.cpp``: PPU behavior tests
* ``docs/architecture``: architecture and subsystem documentation

Build
-----

Clone the repository:

.. code-block:: bash

   git clone https://github.com/byteshiftlabs/gbglow.git
   cd gbglow

Recommended build path:

.. code-block:: bash

   ./build.sh

``build.sh`` performs all of the following:

* dependency checks for ``cmake``, ``pkg-config``, ``cppcheck``, and SDL2
* Dear ImGui download through CMake ``FetchContent`` during configure
* CMake configure into ``build/``
* project build
* test execution with ``ctest --output-on-failure``
* ``cppcheck`` static analysis

Clean rebuild:

.. code-block:: bash

   ./build.sh --clean

Manual CMake flow:

.. code-block:: bash

   mkdir -p build
   cd build
   cmake ..
   cmake --build . -j"$(nproc)"

Build types
-----------

The project defaults to ``Release`` when no build type is specified.

Debug build:

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build -j"$(nproc)"

Release build:

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j"$(nproc)"

RelWithDebInfo build:

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
   cmake --build build -j"$(nproc)"

Tests
-----

Current test executables:

* ``test_core`` built from ``tests/test_core.cpp``
* ``test_persistence`` built from ``tests/test_persistence.cpp``
* ``test_ppu`` built from ``tests/test_ppu.cpp``

Build and run tests manually:

.. code-block:: bash

   cmake --build build --target test_core test_persistence test_ppu
   cd build
   ctest --output-on-failure

Run the test binary directly:

.. code-block:: bash

   ./build/tests/test_core
   ./build/tests/test_persistence
   ./build/tests/test_ppu

Current coverage is focused and limited. It includes core CPU, memory, timer, cartridge, and selected utility behavior. It does not yet provide broad subsystem coverage for the full emulator runtime.

Static analysis
---------------

Run ``cppcheck`` manually:

.. code-block:: bash

   cppcheck --enable=all --inline-suppr --quiet \
       --suppress=missingIncludeSystem \
       --suppress=missingInclude \
       --suppress=unmatchedSuppression \
       --suppressions-list=cppcheck.suppressions \
       --error-exitcode=1 \
       -I src/ src/

If a suppression is necessary, keep it narrow and add a short justification comment in ``cppcheck.suppressions``.

Run
---

Run a ROM directly after a successful build:

.. code-block:: bash

   ./build/gbglow /path/to/game.gb

Key controls currently documented in the public README:

* Arrow keys: D-pad
* ``Z``: A
* ``X``: B
* ``Enter``: Start
* ``Shift``: Select
* ``Ctrl+O``: open ROM dialog
* ``Ctrl+R``: reset emulator
* ``F1`` to ``F9``: save state slots
* ``Shift+F1`` to ``Shift+F9``: load state slots
* ``F11``: toggle debugger
* ``F12``: capture screenshot
* ``ESC``: exit

Workflow
--------

The repository currently uses ``main`` as the active base branch.

Contribution workflow:

1. create a branch from ``main``
2. implement the change incrementally
3. run ``./build.sh`` and keep the pipeline clean
4. open a pull request targeting ``main``

Branch naming examples used in current contributor guidance:

* ``feature/mbc2-support``
* ``fix/timer-overflow``
* ``refactor/ppu-cleanup``
* ``docs/memory-map``

Code standards
--------------

Project expectations taken from current source and contributor guidance:

* C++17 only
* treat warnings as errors
* avoid magic numbers; prefer ``constexpr`` named constants
* keep public interfaces documented in headers
* preserve the existing SPDX and copyright header on source files
* prefer small, reviewable changes over large mixed refactors

References
----------

Additional project documentation:

* ``README.md``
* ``CONTRIBUTING.md``
* ``docs/architecture/index.rst``

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
   gdb ./gbglow
   (gdb) break CPU::step
   (gdb) run tetris.gb
   
   # LLDB
   lldb ./gbglow
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
   ./gbglow --trace rom.gb > gbglow_trace.txt
   
   # Compare with reference emulator
   ./bgb --trace rom.gb > bgb_trace.txt
   diff gbglow_trace.txt bgb_trace.txt

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
* [ ] Runtime behavior reviewed

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

* `Pan Docs <https://gbdev.io/pandocs/>`_ - Hardware reference
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
