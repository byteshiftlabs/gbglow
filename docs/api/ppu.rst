PPU Class
=========

Picture Processing Unit — renders the Game Boy display.

File: ``src/video/ppu.h``

Class Declaration
-----------------

.. code-block:: cpp

   class PPU {
   public:
       explicit PPU(Memory& memory);

       static constexpr int SCREEN_WIDTH = 160;
       static constexpr int SCREEN_HEIGHT = 144;

       enum class Mode { HBlank = 0, VBlank = 1, OAMSearch = 2, Transfer = 3 };

       void step(Cycles cycles);
       Mode mode() const;
       u8 scanline() const;

       void set_mode(Mode m);
       void set_scanline(u8 ly);

       bool frame_ready() const;
       void clear_frame_ready();

       const std::array<u8, SCREEN_WIDTH * SCREEN_HEIGHT>& framebuffer() const;
       std::vector<u8> get_rgba_framebuffer() const;

       // CGB palette registers
       u8 read_bcps() const;
       void write_bcps(u8 value);
       u8 read_bcpd() const;
       void write_bcpd(u8 value);
       u8 read_ocps() const;
       void write_ocps(u8 value);
       u8 read_ocpd() const;
       void write_ocpd(u8 value);

       void set_cartridge(const Cartridge* cartridge);

       void serialize(std::vector<u8>& data) const;
       void deserialize(const u8* data, size_t data_size, size_t& offset);
   };

Rendering Pipeline
------------------

Each scanline passes through four modes:

==========  ======  ========  ==========================================
Mode        Value   Cycles    Description
==========  ======  ========  ==========================================
OAMSearch   2       80        Scan OAM for sprites on current scanline
Transfer    3       172       Read VRAM and render pixels
HBlank      0       204       Horizontal blank — CPU can access VRAM
VBlank      1       4560      10-scanline vertical blank period
==========  ======  ========  ==========================================

One complete frame takes 154 scanlines (144 visible + 10 VBlank)
at 456 dots per scanline = 70224 cycles total.

Framebuffer
-----------

.. code-block:: cpp

   const std::array<u8, 160 * 144>& framebuffer() const;

Returns the grayscale framebuffer where each byte is a palette index
(0–3). Use ``get_rgba_framebuffer()`` for a ready-to-display RGBA
buffer.

CGB Color Palettes
-------------------

The PPU supports Game Boy Color palette registers:

* ``BCPS`` / ``BCPD`` (0xFF68–0xFF69) — background color palettes
* ``OCPS`` / ``OCPD`` (0xFF6A–0xFF6B) — sprite color palettes

Each palette contains four colors in 15-bit RGB format (5 bits per channel).

Serialization
-------------

.. code-block:: cpp

   void serialize(std::vector<u8>& data) const;
   void deserialize(const u8* data, size_t data_size, size_t& offset);

Saves/restores PPU state (scanline, mode, palettes, internal counters)
for save-state support.
