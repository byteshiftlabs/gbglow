#pragma once

#include "../core/types.h"
#include <array>
#include <cstdint>
#include <vector>

namespace emugbc {

class Memory;

/**
 * Picture Processing Unit (PPU)
 * 
 * The Game Boy PPU operates at 4.194 MHz (same as CPU)
 * One scanline takes 456 dots (114 M-cycles)
 * Screen resolution: 160x144 pixels
 * 
 * PPU Modes:
 * - Mode 2 (OAM Search): 80 dots
 * - Mode 3 (Transfer): 172 dots
 * - Mode 0 (HBlank): 204 dots
 * - Mode 1 (VBlank): 4560 dots (10 scanlines)
 */
class PPU {
public:
    enum class Mode {
        HBlank = 0,      // Horizontal blank
        VBlank = 1,      // Vertical blank
        OAMSearch = 2,   // Searching OAM for sprites
        Transfer = 3     // Transferring data to LCD
    };
    
    /**
     * Sprite object from OAM (Object Attribute Memory)
     * Each sprite is 4 bytes in OAM (0xFE00-0xFE9F)
     */
    struct Sprite {
        u8 y;           // Y position + 16
        u8 x;           // X position + 8
        u8 tile;        // Tile index
        u8 flags;       // Attributes
        u8 oam_index;   // Original OAM index (for priority)
    };
    
    explicit PPU(Memory& memory);
    
    // Execute PPU for given number of cycles
    void step(Cycles cycles);
    
    // Accessors
    Mode mode() const;
    u8 scanline() const;
    
    // Frame buffer access
    bool frame_ready() const;
    void clear_frame_ready();
    
    // Get framebuffer (160x144, grayscale 0-3)
    const std::array<u8, 160 * 144>& framebuffer() const;
    
    // Render current frame to terminal as ASCII
    void render_to_terminal() const;
    
private:
    Memory& memory_;
    
    Mode mode_;
    u16 dots_;           // Dot counter within current scanline
    u8 ly_;              // Current scanline (LY register)
    bool frame_ready_;
    
    // Framebuffer: 160x144 pixels, each pixel is 0-3 (grayscale)
    std::array<u8, 160 * 144> framebuffer_;
    
    // Sprites visible on current scanline
    std::vector<Sprite> scanline_sprites_;
    
    // OAM constants
    static constexpr u16 OAM_START = 0xFE00;
    static constexpr u16 OAM_END = 0xFEA0;
    static constexpr u8 OAM_ENTRY_SIZE = 4;
    static constexpr u8 MAX_SPRITES_PER_SCANLINE = 10;
    static constexpr u8 SPRITE_HEIGHT_8X8 = 8;
    static constexpr u8 SPRITE_HEIGHT_8X16 = 16;
    
    // Sprite attribute flags (bit positions)
    static constexpr u8 SPRITE_FLAG_PRIORITY_BIT = 7;      // 0=above BG, 1=behind BG colors 1-3
    static constexpr u8 SPRITE_FLAG_Y_FLIP_BIT = 6;        // Flip vertically
    static constexpr u8 SPRITE_FLAG_X_FLIP_BIT = 5;        // Flip horizontally
    static constexpr u8 SPRITE_FLAG_PALETTE_BIT = 4;       // 0=OBP0, 1=OBP1
    
    // Sprite position offsets
    static constexpr u8 SPRITE_Y_OFFSET = 16;
    static constexpr u8 SPRITE_X_OFFSET = 8;
    
    // Transparent color
    static constexpr u8 TRANSPARENT_COLOR = 0;
    
    // Background rendering
    void render_scanline();
    void render_background();
    void render_sprites();
    
    // OAM search
    void search_oam();
    
    // Sprite helpers
    u8 get_sprite_pixel(const Sprite& sprite, u8 screen_x, u8 screen_y) const;
    bool is_sprite_priority(u8 sprite_flags, u8 bg_color) const;
    
    // Tile data
    u8 get_tile_pixel(u16 tile_data_addr, u8 tile_num, u8 x, u8 y) const;
};

} // namespace emugbc
