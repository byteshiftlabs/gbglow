gbglow Documentation
====================

**A Game Boy Color Emulator**

Welcome to the gbglow documentation. This site covers the emulator layout,
architecture notes, API reference, and contributor guidance.

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

   ./build/gbglow <rom_file.gb>

Features
--------

* **CPU**: Instruction decoding and execution for the LR35902 core
* **PPU**: Background and sprite rendering with DMG and CGB palette handling
* **APU**: Four audio channels
* **Cartridge Support**: MBC1, MBC3, and MBC5 memory bank controllers
* **Save States**: Slot-based state persistence
* **GUI Menu**: ImGui-based interface and debugger overlay
* **Configurable Controls**: Rebindable keyboard controls with persistence

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
