Memory Class
============

The Memory Management Unit — maps the 64KB Game Boy address space.

File: ``src/core/memory.h``

Class Declaration
-----------------

.. code-block:: cpp

   class Memory {
   public:
       Memory();
       ~Memory();

       void load_cartridge(std::unique_ptr<Cartridge> cartridge);
       bool load_boot_rom(const std::string& path);

       u8 read(u16 address) const;
       void write(u16 address, u8 value);
       u16 read16(u16 address) const;
       void write16(u16 address, u16 value);

       Joypad& joypad();
       Timer& timer();
       APU& apu();
       void set_ppu(PPU* ppu);
       Cartridge* cartridge();
       const Cartridge* cartridge() const;

       void serialize(std::vector<u8>& data) const;
       void deserialize(const u8* data, size_t data_size, size_t& offset);
   };

Memory Map
----------

=================  ===========  ==========================
Address Range      Size         Description
=================  ===========  ==========================
``0x0000–0x3FFF``  16 KB        ROM Bank 0 (cartridge)
``0x4000–0x7FFF``  16 KB        Switchable ROM Bank (MBC)
``0x8000–0x9FFF``   8 KB        Video RAM (VRAM)
``0xA000–0xBFFF``   8 KB        External RAM (cartridge)
``0xC000–0xDFFF``   8 KB        Work RAM (WRAM)
``0xE000–0xFDFF``            Echo RAM (mirrors WRAM)
``0xFE00–0xFE9F``  160 B        OAM (sprite attributes)
``0xFF00–0xFF7F``            I/O Registers
``0xFF80–0xFFFE``  127 B        High RAM (HRAM)
``0xFFFF``          1 B         Interrupt Enable register
=================  ===========  ==========================

Read / Write
------------

.. code-block:: cpp

   u8 read(u16 address) const;
   void write(u16 address, u8 value);

Dispatches reads and writes to the appropriate subsystem based on
the address: cartridge, VRAM, WRAM, I/O registers, OAM, or HRAM.

Serialization
-------------

.. code-block:: cpp

   void serialize(std::vector<u8>& data) const;
   void deserialize(const u8* data, size_t data_size, size_t& offset);

Saves/restores all volatile memory regions (WRAM, VRAM, OAM, HRAM,
I/O registers) for save-state support.
