GBCrush Documentation
=====================

**A Game Boy Color Emulator**

Welcome to GBCrush, a Game Boy Color emulator implementation focused on code clarity, 
maintainability, and educational value. Every component is thoroughly documented with 
references to actual Game Boy hardware behavior.

.. raw:: html

   <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1.5rem; margin: 2rem 0;">
     <div style="border: 1px solid #ddd; border-radius: 8px; padding: 1.5rem; background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);">
       <h3 style="margin-top: 0;">📖 Overview</h3>
       <p>Project philosophy, technology stack, and Game Boy hardware introduction with detailed CPU architecture.</p>
       <a href="overview.html" style="font-weight: bold;">Read Overview →</a>
     </div>
     <div style="border: 1px solid #ddd; border-radius: 8px; padding: 1.5rem; background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);">
       <h3 style="margin-top: 0;">🏗️ Architecture</h3>
       <p>Deep dive into CPU, PPU, APU, memory mapping, interrupts, and cartridge systems.</p>
       <a href="architecture/index.html" style="font-weight: bold;">Explore Architecture →</a>
     </div>
     <div style="border: 1px solid #ddd; border-radius: 8px; padding: 1.5rem; background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);">
       <h3 style="margin-top: 0;">📚 API Reference</h3>
       <p>Complete API documentation for all emulator classes, methods, and interfaces.</p>
       <a href="api/index.html" style="font-weight: bold;">View API Docs →</a>
     </div>
     <div style="border: 1px solid #ddd; border-radius: 8px; padding: 1.5rem; background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);">
       <h3 style="margin-top: 0;">🛠️ Development</h3>
       <p>Implementation details, contributing guidelines, and development setup.</p>
       <a href="development/index.html" style="font-weight: bold;">Start Contributing →</a>
     </div>
   </div>

.. toctree::
   :hidden:
   
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

   ./build.sh

Running a ROM
~~~~~~~~~~~~~

.. code-block:: bash

   ./run.sh <rom_file.gb>

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
