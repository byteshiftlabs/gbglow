#include "ppu.h"

#include <iostream>
#include <algorithm>

#include "../core/memory.h"

namespace emugbc {

// Hardware constants
namespace {
    constexpr int SCREEN_WIDTH = 160;
    constexpr int SCREEN_HEIGHT = 144;
    constexpr int TILE_SIZE = 8;
    constexpr int TILEMAP_WIDTH = 32;  // Background map is 32x32 tiles
    constexpr int BYTES_PER_TILE = 16; // Each tile is 16 bytes (8x8 pixels, 2bpp)
    constexpr int BITS_PER_PIXEL = 2;
    constexpr int BYTES_PER_TILE_ROW = 2;
    
    // LCDC register bits
    constexpr u8 LCDC_BG_ENABLE_BIT = 0x01;
    constexpr u8 LCDC_OBJ_ENABLE_BIT = 0x02;
    constexpr u8 LCDC_OBJ_SIZE_BIT = 0x04;
    constexpr u8 LCDC_BG_TILEMAP_BIT = 0x08;
    constexpr u8 LCDC_TILE_DATA_BIT = 0x10;
    constexpr u8 LCDC_WINDOW_ENABLE_BIT = 0x20;
    constexpr u8 LCDC_WINDOW_TILEMAP_BIT = 0x40;
    constexpr u8 LCDC_LCD_ENABLE_BIT = 0x80;
    
    // Register addresses
    constexpr u16 REG_LCDC = 0xFF40;
    constexpr u16 REG_STAT = 0xFF41;
    constexpr u16 REG_SCY = 0xFF42;
    constexpr u16 REG_SCX = 0xFF43;
    constexpr u16 REG_LY = 0xFF44;
    constexpr u16 REG_BGP = 0xFF47;
    constexpr u16 REG_OBP0 = 0xFF48;
    constexpr u16 REG_OBP1 = 0xFF49;
    constexpr u16 REG_WY = 0xFF4A;
    constexpr u16 REG_WX = 0xFF4B;
    constexpr u16 REG_IF = 0xFF0F;
    
    // Palette manipulation
    constexpr u8 PALETTE_MASK = 0x03;
    
    // VBlank interrupt bit
    constexpr u8 VBLANK_INT_BIT = 0x01;
    
    // STAT mode mask
    constexpr u8 STAT_MODE_MASK = 0xFC;
    
    // Tile addressing
    constexpr u16 TILE_DATA_SIGNED_BASE = 0x9000;
    constexpr u16 TILE_DATA_UNSIGNED_BASE = 0x8000;
    constexpr u16 TILE_MAP_0 = 0x9800;
    constexpr u16 TILE_MAP_1 = 0x9C00;
    
    // OAM sprite constants
    constexpr u8 OAM_SPRITE_COUNT = 40;
    constexpr u8 OAM_Y_OFFSET = 0;
    constexpr u8 OAM_X_OFFSET = 1;
    constexpr u8 OAM_TILE_OFFSET = 2;
    constexpr u8 OAM_FLAGS_OFFSET = 3;
    
    // Sprite 8x16 mode constants
    constexpr u8 SPRITE_8X16_TILE_MASK = 0xFE;
    constexpr u8 SPRITE_8X16_BOTTOM_TILE_BIT = 0x01;
    constexpr u8 SPRITE_8X16_ROW_THRESHOLD = 8;
    
    // Sprite visibility constants (gnuboy compatibility)
    constexpr u8 SPRITE_Y_VISIBILITY_OFFSET = 16;  // Sprites use Y+16 coordinate system
    constexpr u8 SPRITE_8X8_HEIGHT_CHECK = 8;      // For 8x8 sprite visibility
    constexpr u8 SPRITE_ROW_MASK = 7;              // Mask for pixel row within 8-pixel tile (0-7)
    
    // Bit manipulation constants
    constexpr u8 BIT_0 = 0;
    constexpr u8 BIT_1 = 1;
    constexpr u8 PIXEL_BIT_SHIFT = 1;
    constexpr u8 PIXEL_MSB_BIT = 7;
}

PPU::PPU(Memory& memory) 
    : memory_(memory)
    , mode_(Mode::OAMSearch)
    , dots_(0)
    , ly_(0)
    , frame_ready_(false)
    , window_line_counter_(0) {
    framebuffer_.fill(0);
}

void PPU::step(Cycles cycles) {
    // Each cycle advances the PPU by 1 dot
    for (Cycles i = 0; i < cycles; i++) {
        dots_++;
        
        switch (mode_) {
            case Mode::OAMSearch:
                if (dots_ >= 80) {
                    // Search OAM for sprites on this scanline
                    search_oam();
                    mode_ = Mode::Transfer;
                }
                break;
                
            case Mode::Transfer:
                if (dots_ >= 252) {  // 80 + 172
                    // Render this scanline
                    render_scanline();
                    mode_ = Mode::HBlank;
                }
                break;
                
            case Mode::HBlank:
                if (dots_ >= 456) {
                    dots_ = 0;
                    ly_++;
                    
                    if (ly_ >= 144) {
                        // Enter VBlank
                        mode_ = Mode::VBlank;
                        frame_ready_ = true;
                        
                        // Set VBlank flag in IF register
                        u8 if_reg = memory_.read(REG_IF);
                        memory_.write(REG_IF, if_reg | VBLANK_INT_BIT);
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
                        // End of VBlank, restart frame
                        ly_ = 0;
                        window_line_counter_ = 0;  // Reset window counter for new frame
                        mode_ = Mode::OAMSearch;
                    }
                }
                break;
        }
        
        // Update LY register
        memory_.write(REG_LY, ly_);
        
        // Update STAT register with current mode
        u8 stat = memory_.read(REG_STAT);
        stat = (stat & STAT_MODE_MASK) | static_cast<u8>(mode_);
        memory_.write(REG_STAT, stat);
    }
}

void PPU::render_scanline() {
    if (ly_ >= SCREEN_HEIGHT) return;  // Only render visible scanlines
    
    // Render background first, then window, then sprites on top
    render_background();
    render_window();
    render_sprites();
}

void PPU::render_background() {
    // Read LCD control register
    u8 lcdc = memory_.read(REG_LCDC);
    bool bg_enabled = (lcdc & LCDC_BG_ENABLE_BIT) != 0;
    
    if (!bg_enabled) {
        // Background disabled, fill with white
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            framebuffer_[ly_ * SCREEN_WIDTH + x] = 0;
        }
        return;
    }
    
    // Hardware scroll registers allow background to pan
    u8 scy = memory_.read(REG_SCY);
    u8 scx = memory_.read(REG_SCX);
    
    // LCDC bit 3 selects which of two tile maps to use
    u16 bg_map = (lcdc & LCDC_BG_TILEMAP_BIT) ? TILE_MAP_1 : TILE_MAP_0;
    
    // LCDC bit 4 selects tile data addressing mode (signed vs unsigned)
    u16 tile_data = (lcdc & LCDC_TILE_DATA_BIT) ? TILE_DATA_UNSIGNED_BASE : TILE_DATA_SIGNED_BASE;
    
    // Background wraps at 256x256 pixels using modulo arithmetic
    u8 y_pos = ly_ + scy;
    u8 tile_y = y_pos / TILE_SIZE;
    u8 pixel_y = y_pos % TILE_SIZE;
    
    // Render each pixel in the scanline
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        u8 x_pos = x + scx;
        u8 tile_x = x_pos / TILE_SIZE;
        u8 pixel_x = x_pos % TILE_SIZE;
        
        // Tile map is 32x32 tiles, each byte is a tile index
        u16 map_addr = bg_map + (tile_y * TILEMAP_WIDTH) + tile_x;
        u8 tile_num = memory_.read(map_addr);
        
        // Decode 2-bit pixel from tile pattern data
        u8 pixel = get_tile_pixel(tile_data, tile_num, pixel_x, pixel_y);
        
        // Background palette maps 2-bit color to 2-bit shade
        u8 palette = memory_.read(REG_BGP);
        u8 color = (palette >> (pixel * BITS_PER_PIXEL)) & PALETTE_MASK;
        
        framebuffer_[ly_ * SCREEN_WIDTH + x] = color;
    }
}

void PPU::render_window() {
    // Read LCD control register
    u8 lcdc = memory_.read(REG_LCDC);
    bool window_enabled = (lcdc & LCDC_WINDOW_ENABLE_BIT) != 0;
    bool bg_enabled = (lcdc & LCDC_BG_ENABLE_BIT) != 0;  // Window requires BG enable on DMG
    
    if (!window_enabled || !bg_enabled) {
        return;
    }
    
    // Window position registers
    u8 wy = memory_.read(REG_WY);  // Window Y position (0-143)
    u8 wx = memory_.read(REG_WX);  // Window X position + 7 (7-166)
    
    // Window is only drawn when ly >= wy
    if (ly_ < wy) {
        return;
    }
    
    // Window X=7 means window starts at screen X=0
    int window_x_start = wx - 7;
    
    // If window is completely off screen to the right, skip
    if (window_x_start >= SCREEN_WIDTH) {
        return;
    }
    
    // LCDC bit 6 selects which tile map to use for window
    u16 window_map = (lcdc & LCDC_WINDOW_TILEMAP_BIT) ? TILE_MAP_1 : TILE_MAP_0;
    
    // LCDC bit 4 selects tile data addressing mode (shared with BG)
    u16 tile_data = (lcdc & LCDC_TILE_DATA_BIT) ? TILE_DATA_UNSIGNED_BASE : TILE_DATA_SIGNED_BASE;
    
    // Window uses its own internal line counter, not ly_
    u8 window_y = window_line_counter_;
    u8 tile_y = window_y / TILE_SIZE;
    u8 pixel_y = window_y % TILE_SIZE;
    
    bool window_drawn = false;
    
    // Render each pixel where window is visible
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Window only draws where x >= window_x_start
        if (x < window_x_start) {
            continue;
        }
        
        window_drawn = true;
        
        // Calculate position within window
        int window_x = x - window_x_start;
        u8 tile_x = window_x / TILE_SIZE;
        u8 pixel_x = window_x % TILE_SIZE;
        
        // Get tile from window tile map
        u16 map_addr = window_map + (tile_y * TILEMAP_WIDTH) + tile_x;
        u8 tile_num = memory_.read(map_addr);
        
        // Decode 2-bit pixel from tile pattern data
        u8 pixel = get_tile_pixel(tile_data, tile_num, pixel_x, pixel_y);
        
        // Window uses same palette as background (BGP)
        u8 palette = memory_.read(REG_BGP);
        u8 color = (palette >> (pixel * BITS_PER_PIXEL)) & PALETTE_MASK;
        
        framebuffer_[ly_ * SCREEN_WIDTH + x] = color;
    }
    
    // Only increment window line counter if window was actually drawn on this scanline
    if (window_drawn) {
        window_line_counter_++;
    }
}

void PPU::search_oam() {
    scanline_sprites_.clear();
    
    // Read LCD control register
    u8 lcdc = memory_.read(REG_LCDC);
    bool sprites_enabled = (lcdc & LCDC_OBJ_ENABLE_BIT) != 0;
    
    if (!sprites_enabled) {
        return;
    }
    
    // Determine sprite height (8x8 or 8x16)
    u8 sprite_height = (lcdc & LCDC_OBJ_SIZE_BIT) ? SPRITE_HEIGHT_8X16 : SPRITE_HEIGHT_8X8;
    
    // Search all 40 sprites in OAM
    for (u16 i = 0; i < OAM_SPRITE_COUNT; i++) {
        u16 oam_addr = OAM_START + (i * OAM_ENTRY_SIZE);
        
        Sprite sprite;
        sprite.y = memory_.read(oam_addr + OAM_Y_OFFSET);
        sprite.x = memory_.read(oam_addr + OAM_X_OFFSET);
        sprite.tile = memory_.read(oam_addr + OAM_TILE_OFFSET);
        sprite.flags = memory_.read(oam_addr + OAM_FLAGS_OFFSET);
        sprite.oam_index = static_cast<u8>(i);
        
        // gnuboy logic: Compare against RAW Y value (not screen-adjusted)
        // Skip if: scanline >= Y OR scanline + 16 < Y
        if (ly_ >= sprite.y || ly_ + SPRITE_Y_VISIBILITY_OFFSET < sprite.y) {
            continue;
        }
        
        // For 8x8 mode: additional check
        // Skip if: scanline + 8 >= Y
        if (sprite_height == SPRITE_HEIGHT_8X8 && ly_ + SPRITE_8X8_HEIGHT_CHECK >= sprite.y) {
            continue;
        }
        
        scanline_sprites_.push_back(sprite);
        
        // Hardware limit: max 10 sprites per scanline
        if (scanline_sprites_.size() >= MAX_SPRITES_PER_SCANLINE) {
            break;
        }
    }
}

void PPU::render_sprites() {
    // Read LCD control register
    u8 lcdc = memory_.read(REG_LCDC);
    bool sprites_enabled = (lcdc & LCDC_OBJ_ENABLE_BIT) != 0;
    
    if (!sprites_enabled || scanline_sprites_.empty()) {
        return;
    }
    
    // Determine sprite height
    u8 sprite_height = (lcdc & LCDC_OBJ_SIZE_BIT) ? SPRITE_HEIGHT_8X16 : SPRITE_HEIGHT_8X8;
    
    // Render sprites in reverse order (lower OAM index = higher priority)
    for (auto it = scanline_sprites_.rbegin(); it != scanline_sprites_.rend(); ++it) {
        const Sprite& sprite = *it;
        
        // gnuboy: v = L - (int)o->y + 16
        int sprite_row = ly_ - static_cast<int>(sprite.y) + SPRITE_Y_VISIBILITY_OFFSET;
        int screen_x = static_cast<int>(sprite.x) - SPRITE_X_OFFSET;
        
        // Get tile number
        u8 tile_num = sprite.tile;
        
        // Handle 8x16 mode
        if (sprite_height == SPRITE_HEIGHT_8X16) {
            tile_num &= SPRITE_8X16_TILE_MASK;  // Clear bit 0
            if (sprite_row >= SPRITE_8X16_ROW_THRESHOLD) {
                sprite_row -= SPRITE_8X16_ROW_THRESHOLD;
                tile_num++;
            }
            // gnuboy: Y-flip swaps tiles
            if (sprite.flags & (BIT_1 << SPRITE_FLAG_Y_FLIP_BIT)) {
                tile_num ^= 1;
            }
        }
        
        // Apply Y-flip to row (for both 8x8 and within 8-pixel row for 8x16)
        if (sprite.flags & (BIT_1 << SPRITE_FLAG_Y_FLIP_BIT)) {
            sprite_row = SPRITE_ROW_MASK - sprite_row;
        }
        
        // Render each pixel of the sprite
        for (u8 pixel_x = 0; pixel_x < TILE_SIZE; pixel_x++) {
            int draw_x = screen_x + pixel_x;
            
            // Skip if off screen
            if (draw_x < 0 || draw_x >= SCREEN_WIDTH) {
                continue;
            }
            
            // Get sprite pixel color (use calculated tile_num, not sprite.tile)
            u8 sprite_pixel = get_sprite_pixel(tile_num, sprite.flags, pixel_x, static_cast<u8>(sprite_row));
            
            // Color 0 is transparent for sprites
            if (sprite_pixel == TRANSPARENT_COLOR) {
                continue;
            }
            
            // Check priority against background
            u8 bg_color = framebuffer_[ly_ * SCREEN_WIDTH + draw_x];
            if (is_sprite_priority(sprite.flags, bg_color)) {
                // Apply sprite palette
                u16 palette_reg = (sprite.flags & (BIT_1 << SPRITE_FLAG_PALETTE_BIT)) ? REG_OBP1 : REG_OBP0;
                u8 palette = memory_.read(palette_reg);
                u8 color = (palette >> (sprite_pixel * BITS_PER_PIXEL)) & PALETTE_MASK;
                
                framebuffer_[ly_ * SCREEN_WIDTH + draw_x] = color;
            }
        }
    }
}

u8 PPU::get_sprite_pixel(u8 tile_num, u8 sprite_flags, u8 pixel_x, u8 pixel_y) const {
    // Apply X-flip if needed
    if (sprite_flags & (BIT_1 << SPRITE_FLAG_X_FLIP_BIT)) {
        pixel_x = (TILE_SIZE - BIT_1) - pixel_x;
    }
    
    // Sprites always use tile data at 0x8000-0x8FFF
    u16 tile_addr = TILE_DATA_UNSIGNED_BASE + (tile_num * BYTES_PER_TILE) + (pixel_y * BYTES_PER_TILE_ROW);
    
    u8 byte1 = memory_.read(tile_addr);
    u8 byte2 = memory_.read(tile_addr + BIT_1);
    
    // Extract the 2-bit pixel value
    u8 bit_pos = PIXEL_MSB_BIT - pixel_x;
    u8 pixel = ((byte2 >> bit_pos) & BIT_1) << PIXEL_BIT_SHIFT | ((byte1 >> bit_pos) & BIT_1);
    
    return pixel;
}

bool PPU::is_sprite_priority(u8 sprite_flags, u8 bg_color) const {
    // Priority bit: 0 = sprite above background, 1 = sprite behind background colors 1-3
    bool behind_bg = (sprite_flags & (BIT_1 << SPRITE_FLAG_PRIORITY_BIT)) != BIT_0;
    
    if (!behind_bg) {
        // Sprite is always above background
        return true;
    }
    
    // Sprite is behind background colors 1-3, only visible if BG color is 0
    return bg_color == BIT_0;
}

PPU::Mode PPU::mode() const
{
    return mode_;
}

u8 PPU::scanline() const
{
    return ly_;
}

bool PPU::frame_ready() const
{
    return frame_ready_;
}

void PPU::clear_frame_ready()
{
    frame_ready_ = false;
}

const std::array<u8, 160 * 144>& PPU::framebuffer() const
{
    return framebuffer_;
}

u8 PPU::get_tile_pixel(u16 tile_data_addr, u8 tile_num, u8 x, u8 y) const {
    // Each tile is 16 bytes (8x8 pixels, 2 bits per pixel)
    // Tiles in 0x8800 mode use signed tile numbers
    u16 tile_addr;
    if (tile_data_addr == TILE_DATA_UNSIGNED_BASE) {
        // Unsigned mode: tiles 0-255
        tile_addr = TILE_DATA_UNSIGNED_BASE + (tile_num * BYTES_PER_TILE);
    } else {
        // Signed mode: tiles -128 to 127, base at 0x9000
        i8 signed_tile = static_cast<i8>(tile_num);
        tile_addr = TILE_DATA_SIGNED_BASE + (signed_tile * BYTES_PER_TILE);
    }
    
    // Each row of the tile is 2 bytes
    tile_addr += y * BYTES_PER_TILE_ROW;
    
    u8 byte1 = memory_.read(tile_addr);
    u8 byte2 = memory_.read(tile_addr + BIT_1);
    
    // Extract the 2-bit pixel value
    u8 bit_pos = PIXEL_MSB_BIT - x;
    u8 pixel = ((byte2 >> bit_pos) & BIT_1) << PIXEL_BIT_SHIFT | ((byte1 >> bit_pos) & BIT_1);
    
    return pixel;
}

void PPU::render_to_terminal() const {
    // ASCII characters for different shades
    const char shades[] = {' ', '.', '+', '#'};  // 0=white, 3=black
    
    std::cout << "\n╔";
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

std::vector<u8> PPU::get_rgba_framebuffer() const {
    // DMG palette: 4 shades with greenish tint (classic Game Boy look)
    const u8 palette[4][4] = {
        {0x9B, 0xBC, 0x0F, 0xFF},  // Shade 0: Lightest (greenish yellow)
        {0x8B, 0xAC, 0x0F, 0xFF},  // Shade 1: Light green
        {0x30, 0x62, 0x30, 0xFF},  // Shade 2: Dark green
        {0x0F, 0x38, 0x0F, 0xFF}   // Shade 3: Darkest green
    };
    
    // Allocate RGBA framebuffer (160×144×4 bytes)
    const size_t pixel_count = 160 * 144;
    const size_t rgba_size = pixel_count * 4;
    std::vector<u8> rgba_framebuffer(rgba_size);
    
    // Convert each grayscale pixel to RGBA
    for (size_t i = 0; i < pixel_count; i++) {
        u8 shade = framebuffer_[i] & 0x03;  // Ensure shade is 0-3
        
        // Write RGBA components
        rgba_framebuffer[i * 4 + 0] = palette[shade][0];  // R
        rgba_framebuffer[i * 4 + 1] = palette[shade][1];  // G
        rgba_framebuffer[i * 4 + 2] = palette[shade][2];  // B
        rgba_framebuffer[i * 4 + 3] = palette[shade][3];  // A
    }
    
    return rgba_framebuffer;
}

} // namespace emugbc
