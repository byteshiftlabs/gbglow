PPU
===

Picture Processing Unit for graphics rendering.

Renders the 160×144 pixel display. Handles background, window, and sprite
layers with tile-based graphics.

**Header:** ``video/ppu.h``

**Namespace:** ``emugbc``


PPU Modes
---------

.. cpp:enum-class:: PPU::Mode

   PPU operating modes during each scanline.
   
   .. cpp:enumerator:: HBlank = 0
   
      Horizontal blank. CPU can access VRAM and OAM.
      Duration: 85-208 cycles (variable).
   
   .. cpp:enumerator:: VBlank = 1
   
      Vertical blank (scanlines 144-153). CPU can access VRAM and OAM.
      Duration: 4,560 cycles total.
   
   .. cpp:enumerator:: OAMSearch = 2
   
      OAM search for sprites on current scanline. OAM is locked.
      Duration: 80 cycles.
   
   .. cpp:enumerator:: Transfer = 3
   
      Pixel transfer to LCD. VRAM and OAM are locked.
      Duration: 168-291 cycles (variable).


Sprite Structure
----------------

.. cpp:struct:: PPU::Sprite

   Sprite attributes from OAM.
   
   .. cpp:member:: u8 y
   
      Y position (screen Y = y - 16)
   
   .. cpp:member:: u8 x
   
      X position (screen X = x - 8)
   
   .. cpp:member:: u8 tile
   
      Tile index in VRAM
   
   .. cpp:member:: u8 flags
   
      Attribute flags:
      
      - Bit 7: Priority (0=above BG, 1=behind BG colors 1-3)
      - Bit 6: Y flip
      - Bit 5: X flip
      - Bit 4: Palette (DMG: 0=OBP0, 1=OBP1)
      - Bit 3: VRAM bank (CGB only)
      - Bit 2-0: Palette number (CGB only)
   
   .. cpp:member:: u8 oam_index
   
      Original index in OAM (for priority sorting)


Constructor
-----------

.. cpp:function:: explicit PPU::PPU(Memory& memory)

   Creates a PPU connected to the specified memory system.
   
   :param memory: Reference to Memory instance


Execution
---------

.. cpp:function:: void PPU::step(Cycles cycles)

   Advances PPU state by the specified number of cycles.
   
   :param cycles: Number of M-cycles to advance
   
   Internally handles mode transitions, scanline rendering, and
   interrupt generation.


State Access
------------

.. cpp:function:: PPU::Mode PPU::mode() const

   :return: Current PPU mode (0-3)

.. cpp:function:: u8 PPU::scanline() const

   :return: Current scanline (LY register, 0-153)


Framebuffer Access
------------------

.. cpp:function:: bool PPU::frame_ready() const

   :return: ``true`` if a complete frame has been rendered

.. cpp:function:: void PPU::clear_frame_ready()

   Clears the frame-ready flag after consuming the frame.

.. cpp:function:: const std::array<u8, 160 * 144>& PPU::framebuffer() const

   :return: Reference to grayscale framebuffer
   
   Each pixel is a value 0-3 representing grayscale intensity.

.. cpp:function:: std::vector<u8> PPU::get_rgba_framebuffer() const

   :return: RGBA framebuffer (160×144×4 bytes)
   
   Converts the internal grayscale buffer to RGBA format for display.


CGB Palette Registers
---------------------

.. cpp:function:: u8 PPU::read_bcps() const

   Reads Background Color Palette Specification register (0xFF68).
   
   :return: BCPS value

.. cpp:function:: void PPU::write_bcps(u8 value)

   Writes to BCPS register.
   
   :param value: New BCPS value (bits 0-5: index, bit 7: auto-increment)

.. cpp:function:: u8 PPU::read_bcpd() const

   Reads Background Color Palette Data register (0xFF69).
   
   :return: Palette data at current BCPS index

.. cpp:function:: void PPU::write_bcpd(u8 value)

   Writes to BCPD register.
   
   :param value: Palette data byte

.. cpp:function:: u8 PPU::read_ocps() const

   Reads Object Color Palette Specification register (0xFF6A).
   
   :return: OCPS value

.. cpp:function:: void PPU::write_ocps(u8 value)

   Writes to OCPS register.
   
   :param value: New OCPS value

.. cpp:function:: u8 PPU::read_ocpd() const

   Reads Object Color Palette Data register (0xFF6B).
   
   :return: Palette data at current OCPS index

.. cpp:function:: void PPU::write_ocpd(u8 value)

   Writes to OCPD register.
   
   :param value: Palette data byte


Cartridge Reference
-------------------

.. cpp:function:: void PPU::set_cartridge(const Cartridge* cart)

   Sets cartridge reference for CGB mode detection.
   
   :param cart: Pointer to cartridge


Debug Output
------------

.. cpp:function:: void PPU::render_to_terminal() const

   Renders current frame to terminal as ASCII art.
   
   Useful for debugging without a graphical display.


Serialization
-------------

.. cpp:function:: void PPU::serialize(std::vector<u8>& data) const

   Serializes PPU state for save states.
   
   :param data: Vector to append serialized data to

.. cpp:function:: void PPU::deserialize(const u8* data, size_t& offset)

   Restores PPU state from serialized data.
   
   :param data: Serialized data buffer
   :param offset: Current offset (updated after reading)
