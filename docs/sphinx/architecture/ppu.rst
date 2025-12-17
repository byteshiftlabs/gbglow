PPU Architecture
================

The Picture Processing Unit (PPU) renders graphics to the LCD display.

Display Characteristics
-----------------------

Resolution
   160×144 pixels (visible area)

Colors
   * DMG: 4 shades of gray-green
   * CGB: 32,768 colors (15-bit RGB)

Refresh Rate
   59.7 Hz (~16.7ms per frame)

Total Scanlines
   154 (144 visible + 10 VBlank)

PPU Timing
----------

The PPU operates in four modes across 154 scanlines:

.. code-block:: text

   Scanline (456 dots / 114 M-cycles):
   
   ├─OAM Search─┬──Transfer───┬─────────HBlank──────────┤
   │  80 dots   │  172 dots   │      204 dots           │
   │  Mode 2    │   Mode 3    │       Mode 0            │
   └────────────┴─────────────┴─────────────────────────┘
   
   Frame timing:
   Scanlines 0-143:  Visible area (each goes through modes 2→3→0)
   Scanlines 144-153: VBlank period (Mode 1, 4560 dots = 10 lines)

PPU Modes
~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 15 15 15 55

   * - Mode
     - Value
     - Duration
     - Description
   * - HBlank
     - 0
     - 204 dots
     - Horizontal blank, free VRAM/OAM access
   * - VBlank
     - 1
     - 4560 dots
     - Vertical blank, free access, interrupts possible
   * - OAM Search
     - 2
     - 80 dots
     - Scanning OAM for sprites on current line
   * - Transfer
     - 3
     - 172 dots
     - Transferring data to LCD, VRAM/OAM locked

Rendering Pipeline
------------------

Background Layer
~~~~~~~~~~~~~~~~

The background is a 256×256 pixel scrollable area made of 8×8 pixel tiles.

**Tile Data**
   * Located at 0x8000-0x97FF (384 tiles)
   * Each tile is 16 bytes (8×8 pixels, 2 bits per pixel)
   * Two addressing modes:
     
     * Unsigned (0x8000): tiles 0-255
     * Signed (0x8800): tiles -128 to +127 (base 0x9000)

**Tile Maps**
   * Two 32×32 tile maps at 0x9800-0x9BFF and 0x9C00-0x9FFF
   * Each entry is one byte (tile number)
   * LCDC bit 3 selects which map to use

**Scrolling**
   * SCX (0xFF43): Horizontal scroll
   * SCY (0xFF42): Vertical scroll
   * Wraps around at 256 pixels

Window Layer
~~~~~~~~~~~~

The window is a non-scrollable overlay:

* Position set by WX (0xFF4B) and WY (0xFF4A)
* Uses same tile data as background
* Separate tile map (LCDC bit 6 selects)
* Can be enabled/disabled (LCDC bit 5)

Sprite Layer
~~~~~~~~~~~~

40 sprites (8×8 or 8×16 pixels):

* Stored in OAM (0xFE00-0xFE9F)
* 4 bytes per sprite: Y, X, Tile#, Attributes
* Up to 10 sprites per scanline
* Priority system determines rendering order

Palettes
--------

DMG Palettes
~~~~~~~~~~~~

Monochrome palettes map 2-bit pixel values to 4 shades:

**BGP (0xFF47)** - Background Palette

.. code-block:: text

   Bits 7-6: Color for pixel value 3
   Bits 5-4: Color for pixel value 2
   Bits 3-2: Color for pixel value 1
   Bits 1-0: Color for pixel value 0
   
   Color values:
   00 = White
   01 = Light gray
   10 = Dark gray
   11 = Black

**OBP0/OBP1 (0xFF48-0xFF49)** - Sprite Palettes

Two separate palettes for sprites (bit 4 of sprite attributes selects).

LCD Control
-----------

LCDC Register (0xFF40)
~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 10 15 75

   * - Bit
     - Name
     - Description
   * - 7
     - LCD Enable
     - 0=Off, 1=On
   * - 6
     - Win Map
     - Window tile map: 0=9800-9BFF, 1=9C00-9FFF
   * - 5
     - Win Enable
     - 0=Off, 1=On
   * - 4
     - BG/Win Tiles
     - Tile data: 0=8800-97FF, 1=8000-8FFF
   * - 3
     - BG Map
     - BG tile map: 0=9800-9BFF, 1=9C00-9FFF
   * - 2
     - Obj Size
     - Sprite size: 0=8×8, 1=8×16
   * - 1
     - Obj Enable
     - 0=Off, 1=On
   * - 0
     - BG/Win Enable
     - 0=Off, 1=On (DMG) / Priority (CGB)

STAT Register (0xFF41)
~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 10 20 70

   * - Bit
     - Name
     - Description
   * - 6
     - LYC=LY Int
     - Enable LYC=LY interrupt
   * - 5
     - Mode 2 Int
     - Enable OAM interrupt
   * - 4
     - Mode 1 Int
     - Enable VBlank interrupt
   * - 3
     - Mode 0 Int
     - Enable HBlank interrupt
   * - 2
     - LYC=LY Flag
     - 1 if LY == LYC
   * - 1-0
     - Mode Flag
     - Current PPU mode (0-3)

Implementation
--------------

PPU Class
~~~~~~~~~

.. code-block:: cpp

   class PPU {
   public:
       enum class Mode {
           HBlank = 0,
           VBlank = 1,
           OAMSearch = 2,
           Transfer = 3
       };
       
       explicit PPU(Memory& memory);
       
       void step(Cycles cycles);
       
       Mode mode() const;
       u8 scanline() const;
       bool frame_ready() const;
       void clear_frame_ready();
       
       const std::array<u8, 160 * 144>& framebuffer() const;
       void render_to_terminal() const;
       
   private:
       Memory& memory_;
       Mode mode_;
       u16 dots_;
       u8 ly_;
       bool frame_ready_;
       std::array<u8, 160 * 144> framebuffer_;
       
       void render_scanline();
       u8 get_tile_pixel(u16 tile_data_addr, u8 tile_num, 
                         u8 x, u8 y);
   };

Rendering Process
~~~~~~~~~~~~~~~~~

The PPU renders one scanline at a time during mode 3:

.. code-block:: cpp

   void PPU::render_scanline() {
       if (ly_ >= 144) return;  // Only visible scanlines
       
       u8 lcdc = memory_.read(0xFF40);
       if (!(lcdc & 0x01)) {
           // Background disabled
           fill_scanline_white();
           return;
       }
       
       u8 scy = memory_.read(0xFF42);
       u8 scx = memory_.read(0xFF43);
       
       u16 bg_map = (lcdc & 0x08) ? 0x9C00 : 0x9800;
       u16 tile_data = (lcdc & 0x10) ? 0x8000 : 0x8800;
       
       // For each pixel in scanline
       for (int x = 0; x < 160; x++) {
           u8 x_pos = x + scx;
           u8 y_pos = ly_ + scy;
           
           // Get tile from map
           u8 tile_x = x_pos / 8;
           u8 tile_y = y_pos / 8;
           u16 map_addr = bg_map + (tile_y * 32) + tile_x;
           u8 tile_num = memory_.read(map_addr);
           
           // Get pixel from tile
           u8 pixel_x = x_pos % 8;
           u8 pixel_y = y_pos % 8;
           u8 pixel = get_tile_pixel(tile_data, tile_num, 
                                     pixel_x, pixel_y);
           
           // Apply palette
           u8 palette = memory_.read(0xFF47);
           u8 color = (palette >> (pixel * 2)) & 0x03;
           
           framebuffer_[ly_ * 160 + x] = color;
       }
   }

Tile Decoding
~~~~~~~~~~~~~

Tiles are stored as 2-bit planar format:

.. code-block:: cpp

   u8 PPU::get_tile_pixel(u16 tile_data_addr, u8 tile_num,
                          u8 x, u8 y) {
       // Calculate tile address
       u16 tile_addr;
       if (tile_data_addr == 0x8000) {
           // Unsigned mode
           tile_addr = 0x8000 + (tile_num * 16);
       } else {
           // Signed mode
           i8 signed_tile = static_cast<i8>(tile_num);
           tile_addr = 0x9000 + (signed_tile * 16);
       }
       
       // Each row is 2 bytes
       tile_addr += y * 2;
       
       u8 byte1 = memory_.read(tile_addr);
       u8 byte2 = memory_.read(tile_addr + 1);
       
       // Extract 2-bit pixel
       u8 bit_pos = 7 - x;
       u8 pixel = ((byte2 >> bit_pos) & 1) << 1 | 
                  ((byte1 >> bit_pos) & 1);
       
       return pixel;
   }

Mode Transitions
~~~~~~~~~~~~~~~~

PPU advances through modes based on dot counter:

.. code-block:: cpp

   void PPU::step(Cycles cycles) {
       for (Cycles i = 0; i < cycles; i++) {
           dots_++;
           
           switch (mode_) {
               case Mode::OAMSearch:
                   if (dots_ >= 80) {
                       mode_ = Mode::Transfer;
                   }
                   break;
                   
               case Mode::Transfer:
                   if (dots_ >= 252) {
                       render_scanline();
                       mode_ = Mode::HBlank;
                   }
                   break;
                   
               case Mode::HBlank:
                   if (dots_ >= 456) {
                       dots_ = 0;
                       ly_++;
                       
                       if (ly_ >= 144) {
                           mode_ = Mode::VBlank;
                           frame_ready_ = true;
                           request_vblank_interrupt();
                       } else {
                           mode_ = Mode::OAMSearch;
                       }
                   }
                   break;
                   
               case Mode::VBlank:
                   if (dots_ >= 456) {
                       dots_ = 0;
                       ly_++;
                       
                       if (ly_ >= 154) {
                           ly_ = 0;
                           mode_ = Mode::OAMSearch;
                       }
                   }
                   break;
           }
           
           // Update LY register
           memory_.write(0xFF44, ly_);
           
           // Update STAT with current mode
           update_stat_register();
       }
   }

Terminal Output
~~~~~~~~~~~~~~~

For debugging, frames can be rendered as ASCII:

.. code-block:: cpp

   void PPU::render_to_terminal() const {
       const char shades[] = {' ', '.', '+', '#'};
       
       std::cout << "╔";
       for (int i = 0; i < 160; i++) std::cout << "═";
       std::cout << "╗\n";
       
       for (int y = 0; y < 144; y++) {
           std::cout << "║";
           for (int x = 0; x < 160; x++) {
               u8 pixel = framebuffer_[y * 160 + x];
               std::cout << shades[pixel];
           }
           std::cout << "║\n";
       }
       
       std::cout << "╚";
       for (int i = 0; i < 160; i++) std::cout << "═";
       std::cout << "╝\n";
   }

Testing
-------

PPU tests verify:

* Correct mode transitions and timing
* Scanline rendering accuracy
* Palette application
* Scroll position handling
* VRAM access during different modes

Future Enhancements
-------------------

Not Yet Implemented
~~~~~~~~~~~~~~~~~~~

* Window layer rendering
* Sprite rendering
* Sprite priority system
* STAT interrupts (HBlank, OAM, LYC=LY)
* Color support (CGB mode)
* VRAM banking (CGB mode)

Performance Notes
-----------------

* Framebuffer stored as flat array for cache efficiency
* Tile pixel lookup cached where possible
* Mode transitions checked per-cycle for accuracy
* Rendering only during Transfer mode (not every cycle)
