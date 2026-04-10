GBCrush Documentation
=====================

**A Game Boy Color Emulator**

Welcome to GBCrush, a Game Boy Color emulator implementation focused on code clarity, 
maintainability, and educational value. Every component is thoroughly documented with 
references to actual Game Boy hardware behavior.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started
   
   overview

.. toctree::
   :maxdepth: 2
   :caption: Architecture
   
   architecture/index

.. toctree::
   :maxdepth: 2
   :caption: API Reference
   
   api/index

.. toctree::
   :maxdepth: 2
   :caption: Development
   
   implementation/index
   development/index

Quick Start
-----------

Building the Emulator
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   ./build.sh

Running a ROM
~~~~~~~~~~~~~

.. code-block:: bash

   ./build/gbcrush <rom_file.gb>

Features
--------

* **Complete CPU Emulation**: All 180+ opcodes including CB instructions
* **PPU with CGB Support**: Background/sprite rendering, DMG and CGB color palettes
* **APU**: 4-channel audio with real-time playback
* **Cartridge Support**: MBC1, MBC3, MBC5 memory bank controllers
* **Save States**: 10 slots with metadata and timestamps
* **GUI Menu**: ImGui-based interface with configuration options
* **Configurable Controls**: Rebindable keyboard controls with persistence
* **Performance**: Optimized with -O3, LTO, and native CPU instructions

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
