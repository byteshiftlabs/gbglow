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

**OAM Structure**

Each sprite occupies 4 bytes in OAM:

.. list-table::
   :header-rows: 1
   :widths: 15 15 70

   * - Byte
     - Name
     - Description
   * - 0
     - Y Position
     - Y coordinate + 16 (0-255)
   * - 1
     - X Position
     - X coordinate + 8 (0-255)
   * - 2
     - Tile Number
     - Index into sprite tile data (8x16: bit 0 ignored)
   * - 3
     - Attributes
     - Flags (priority, flip, palette)

**Sprite Attributes (Byte 3)**

.. list-table::
   :header-rows: 1
   :widths: 10 20 70

   * - Bit
     - Name
     - Description
   * - 7
     - Priority
     - 0=Above BG, 1=Behind BG colors 1-3
   * - 6
     - Y-Flip
     - 0=Normal, 1=Vertically mirrored
   * - 5
     - X-Flip
     - 0=Normal, 1=Horizontally mirrored
   * - 4
     - Palette
     - 0=OBP0, 1=OBP1
   * - 3-0
     - Unused
     - DMG: unused, CGB: palette number

**OAM DMA Transfer**

Games use DMA (Direct Memory Access) to quickly copy sprite data to OAM:

.. code-block:: text

   Register: 0xFF46 (DMA)
   
   Writing value XX triggers:
   - Source address: XX00 (e.g., 0xC3 → 0xC300)
   - Copies 160 bytes to OAM (0xFE00-0xFE9F)
   - Takes 160 machine cycles on real hardware
   
   Common usage:
   ld a, $C3      ; High byte of source
   ldh ($46), a   ; Trigger DMA

**Coordinate System**

Sprites use an offset coordinate system to allow partial off-screen positioning:

.. code-block:: text

   Screen position (0,0) = Sprite position (8, 16)
   
   X coordinate: sprite.x = screen_x + 8
   Y coordinate: sprite.y = screen_y + 16
   
   Valid ranges:
   - X: 0-168 (sprite.x: 8-176)
   - Y: 0-160 (sprite.y: 16-176)
   
   Fully off-screen:
   - Left: X < 8
   - Right: X >= 168
   - Top: Y < 16
   - Bottom: Y >= 160

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
       
       struct Sprite {
           u8 y;           // Y position + 16
           u8 x;           // X position + 8
           u8 tile;        // Tile index
           u8 flags;       // Attributes
           u8 oam_index;   // Original OAM index (for priority)
       };
       
       explicit PPU(Memory& memory);
       
       void step(Cycles cycles);
       
       Mode mode() const;
       u8 scanline() const;
       bool frame_ready() const;
       void clear_frame_ready();
       
       const std::array<u8, 160 * 144>& framebuffer() const;
       std::vector<u8> get_rgba_framebuffer() const;
       
   private:
       Memory& memory_;
       Mode mode_;
       u16 dots_;
       u8 ly_;
       bool frame_ready_;
       u8 window_line_counter_;
       std::array<u8, 160 * 144> framebuffer_;
       std::vector<Sprite> scanline_sprites_;
       
       void render_scanline();
       void render_background();
       void render_window();
       void render_sprites();
       
       void search_oam();
       
       u8 get_tile_pixel(u16 tile_data_addr, u8 tile_num, 
                         u8 x, u8 y) const;
       u8 get_sprite_pixel(u8 tile_num, u8 sprite_flags,
                          u8 pixel_x, u8 pixel_y) const;
       bool is_sprite_priority(u8 sprite_flags, u8 bg_color) const;
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
                       search_oam();  // Find sprites for this scanline
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
                           window_line_counter_ = 0;
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

Sprite Rendering Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**OAM Search Phase**

During mode 2, PPU searches all 40 sprites to find up to 10 visible on current scanline:

.. code-block:: cpp

   void PPU::search_oam() {
       scanline_sprites_.clear();
       
       u8 lcdc = memory_.read(0xFF40);
       if (!(lcdc & 0x02)) return;  // Sprites disabled
       
       u8 sprite_height = (lcdc & 0x04) ? 16 : 8;
       
       // Check all 40 sprites in OAM
       for (u16 i = 0; i < 40; i++) {
           u16 oam_addr = 0xFE00 + (i * 4);
           
           Sprite sprite;
           sprite.y = memory_.read(oam_addr + 0);
           sprite.x = memory_.read(oam_addr + 1);
           sprite.tile = memory_.read(oam_addr + 2);
           sprite.flags = memory_.read(oam_addr + 3);
           sprite.oam_index = i;
           
           // Visibility check (hardware logic)
           // Uses raw Y value (not screen-adjusted)
           if (ly_ >= sprite.y || ly_ + 16 < sprite.y) {
               continue;
           }
           
           // 8x8 mode: additional check
           if (sprite_height == 8 && ly_ + 8 >= sprite.y) {
               continue;
           }
           
           scanline_sprites_.push_back(sprite);
           
           // Hardware limit: 10 sprites per scanline
           if (scanline_sprites_.size() >= 10) {
               break;
           }
       }
   }

**Why This Visibility Logic?**

The visibility check ``ly_ >= sprite.y || ly_ + 16 < sprite.y`` works because:

1. Sprites use Y+16 coordinate system
2. Check if scanline is within sprite's vertical range
3. First condition: scanline hasn't reached sprite yet
4. Second condition: scanline has passed sprite

**Sprite Rendering Phase**

During mode 3, render found sprites over background:

.. code-block:: cpp

   void PPU::render_sprites() {
       if (scanline_sprites_.empty()) return;
       
       u8 lcdc = memory_.read(0xFF40);
       u8 sprite_height = (lcdc & 0x04) ? 16 : 8;
       
       // Render in reverse order (lower OAM index = higher priority)
       for (auto it = scanline_sprites_.rbegin(); 
            it != scanline_sprites_.rend(); ++it) {
           const Sprite& sprite = *it;
           
           // Calculate row within sprite
           int sprite_row = ly_ - static_cast<int>(sprite.y) + 16;
           int screen_x = static_cast<int>(sprite.x) - 8;
           
           u8 tile_num = sprite.tile;
           
           // Handle 8x16 mode
           if (sprite_height == 16) {
               tile_num &= 0xFE;  // Clear bit 0
               if (sprite_row >= 8) {
                   sprite_row -= 8;
                   tile_num++;
               }
               // Y-flip swaps tiles in 8x16 mode
               if (sprite.flags & 0x40) {
                   tile_num ^= 1;
               }
           }
           
           // Apply Y-flip to row
           if (sprite.flags & 0x40) {
               sprite_row = 7 - sprite_row;
           }
           
           // Render each pixel
           for (u8 pixel_x = 0; pixel_x < 8; pixel_x++) {
               int draw_x = screen_x + pixel_x;
               if (draw_x < 0 || draw_x >= 160) continue;
               
               u8 sprite_pixel = get_sprite_pixel(
                   tile_num, sprite.flags, pixel_x, sprite_row);
               
               // Color 0 is transparent
               if (sprite_pixel == 0) continue;
               
               // Check priority against background
               u8 bg_color = framebuffer_[ly_ * 160 + draw_x];
               if (is_sprite_priority(sprite.flags, bg_color)) {
                   // Apply sprite palette
                   u16 palette_reg = (sprite.flags & 0x10) ? 
                                     0xFF49 : 0xFF48;
                   u8 palette = memory_.read(palette_reg);
                   u8 color = (palette >> (sprite_pixel * 2)) & 0x03;
                   
                   framebuffer_[ly_ * 160 + draw_x] = color;
               }
           }
       }
   }

**8x16 Sprite Tile Selection**

In 8x16 mode:

* Bit 0 of tile number is ignored (always uses even tile)
* Top 8 pixels: use ``tile & 0xFE``
* Bottom 8 pixels: use ``(tile & 0xFE) + 1``
* Y-flip: XOR final tile number with 1 (swaps top/bottom)

**Priority System**

.. code-block:: cpp

   bool PPU::is_sprite_priority(u8 sprite_flags, u8 bg_color) const {
       // Priority bit 7: 0=above BG, 1=behind BG colors 1-3
       bool behind_bg = (sprite_flags & 0x80) != 0;
       
       if (!behind_bg) {
           return true;  // Always above background
       }
       
       // Behind BG colors 1-3, visible only if BG is color 0
       return bg_color == 0;
   }

**Sprite Pixel Extraction**

.. code-block:: cpp

   u8 PPU::get_sprite_pixel(u8 tile_num, u8 sprite_flags,
                            u8 pixel_x, u8 pixel_y) const {
       // Apply X-flip
       if (sprite_flags & 0x20) {
           pixel_x = 7 - pixel_x;
       }
       
       // Sprites always use 0x8000 tile data
       u16 tile_addr = 0x8000 + (tile_num * 16) + (pixel_y * 2);
       
       u8 byte1 = memory_.read(tile_addr);
       u8 byte2 = memory_.read(tile_addr + 1);
       
       // Extract 2-bit pixel
       u8 bit_pos = 7 - pixel_x;
       u8 pixel = ((byte2 >> bit_pos) & 1) << 1 | 
                  ((byte1 >> bit_pos) & 1);
       
       return pixel;
   }

Testing
-------

PPU tests verify:

* Correct mode transitions and timing
* Scanline rendering accuracy
* Background and window rendering
* Sprite visibility logic
* Sprite 8x16 mode tile selection
* Sprite priority system
* Palette application
* Scroll position handling
* VRAM access during different modes
* OAM DMA transfer functionality

Test ROMs
~~~~~~~~~

**Pokémon Red/Blue**
   * Tests OAM DMA, sprite rendering, 8x16 sprites
   * Game Freak logo uses sprites
   * Battle scenes test sprite positioning

**Test ROM Suite**
   * blargg's test ROMs for PPU timing
   * Sprite priority tests
   * OAM timing tests

Future Enhancements
-------------------

Not Yet Implemented
~~~~~~~~~~~~~~~~~~~

* Color support (CGB mode)
* VRAM banking (CGB mode)
* Cycle-accurate OAM DMA delay (currently instant)
* Sprite rendering edge cases (X=0 wrapping)

Implemented Features
~~~~~~~~~~~~~~~~~~~~

* ✅ Background layer rendering
* ✅ Window layer rendering
* ✅ Sprite rendering (8x8 and 8x16)
* ✅ Sprite priority system
* ✅ OAM DMA transfer
* ✅ Hardware-accurate sprite visibility
* ✅ Sprite flip (X and Y)
* ✅ Proper 8x16 tile selection
* ✅ STAT interrupts (HBlank, OAM, LYC=LY) with rising-edge deduplication
* ✅ LCD enable/disable gating (mode reset on re-enable)
* ✅ Window line counter (internal counter, serialized in save states)
* ✅ PPU state serialization (scanline, mode, dots, palettes, window counter)

