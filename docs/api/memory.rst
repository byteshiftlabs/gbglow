Memory
======

Memory Management Unit (MMU) handling the 64KB address space.

Implements the Game Boy memory map with ROM banking, I/O registers,
and memory-mapped hardware components.

**Header:** ``core/memory.h``

**Namespace:** ``gbcrush``


Memory Map
----------

::

    0x0000-0x3FFF : ROM Bank 0 (16KB)
    0x4000-0x7FFF : ROM Bank 1-N (16KB, switchable)
    0x8000-0x9FFF : Video RAM (8KB)
    0xA000-0xBFFF : External RAM (8KB, cartridge)
    0xC000-0xCFFF : Work RAM Bank 0 (4KB)
    0xD000-0xDFFF : Work RAM Bank 1 (4KB)
    0xE000-0xFDFF : Echo RAM (mirror of C000-DDFF)
    0xFE00-0xFE9F : Object Attribute Memory (OAM)
    0xFEA0-0xFEFF : Unusable
    0xFF00-0xFF7F : I/O Registers
    0xFF80-0xFFFE : High RAM (HRAM)
    0xFFFF        : Interrupt Enable Register


Constructor & Destructor
------------------------

.. cpp:function:: Memory::Memory()

   Creates memory system with all regions initialized.

.. cpp:function:: Memory::~Memory()

   Destructor.


Read/Write Operations
---------------------

.. cpp:function:: u8 Memory::read(u16 address) const

   Reads a byte from the specified address.
   
   :param address: 16-bit address (0x0000-0xFFFF)
   :return: Byte value at address
   
   Reading from I/O registers may trigger hardware behavior (e.g., reading
   JOYP updates button state).

.. cpp:function:: void Memory::write(u16 address, u8 value)

   Writes a byte to the specified address.
   
   :param address: 16-bit address (0x0000-0xFFFF)
   :param value: Byte value to write
   
   Writing to certain addresses triggers hardware behavior:
   
   - 0x0000-0x7FFF: MBC register writes (bank switching)
   - 0xFF46: OAM DMA transfer
   - 0xFF04: DIV register reset

.. cpp:function:: u16 Memory::read16(u16 address) const

   Reads a 16-bit value (little-endian).
   
   :param address: Starting address
   :return: 16-bit value (low byte at address, high byte at address+1)

.. cpp:function:: void Memory::write16(u16 address, u16 value)

   Writes a 16-bit value (little-endian).
   
   :param address: Starting address
   :param value: 16-bit value to write


Cartridge Management
--------------------

.. cpp:function:: void Memory::load_cartridge(std::unique_ptr<Cartridge> cart)

   Loads a cartridge into the memory system.
   
   :param cart: Cartridge to load (ownership transferred)

.. cpp:function:: Cartridge* Memory::cartridge()

   :return: Pointer to loaded cartridge, or ``nullptr``


Boot ROM
--------

.. cpp:function:: bool Memory::load_boot_rom(const std::string& path)

   Loads the 256-byte boot ROM.
   
   :param path: Path to boot ROM file
   :return: ``true`` on success
   
   When loaded, the boot ROM is mapped to 0x0000-0x00FF until disabled
   by writing to 0xFF50.


Component Access
----------------

.. cpp:function:: Joypad& Memory::joypad()

   :return: Reference to joypad handler

.. cpp:function:: Timer& Memory::timer()

   :return: Reference to timer system

.. cpp:function:: APU& Memory::apu()

   :return: Reference to audio processing unit

.. cpp:function:: void Memory::set_ppu(PPU* ppu)

   Sets the PPU reference for VRAM/OAM access control.
   
   :param ppu: Pointer to PPU instance


Serialization
-------------

.. cpp:function:: void Memory::serialize(std::vector<u8>& data) const

   Serializes memory state for save states.
   
   :param data: Vector to append serialized data to

.. cpp:function:: void Memory::deserialize(const u8* data, size_t& offset)

   Restores memory state from serialized data.
   
   :param data: Serialized data buffer
   :param offset: Current offset (updated after reading)
