// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "ppu.h"

#include <iostream>
#include <algorithm>

#include "../core/memory.h"
#include "../cartridge/cartridge.h"

namespace gbglow {

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
    
    // Sprite visibility constants
    constexpr u8 SPRITE_Y_VISIBILITY_OFFSET = 16;  // Sprites use Y+16 coordinate system
    constexpr u8 SPRITE_8X8_HEIGHT_CHECK = 8;      // For 8x8 sprite visibility
    constexpr u8 SPRITE_ROW_MASK = 7;              // Mask for pixel row within 8-pixel tile (0-7)
    
    // Bit manipulation constants
    constexpr u8 BIT_0 = 0;
    constexpr u8 BIT_1 = 1;
    constexpr u8 PIXEL_BIT_SHIFT = 1;
    constexpr u8 PIXEL_MSB_BIT = 7;
    
    // CGB tile attribute bits
    constexpr u8 CGB_ATTR_PALETTE_MASK = 0x07;  // Bits 0-2: palette number (0-7)
    constexpr u8 CGB_ATTR_VRAM_BANK_BIT = 0x08; // Bit 3: VRAM bank
    constexpr u8 CGB_ATTR_X_FLIP_BIT = 0x20;    // Bit 5: horizontal flip
    constexpr u8 CGB_ATTR_Y_FLIP_BIT = 0x40;    // Bit 6: vertical flip
    constexpr u8 CGB_ATTR_PRIORITY_BIT = 0x80;  // Bit 7: BG-to-OAM priority
    
    // CGB register addresses
    constexpr u16 CGB_VBK = 0xFF4F;  // VRAM bank register
    
    // CGB color constants
    constexpr u8 CGB_PALETTE_SHIFT = 2;         // Shift amount for palette in framebuffer
    constexpr u8 CGB_TILE_FLIP_MAX = 7;         // Maximum tile coordinate before flip
    constexpr u8 CGB_PALETTE_SIZE_BYTES = 8;    // 8 bytes per palette (4 colors × 2 bytes)
    constexpr u8 CGB_COLOR_SIZE_BYTES = 2;      // 2 bytes per color
    constexpr u8 CGB_RGB555_RED_SHIFT = 0;      // Red channel bit position
    constexpr u8 CGB_RGB555_GREEN_SHIFT = 5;    // Green channel bit position
    constexpr u8 CGB_RGB555_BLUE_SHIFT = 10;    // Blue channel bit position
    constexpr u8 CGB_RGB555_CHANNEL_MASK = 0x1F; // 5-bit channel mask
    constexpr u16 CGB_RGB555_SCALE_NUMERATOR = 255;  // RGB scaling numerator
    constexpr u8 CGB_RGB555_SCALE_OFFSET = 15;       // RGB scaling offset for rounding
    constexpr u8 CGB_RGB555_SCALE_DIVISOR = 31;      // RGB scaling divisor

    // PPU timing constants (in dots)
    constexpr int DOTS_OAM_SEARCH = 80;          // Mode 2: OAM search duration
    constexpr int DOTS_TRANSFER_END = 252;       // Mode 3 ends at dot 252 (80 + 172)
    constexpr int DOTS_PER_SCANLINE = 456;       // Total dots per scanline
    constexpr int SCANLINES_VISIBLE = 144;       // Visible scanlines (same as SCREEN_HEIGHT)
    constexpr int SCANLINES_TOTAL = 154;         // Total scanlines including VBlank

    // CGB palette specification register constants
    constexpr u8 CPS_INDEX_MASK = 0x3F;          // Lower 6 bits: palette RAM index
    constexpr u8 CPS_AUTO_INCREMENT_BIT = 0x80;  // Bit 7: auto-increment after write
    constexpr u8 CPS_UNUSED_BITS = 0x40;         // Bits 6: always reads as 1

    // DMG shade mask
    constexpr u8 DMG_SHADE_MASK = 0x03;          // 2-bit shade value (0-3)

    // RGBA framebuffer constants
    constexpr int RGBA_CHANNELS = 4;             // R, G, B, A
    constexpr u8 ALPHA_OPAQUE = 0xFF;            // Fully opaque alpha

    // Window X offset (WX register stores position + 7)
    constexpr int WINDOW_X_OFFSET = 7;
}

PPU::PPU(Memory& memory) 
    : memory_(memory)
    , cartridge_(nullptr)
    , mode_(Mode::OAMSearch)
    , dots_(0)
    , ly_(0)
    , frame_ready_(false)
    , bcps_(0)
    , ocps_(0)
    , window_line_counter_(0) {
    framebuffer_.fill(0);
    bg_palette_ram_.fill(0xFF);   // Initialize to white (as per CGB boot ROM)
    obj_palette_ram_.fill(0);     // Initialize to black (sprites unused by default)
}

void PPU::step(Cycles cycles) {
    // Each cycle advances the PPU by 1 dot
    for (Cycles i = 0; i < cycles; i++) {
        dots_++;
        
        switch (mode_) {
            case Mode::OAMSearch:
                if (dots_ >= DOTS_OAM_SEARCH) {
                    // Search OAM for sprites on this scanline
                    search_oam();
                    mode_ = Mode::Transfer;
                }
                break;
                
            case Mode::Transfer:
                if (dots_ >= DOTS_TRANSFER_END) {
                    // Render this scanline
                    render_scanline();
                    mode_ = Mode::HBlank;
                }
                break;
                
            case Mode::HBlank:
                if (dots_ >= DOTS_PER_SCANLINE) {
                    dots_ = 0;
                    ly_++;
                    
                    if (ly_ >= SCANLINES_VISIBLE) {
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
                if (dots_ >= DOTS_PER_SCANLINE) {
                    dots_ = 0;
                    ly_++;
                    
                    if (ly_ >= SCANLINES_TOTAL) {
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
    
    // Check if we're in CGB mode
    bool is_cgb = cartridge_ && cartridge_->is_cgb_supported();
    
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
        
        // CGB: Read tile attributes from VRAM bank 1
        u8 tile_attr = 0;
        u8 palette_num = 0;
        bool x_flip = false;
        bool y_flip = false;
        
        if (is_cgb) {
            // Temporarily switch to VRAM bank 1 to read attributes
            u8 saved_bank = memory_.read(CGB_VBK) & BIT_1;
            memory_.write(CGB_VBK, BIT_1);
            tile_attr = memory_.read(map_addr);
            memory_.write(CGB_VBK, saved_bank);
            
            palette_num = tile_attr & CGB_ATTR_PALETTE_MASK;
            x_flip = (tile_attr & CGB_ATTR_X_FLIP_BIT) != BIT_0;
            y_flip = (tile_attr & CGB_ATTR_Y_FLIP_BIT) != BIT_0;
            // Bit 7 is priority (BG over OBJ), we'll handle this later
        }
        
        // Apply flipping to pixel coordinates
        u8 actual_pixel_x = x_flip ? (CGB_TILE_FLIP_MAX - pixel_x) : pixel_x;
        u8 actual_pixel_y = y_flip ? (CGB_TILE_FLIP_MAX - pixel_y) : pixel_y;
        
        // Decode 2-bit pixel from tile pattern data
        u8 pixel = get_tile_pixel(tile_data, tile_num, actual_pixel_x, actual_pixel_y);
        
        if (is_cgb) {
            // CGB mode: Store palette number in upper bits, color index in lower bits
            framebuffer_[ly_ * SCREEN_WIDTH + x] = (palette_num << CGB_PALETTE_SHIFT) | pixel;
        } else {
            // DMG mode: Apply background palette
            u8 palette = memory_.read(REG_BGP);
            u8 color = (palette >> (pixel * BITS_PER_PIXEL)) & PALETTE_MASK;
            framebuffer_[ly_ * SCREEN_WIDTH + x] = color;
        }
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
    int window_x_start = wx - WINDOW_X_OFFSET;
    
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
        
        // Hardware logic: Compare against RAW Y value (not screen-adjusted)
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
        
        // Calculate sprite row: v = L - (int)o->y + 16
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
            // Y-flip swaps tiles in 8x16 mode
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

const std::array<u8, SCREEN_WIDTH * SCREEN_HEIGHT>& PPU::framebuffer() const
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
    for (int i = 0; i < SCREEN_WIDTH; i++) std::cout << "═";
    std::cout << "╗\n";
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        std::cout << "║";
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            u8 pixel = framebuffer_[y * SCREEN_WIDTH + x];
            std::cout << shades[pixel];
        }
        std::cout << "║\n";
    }
    
    std::cout << "╚";
    for (int i = 0; i < SCREEN_WIDTH; i++) std::cout << "═";
    std::cout << "╝\n";
}

std::vector<u8> PPU::get_rgba_framebuffer() const {
    // Allocate RGBA framebuffer
    const size_t pixel_count = SCREEN_WIDTH * SCREEN_HEIGHT;
    const size_t rgba_size = pixel_count * RGBA_CHANNELS;
    std::vector<u8> rgba_framebuffer(rgba_size);
    
    // Only use CGB color mode for CGB-only games (flag 0xC0)
    // Games with flag 0x80 (CGB supported but also works on DMG) like Pokémon Red
    // are DMG games that don't use CGB color palettes
    bool is_cgb_only = cartridge_ && cartridge_->is_cgb_only();
    
    if (is_cgb_only) {
        // CGB mode: Framebuffer stores (palette_num << CGB_PALETTE_SHIFT) | color_index
        for (size_t i = 0; i < pixel_count; i++) {
            u8 fb_value = framebuffer_[i];
            u8 palette_num = fb_value >> CGB_PALETTE_SHIFT;  // Upper bits: palette number (0-7)
            u8 color_index = fb_value & PALETTE_MASK;  // Lower 2 bits: color index (0-3)
            
            // Read 15-bit RGB from palette RAM
            // Each palette is 4 colors × 2 bytes = 8 bytes
            // Each color is 2 bytes: gggrrrrr 0bbbbbgg (little-endian)
            u8 palette_offset = palette_num * CGB_PALETTE_SIZE_BYTES;
            u8 color_offset = palette_offset + (color_index * CGB_COLOR_SIZE_BYTES);
            u16 rgb555 = bg_palette_ram_[color_offset] | (bg_palette_ram_[color_offset + 1] << 8);
            
            // Convert 15-bit RGB to 8-bit RGB
            u8 r, g, b;
            cgb_rgb555_to_rgba(rgb555, r, g, b);
            
            // Write RGBA components
            rgba_framebuffer[i * RGBA_CHANNELS + 0] = r;
            rgba_framebuffer[i * RGBA_CHANNELS + 1] = g;
            rgba_framebuffer[i * RGBA_CHANNELS + 2] = b;
            rgba_framebuffer[i * RGBA_CHANNELS + 3] = ALPHA_OPAQUE;
        }
    } else {
        // DMG mode: Use neutral grayscale palette
        const u8 palette[4][4] = {
            {0xFF, 0xFF, 0xFF, 0xFF},  // Shade 0: White
            {0xAA, 0xAA, 0xAA, 0xFF},  // Shade 1: Light gray
            {0x55, 0x55, 0x55, 0xFF},  // Shade 2: Dark gray
            {0x00, 0x00, 0x00, 0xFF}   // Shade 3: Black
        };
        
        for (size_t i = 0; i < pixel_count; i++) {
            u8 shade = framebuffer_[i] & DMG_SHADE_MASK;
            
            // Write RGBA components
            rgba_framebuffer[i * RGBA_CHANNELS + 0] = palette[shade][0];  // R
            rgba_framebuffer[i * RGBA_CHANNELS + 1] = palette[shade][1];  // G
            rgba_framebuffer[i * RGBA_CHANNELS + 2] = palette[shade][2];  // B
            rgba_framebuffer[i * RGBA_CHANNELS + 3] = palette[shade][3];  // A
        }
    }
    
    return rgba_framebuffer;
}

// CGB Palette Register Access

u8 PPU::read_bcps() const {
    return bcps_ | CPS_UNUSED_BITS;
}

void PPU::write_bcps(u8 value) {
    bcps_ = value;
}

u8 PPU::read_bcpd() const {
    // Can only read during VBlank or HBlank (Mode 0 or 1)
    if (mode_ != Mode::Transfer && mode_ != Mode::OAMSearch) {
        u8 index = bcps_ & CPS_INDEX_MASK;
        return bg_palette_ram_[index];
    }
    return 0xFF;  // Return 0xFF if PPU is busy
}

void PPU::write_bcpd(u8 value) {
    // Can only write during VBlank or HBlank (Mode 0 or 1)
    if (mode_ != Mode::Transfer && mode_ != Mode::OAMSearch) {
        u8 index = bcps_ & CPS_INDEX_MASK;
        bg_palette_ram_[index] = value;
        
        // Auto-increment if bit 7 is set
        if (bcps_ & CPS_AUTO_INCREMENT_BIT) {
            u8 new_index = (index + 1) & CPS_INDEX_MASK;
            bcps_ = (bcps_ & CPS_AUTO_INCREMENT_BIT) | new_index;
        }
    }
}

u8 PPU::read_ocps() const {
    return ocps_ | CPS_UNUSED_BITS;
}

void PPU::write_ocps(u8 value) {
    ocps_ = value;
}

u8 PPU::read_ocpd() const {
    // Can only read during VBlank or HBlank (Mode 0 or 1)
    if (mode_ != Mode::Transfer && mode_ != Mode::OAMSearch) {
        u8 index = ocps_ & CPS_INDEX_MASK;
        return obj_palette_ram_[index];
    }
    return 0xFF;  // Return 0xFF if PPU is busy
}

void PPU::write_ocpd(u8 value) {
    // Can only write during VBlank or HBlank (Mode 0 or 1)
    if (mode_ != Mode::Transfer && mode_ != Mode::OAMSearch) {
        u8 index = ocps_ & CPS_INDEX_MASK;
        obj_palette_ram_[index] = value;
        
        // Auto-increment if bit 7 is set
        if (ocps_ & CPS_AUTO_INCREMENT_BIT) {
            u8 new_index = (index + 1) & CPS_INDEX_MASK;
            ocps_ = (ocps_ & CPS_AUTO_INCREMENT_BIT) | new_index;
        }
    }
}

void PPU::set_cartridge(const Cartridge* cartridge) {
    cartridge_ = cartridge;
}

void PPU::cgb_rgb555_to_rgba(u16 rgb555, u8& r, u8& g, u8& b) const {
    // CGB format: gggrrrrr 0bbbbbgg (little-endian, stored as 2 bytes)
    // Extract 5-bit values
    u8 r5 = (rgb555 >> CGB_RGB555_RED_SHIFT) & CGB_RGB555_CHANNEL_MASK;
    u8 g5 = (rgb555 >> CGB_RGB555_GREEN_SHIFT) & CGB_RGB555_CHANNEL_MASK;
    u8 b5 = (rgb555 >> CGB_RGB555_BLUE_SHIFT) & CGB_RGB555_CHANNEL_MASK;
    
    // Scale from 5-bit (0-31) to 8-bit (0-255)
    // Formula: (value * 255 + 15) / 31 to get proper rounding
    r = (r5 * CGB_RGB555_SCALE_NUMERATOR + CGB_RGB555_SCALE_OFFSET) / CGB_RGB555_SCALE_DIVISOR;
    g = (g5 * CGB_RGB555_SCALE_NUMERATOR + CGB_RGB555_SCALE_OFFSET) / CGB_RGB555_SCALE_DIVISOR;
    b = (b5 * CGB_RGB555_SCALE_NUMERATOR + CGB_RGB555_SCALE_OFFSET) / CGB_RGB555_SCALE_DIVISOR;
}

// ============================================================================
// Serialization for Save States
// ============================================================================

void PPU::serialize(std::vector<u8>& data) const
{
    // PPU mode and timing
    data.push_back(static_cast<u8>(mode_));
    data.push_back(static_cast<u8>(dots_ & 0xFF));
    data.push_back(static_cast<u8>((dots_ >> 8) & 0xFF));
    data.push_back(ly_);
    data.push_back(frame_ready_ ? 1 : 0);
    data.push_back(window_line_counter_);
    
    // CGB palette specification registers
    data.push_back(bcps_);
    data.push_back(ocps_);
    
    // CGB palette RAM (64 bytes each)
    for (const auto& byte : bg_palette_ram_) {
        data.push_back(byte);
    }
    for (const auto& byte : obj_palette_ram_) {
        data.push_back(byte);
    }
    
    // We don't save framebuffer or scanline_sprites as they're reconstructed
}

void PPU::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    constexpr size_t MODE_BYTE = 1;         // mode_
    constexpr size_t DOTS_BYTES = 2;         // dots_ (u16)
    constexpr size_t LY_BYTE = 1;            // ly_
    constexpr size_t FRAME_READY_BYTE = 1;   // frame_ready_
    constexpr size_t WIN_LINE_BYTE = 1;      // window_line_counter_
    constexpr size_t BCPS_BYTE = 1;          // bcps_
    constexpr size_t OCPS_BYTE = 1;          // ocps_
    constexpr size_t BG_PALETTE_BYTES = 64;  // bg_palette_ram_
    constexpr size_t OBJ_PALETTE_BYTES = 64; // obj_palette_ram_
    constexpr size_t PPU_STATE_SIZE = MODE_BYTE + DOTS_BYTES + LY_BYTE + FRAME_READY_BYTE
        + WIN_LINE_BYTE + BCPS_BYTE + OCPS_BYTE + BG_PALETTE_BYTES + OBJ_PALETTE_BYTES;
    if (offset + PPU_STATE_SIZE > data_size) return;

    // PPU mode and timing
    mode_ = static_cast<Mode>(data[offset++]);
    dots_ = static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8);
    offset += 2;
    ly_ = data[offset++];
    frame_ready_ = data[offset++] != 0;
    window_line_counter_ = data[offset++];
    
    // CGB palette specification registers
    bcps_ = data[offset++];
    ocps_ = data[offset++];
    
    // CGB palette RAM
    for (auto& byte : bg_palette_ram_) {
        byte = data[offset++];
    }
    for (auto& byte : obj_palette_ram_) {
        byte = data[offset++];
    }
    
    // Clear transient state
    scanline_sprites_.clear();
}

} // namespace gbglow
