EmuGBC Documentation
====================

**A Crystal-Clear Game Boy Color Emulator**

EmuGBC is a Game Boy Color emulator implementation focused on code clarity, maintainability, 
and educational value. Every component is thoroughly documented with references to actual 
Game Boy hardware behavior.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   overview
   architecture/index
   api/index
   implementation/index
   development/index

Quick Start
-----------

Building the Emulator
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   mkdir build
   cd build
   cmake ..
   make

Running a ROM
~~~~~~~~~~~~~

.. code-block:: bash

   ./emugbc <rom_file.gb> [frames]

Current Status
--------------

**Phase 1: CPU ✅ COMPLETE**

* 180+ opcodes implemented
* All CB instructions (bit operations)
* Full ALU operations
* Complete jump/call/return logic

**Phase 2: PPU ✅ COMPLETE**

* Background tile rendering
* LCD timing and modes
* VBlank interrupt support
* ASCII terminal output

**Phase 3: In Progress**

* Additional interrupt handling
* Joypad input
* Audio processing unit

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
