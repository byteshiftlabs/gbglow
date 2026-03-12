Memory Bank Controllers
=======================

Concrete ``Cartridge`` subclasses implementing Game Boy MBC hardware.

Files: ``src/cartridge/mbc1.h``, ``src/cartridge/mbc3.h``, ``src/cartridge/mbc5.h``

MBC1
----

.. code-block:: cpp

   class MBC1 : public Cartridge {
   public:
       explicit MBC1(std::vector<u8> rom_data, size_t ram_size);
       u8 read(u16 address) const override;
       void write(u16 address, u8 value) override;
   };

Supports up to 2 MB ROM (128 banks) and 32 KB RAM (4 banks).
Implements both 16 Mbit ROM / 8 KB RAM and 4 Mbit ROM / 32 KB RAM
banking modes.

MBC3
----

.. code-block:: cpp

   class MBC3 : public Cartridge {
   public:
       explicit MBC3(std::vector<u8> rom_data, size_t ram_size, bool has_rtc);
       u8 read(u16 address) const override;
       void write(u16 address, u8 value) override;
       bool save_ram_to_file(const std::string& path) override;
       bool load_ram_from_file(const std::string& path) override;
       void update_rtc();
   };

Adds a Real-Time Clock (RTC) with seconds, minutes, hours, and
day counter registers (``0x08``–``0x0C``). The RTC is latched by
writing ``0x00`` then ``0x01`` to the latch register.
``save_ram_to_file`` / ``load_ram_from_file`` persist both RAM and
RTC state.

MBC5
----

.. code-block:: cpp

   class MBC5 : public Cartridge {
   public:
       explicit MBC5(std::vector<u8> rom_data, size_t ram_size, bool has_rumble = false);
       u8 read(u16 address) const override;
       void write(u16 address, u8 value) override;
   };

Supports up to 8 MB ROM (9-bit bank number) and 128 KB RAM (16 banks).
Optionally includes rumble motor support — when enabled, bit 3 of the
RAM bank register controls the motor.
