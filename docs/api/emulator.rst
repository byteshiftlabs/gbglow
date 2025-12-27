Emulator
========

Main emulator class that coordinates all Game Boy hardware components.

Owns and manages the CPU, Memory, PPU, APU, Timer, and Joypad. Provides
the primary interface for loading ROMs and controlling execution.

**Header:** ``core/emulator.h``

**Namespace:** ``gbcrush``


Constructor & Destructor
------------------------

.. cpp:function:: Emulator::Emulator()

   Creates a new emulator instance with all components initialized to
   power-on state (post-boot ROM values).

.. cpp:function:: Emulator::~Emulator()

   Destroys the emulator. Automatically saves battery-backed RAM if a
   cartridge with save support is loaded.


ROM Management
--------------

.. cpp:function:: bool Emulator::load_rom(const std::string& path)

   Loads a Game Boy ROM file.
   
   :param path: Path to ROM file (``.gb`` or ``.gbc``)
   :return: ``true`` on success, ``false`` on failure
   :throws std::runtime_error: Unsupported cartridge type
   :throws std::ios_base::failure: File I/O error
   
   Automatically loads the corresponding ``.sav`` file if the cartridge
   has battery-backed RAM.

.. cpp:function:: void Emulator::reset()

   Resets emulator to power-on state. Preserves loaded ROM and
   battery-backed save data.

.. cpp:function:: const std::string& Emulator::get_rom_path() const

   :return: Path to currently loaded ROM file

.. cpp:function:: std::string Emulator::get_save_path() const

   :return: Path to ``.sav`` file for current ROM


Execution
---------

.. cpp:function:: void Emulator::run_frame()

   Executes exactly one frame (70,224 M-cycles).
   
   After returning, the framebuffer contains a complete rendered frame.

.. cpp:function:: void Emulator::run_cycles(Cycles cycles)

   Executes approximately the specified number of M-cycles.
   
   :param cycles: Target number of cycles to execute
   
   Actual cycles may exceed target (instructions are atomic).

.. cpp:function:: void Emulator::run(const std::string& window_title, int scale_factor = 4)

   Main game loop with integrated display and input handling.
   
   :param window_title: Window title
   :param scale_factor: Display scale (default 4x)
   
   Runs until window is closed.


Component Access
----------------

.. cpp:function:: CPU& Emulator::cpu()
.. cpp:function:: const CPU& Emulator::cpu() const

   :return: Reference to the CPU

.. cpp:function:: PPU& Emulator::ppu()
.. cpp:function:: const PPU& Emulator::ppu() const

   :return: Reference to the PPU

.. cpp:function:: Memory& Emulator::memory()

   :return: Reference to Memory

.. cpp:function:: Joypad& Emulator::joypad()

   :return: Reference to Joypad

.. cpp:function:: Cartridge* Emulator::cartridge()

   :return: Pointer to loaded cartridge, or ``nullptr`` if none loaded


Save States
-----------

.. cpp:function:: bool Emulator::save_state(int slot)

   Saves complete emulator state to a slot.
   
   :param slot: Slot number (0-9)
   :return: ``true`` on success

.. cpp:function:: bool Emulator::load_state(int slot)

   Loads emulator state from a slot.
   
   :param slot: Slot number (0-9)
   :return: ``true`` on success

.. cpp:function:: bool Emulator::delete_state(int slot)

   Deletes a save state file.
   
   :param slot: Slot number (0-9)
   :return: ``true`` on success

.. cpp:function:: std::string Emulator::get_state_path(int slot) const

   :param slot: Slot number (0-9)
   :return: Path to state file for given slot
