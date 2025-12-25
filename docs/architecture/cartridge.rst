Cartridge System
================

Game Boy cartridges contain ROM, optional RAM, and Memory Bank Controllers (MBCs).

Cartridge Header
----------------

Every cartridge has a header at 0x0100-0x014F:

.. list-table::
   :header-rows: 1
   :widths: 15 15 70

   * - Address
     - Size
     - Description
   * - 0x0100-0x0103
     - 4 bytes
     - Entry point (usually NOP; JP 0x0150)
   * - 0x0104-0x0133
     - 48 bytes
     - Nintendo logo (must match for boot ROM)
   * - 0x0134-0x0143
     - 16 bytes
     - Game title (ASCII, null-padded)
   * - 0x0143
     - 1 byte
     - CGB flag (0x80=CGB, 0xC0=CGB only)
   * - 0x0144-0x0145
     - 2 bytes
     - New licensee code
   * - 0x0146
     - 1 byte
     - SGB flag (0x03=SGB functions)
   * - 0x0147
     - 1 byte
     - Cartridge type
   * - 0x0148
     - 1 byte
     - ROM size
   * - 0x0149
     - 1 byte
     - RAM size
   * - 0x014A
     - 1 byte
     - Destination code (0=Japan, 1=Overseas)
   * - 0x014B
     - 1 byte
     - Old licensee code
   * - 0x014C
     - 1 byte
     - ROM version
   * - 0x014D
     - 1 byte
     - Header checksum
   * - 0x014E-0x014F
     - 2 bytes
     - Global checksum

Cartridge Types
---------------

Common cartridge types (0x0147):

.. list-table::
   :header-rows: 1
   :widths: 10 30 60

   * - Code
     - Name
     - Features
   * - 0x00
     - ROM ONLY
     - 32KB ROM, no MBC
   * - 0x01
     - MBC1
     - ROM banking
   * - 0x02
     - MBC1+RAM
     - ROM + RAM banking
   * - 0x03
     - MBC1+RAM+BATTERY
     - ROM + battery-backed RAM
   * - 0x05
     - MBC2
     - ROM + built-in 512×4bit RAM
   * - 0x06
     - MBC2+BATTERY
     - MBC2 with battery
   * - 0x0F
     - MBC3+TIMER+BATTERY
     - MBC3 with RTC
   * - 0x10
     - MBC3+TIMER+RAM+BATTERY
     - MBC3 with RTC and RAM
   * - 0x11
     - MBC3
     - ROM banking
   * - 0x12
     - MBC3+RAM
     - ROM + RAM banking
   * - 0x13
     - MBC3+RAM+BATTERY
     - ROM + battery-backed RAM
   * - 0x19
     - MBC5
     - Enhanced banking
   * - 0x1A
     - MBC5+RAM
     - MBC5 with RAM
   * - 0x1B
     - MBC5+RAM+BATTERY
     - MBC5 with battery RAM
   * - 0x1C
     - MBC5+RUMBLE
     - MBC5 with rumble motor
   * - 0x1D
     - MBC5+RUMBLE+RAM
     - MBC5 with rumble and RAM
   * - 0x1E
     - MBC5+RUMBLE+RAM+BATTERY
     - MBC5 with all features

ROM Sizes
~~~~~~~~~

ROM size byte (0x0148):

.. code-block:: text

   0x00:  32KB (2 banks, no banking)
   0x01:  64KB (4 banks)
   0x02: 128KB (8 banks)
   0x03: 256KB (16 banks)
   0x04: 512KB (32 banks)
   0x05:   1MB (64 banks)
   0x06:   2MB (128 banks)
   0x07:   4MB (256 banks)
   0x08:   8MB (512 banks)

RAM Sizes
~~~~~~~~~

RAM size byte (0x0149):

.. code-block:: text

   0x00: None
   0x01: Unused
   0x02:  8KB (1 bank)
   0x03: 32KB (4 banks of 8KB)
   0x04: 128KB (16 banks of 8KB)
   0x05: 64KB (8 banks of 8KB)

Memory Bank Controllers
-----------------------

ROM-Only Cartridges
~~~~~~~~~~~~~~~~~~~

Simplest cartridge type:

* 32KB ROM maximum
* No banking
* No RAM

Implementation:

.. code-block:: cpp

   class ROMOnly : public Cartridge {
   public:
       u8 read(u16 address) const override {
           if (address < 0x8000 && address < rom_.size()) {
               return rom_[address];
           }
           return 0xFF;
       }
       
       void write(u16 address, u8 value) override {
           // ROM-only cartridges ignore writes
       }
   };

MBC1 - Memory Bank Controller 1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Most common MBC, used in early games.

**Features**
   * Up to 2MB ROM (125 banks of 16KB)
   * Up to 32KB RAM (4 banks of 8KB)
   * Two banking modes

**Memory Map**

.. code-block:: text

   0x0000-0x1FFF: RAM Enable
   0x2000-0x3FFF: ROM Bank Number (lower 5 bits)
   0x4000-0x5FFF: RAM Bank / Upper ROM bits
   0x6000-0x7FFF: Banking Mode Select

**Banking Modes**
   * Mode 0 (ROM mode): Full ROM space, single RAM bank
   * Mode 1 (RAM mode): Limited ROM, all RAM banks

Implementation:

.. code-block:: cpp

   class MBC1 : public Cartridge {
   private:
       bool ram_enabled_;
       u8 rom_bank_;       // 5-bit (0x01-0x1F)
       u8 ram_bank_;       // 2-bit (0x00-0x03)
       bool banking_mode_; // false=ROM, true=RAM
       
   public:
       u8 read(u16 address) const override {
           if (address < 0x4000) {
               // Bank 0 (or 0x00/0x20/0x40/0x60 in RAM mode)
               size_t bank = banking_mode_ ? (ram_bank_ << 5) : 0;
               return rom_[bank * 0x4000 + address];
           } else if (address < 0x8000) {
               // Switchable bank
               size_t bank = rom_bank_ | (ram_bank_ << 5);
               return rom_[bank * 0x4000 + (address - 0x4000)];
           } else if (address >= 0xA000 && address < 0xC000) {
               // External RAM
               if (!ram_enabled_) return 0xFF;
               size_t bank = banking_mode_ ? ram_bank_ : 0;
               return ram_[bank * 0x2000 + (address - 0xA000)];
           }
           return 0xFF;
       }
       
       void write(u16 address, u8 value) override {
           if (address < 0x2000) {
               // RAM enable
               ram_enabled_ = (value & 0x0F) == 0x0A;
           } else if (address < 0x4000) {
               // ROM bank (lower 5 bits)
               rom_bank_ = value & 0x1F;
               if (rom_bank_ == 0) rom_bank_ = 1;
           } else if (address < 0x6000) {
               // RAM bank / upper ROM bits
               ram_bank_ = value & 0x03;
           } else if (address < 0x8000) {
               // Banking mode
               banking_mode_ = (value & 0x01) != 0;
           } else if (address >= 0xA000 && address < 0xC000) {
               // RAM write
               if (!ram_enabled_) return;
               size_t bank = banking_mode_ ? ram_bank_ : 0;
               ram_[bank * 0x2000 + (address - 0xA000)] = value;
           }
       }
   };

MBC3 - Memory Bank Controller 3
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Enhanced MBC with optional Real-Time Clock (RTC).

**Features**
   * Up to 2MB ROM (128 banks)
   * Up to 32KB RAM (4 banks)
   * Real-Time Clock (optional)

**RTC Registers**
   * 0x08: Seconds (0-59)
   * 0x09: Minutes (0-59)
   * 0x0A: Hours (0-23)
   * 0x0B: Days (lower 8 bits)
   * 0x0C: Days (upper 1 bit) + Halt + Carry

**Note**: RTC not yet implemented in GBCrush.

MBC5 - Memory Bank Controller 5
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Most advanced MBC, used in later games.

**Features**
   * Up to 8MB ROM (512 banks)
   * Up to 128KB RAM (16 banks)
   * Optional rumble motor

**Memory Map**

.. code-block:: text

   0x0000-0x1FFF: RAM Enable
   0x2000-0x2FFF: ROM Bank (lower 8 bits)
   0x3000-0x3FFF: ROM Bank (9th bit)
   0x4000-0x5FFF: RAM Bank

**Note**: MBC5 not yet implemented in GBCrush.

Implementation
--------------

Cartridge Base Class
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class Cartridge {
   public:
       virtual ~Cartridge() = default;
       
       static std::unique_ptr<Cartridge> load_from_file(
           const std::string& path);
       
       virtual u8 read(u16 address) const = 0;
       virtual void write(u16 address, u8 value) = 0;
       
       const std::string& title() const;
       u8 cartridge_type() const;
       bool has_battery() const;
       
   protected:
       std::vector<u8> rom_;
       std::vector<u8> ram_;
       std::string title_;
       u8 cartridge_type_;
       bool has_battery_;
       
       void parse_header();
   };

Cartridge Loading
~~~~~~~~~~~~~~~~~

Factory method creates appropriate cartridge type:

.. code-block:: cpp

   std::unique_ptr<Cartridge> Cartridge::load_from_file(
       const std::string& path) {
       
       // Read ROM file
       std::vector<u8> rom_data = read_file(path);
       
       // Check cartridge type
       u8 type = rom_data[0x0147];
       
       switch (type) {
           case 0x00:
               return std::make_unique<ROMOnly>(rom_data);
           case 0x01:
           case 0x02:
           case 0x03:
               return std::make_unique<MBC1>(rom_data);
           case 0x0F:
           case 0x10:
           case 0x11:
           case 0x12:
           case 0x13:
               return std::make_unique<MBC3>(rom_data);
           default:
               throw std::runtime_error("Unsupported cartridge type");
       }
   }

Battery-Backed RAM
~~~~~~~~~~~~~~~~~~

Cartridges with battery save RAM to disk:

.. code-block:: cpp

   class MBC1 : public Cartridge {
   public:
       ~MBC1() {
           if (has_battery_) {
               save_ram_to_disk();
           }
       }
       
   private:
       void save_ram_to_disk() {
           std::string save_file = title_ + ".sav";
           std::ofstream file(save_file, std::ios::binary);
           file.write(reinterpret_cast<char*>(ram_.data()), 
                     ram_.size());
       }
       
       void load_ram_from_disk() {
           std::string save_file = title_ + ".sav";
           std::ifstream file(save_file, std::ios::binary);
           if (file) {
               file.read(reinterpret_cast<char*>(ram_.data()), 
                        ram_.size());
           }
       }
   };

Testing
-------

Cartridge tests verify:

* Header parsing
* ROM bank switching
* RAM bank switching
* Banking mode behavior
* Battery save/load

Future Enhancements
-------------------

Not Yet Implemented
~~~~~~~~~~~~~~~~~~~

* MBC2 (built-in RAM)
* MBC3 (RTC support)
* MBC5 (9-bit banking)
* MBC6 (flash memory)
* MBC7 (accelerometer)
* HuC1/HuC3 (IR communication)
* MMM01 (multi-game)
* Camera cartridge
