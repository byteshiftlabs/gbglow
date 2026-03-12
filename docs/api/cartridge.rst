Cartridge Class
===============

Abstract base class for Game Boy cartridge types.

File: ``src/cartridge/cartridge.h``

Class Declaration
-----------------

.. code-block:: cpp

   class Cartridge {
   public:
       virtual ~Cartridge() = default;

       static std::unique_ptr<Cartridge> load_rom_from_file(const std::string& path);

       virtual u8 read(u16 address) const = 0;
       virtual void write(u16 address, u8 value) = 0;

       virtual bool save_ram_to_file(const std::string& path);
       virtual bool load_ram_from_file(const std::string& path);

       const std::string& title() const;
       u8 cartridge_type() const;
       bool has_battery() const;
       bool is_cgb_supported() const;
       bool is_cgb_only() const;

       std::vector<u8> get_ram_data() const;
       void set_ram_data(const std::vector<u8>& data);
   };

Factory Method
--------------

.. code-block:: cpp

   static std::unique_ptr<Cartridge> load_rom_from_file(const std::string& path);

Reads the ROM file, inspects the cartridge header at ``0x0147``, and
returns the appropriate MBC implementation (``ROMOnly``, ``MBC1``,
``MBC3``, or ``MBC5``).

Supported Cartridge Types
-------------------------

============  ========  ============  ==================================
Type Byte     Class     RAM Banks     Features
============  ========  ============  ==================================
0x00          ROMOnly   None          32 KB ROM, no banking
0x01–0x03     MBC1      0–4           Up to 2 MB ROM, 32 KB RAM
0x0F–0x13     MBC3      0–4           RTC, battery-backed RAM
0x19–0x1E     MBC5      0–16          Up to 8 MB ROM, 128 KB RAM, rumble
============  ========  ============  ==================================

Save RAM
--------

.. code-block:: cpp

   virtual bool save_ram_to_file(const std::string& path);
   virtual bool load_ram_from_file(const std::string& path);

Writes/reads battery-backed RAM to/from a ``.sav`` file. Only
meaningful for cartridge types with battery support
(``has_battery() == true``).
