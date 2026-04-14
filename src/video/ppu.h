// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "../core/types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gbglow {

class Memory;
class Cartridge;

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
    PPU(const PPU&) = delete;
    PPU& operator=(const PPU&) = delete;
    PPU(PPU&&) = delete;
    PPU& operator=(PPU&&) = delete;
    
    // Execute PPU for given number of cycles
    void step(Cycles cycles);
    
    // Accessors
    Mode mode() const;
    u8 scanline() const;
    
    // Controlled mutators used by tests and state setup. These keep MMIO-visible
    // state coherent instead of exposing raw field writes that bypass invariants.
    void set_mode(Mode m);
    void set_scanline(u8 ly);
    
    // Save state accessors (return raw values for serialization)
    u8 get_ly() const { return ly_; }
    u8 get_mode() const { return static_cast<u8>(mode_); }

    // Frame buffer access
    bool frame_ready() const;
    void clear_frame_ready();
    
    // Screen dimensions
    static constexpr int SCREEN_WIDTH = 160;
    static constexpr int SCREEN_HEIGHT = 144;

    // Get framebuffer (SCREEN_WIDTH x SCREEN_HEIGHT, grayscale 0-3)
    const std::array<u8, SCREEN_WIDTH * SCREEN_HEIGHT>& framebuffer() const;
    
    // Get RGBA framebuffer for display (160x144x4 bytes)
    std::vector<u8> get_rgba_framebuffer() const;
    
    // CGB Palette register access
    u8 read_bcps() const;       // Read 0xFF68
    void write_bcps(u8 value);  // Write 0xFF68
    u8 read_bcpd() const;       // Read 0xFF69
    void write_bcpd(u8 value);  // Write 0xFF69
    u8 read_ocps() const;       // Read 0xFF6A
    void write_ocps(u8 value);  // Write 0xFF6A
    u8 read_ocpd() const;       // Read 0xFF6B
    void write_ocpd(u8 value);  // Write 0xFF6B
    
    // Cartridge access for CGB mode detection
    void set_cartridge(const Cartridge* cartridge);
    
    // Serialization for save states
    void serialize(std::vector<u8>& data) const;
    void deserialize(const u8* data, size_t data_size, size_t& offset);

    // Refresh coincidence and STAT IRQ state after CPU-visible MMIO writes.
    void refresh_stat_signal();
    
private:
    Memory& memory_;
    const Cartridge* cartridge_;  // Non-owning pointer for CGB mode detection
    
    Mode mode_;
    u16 dots_;           // Dot counter within current scanline
    u8 ly_;              // Current scanline (LY register)
    bool frame_ready_;
    bool lcd_was_on_;     // Tracks LCD enable state for edge detection
    bool stat_irq_line_;  // Combined STAT IRQ OR-gate state; interrupt fires only on low→high edge
    
    // Framebuffer: SCREEN_WIDTH x SCREEN_HEIGHT pixels, each pixel is 0-3 (grayscale)
    std::array<u8, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer_;
    
    // CGB Color Palette Memory
    std::array<u8, 64> bg_palette_ram_;   // 8 palettes × 4 colors × 2 bytes
    std::array<u8, 64> obj_palette_ram_;  // 8 palettes × 4 colors × 2 bytes
    u8 bcps_;  // Background Palette Specification (0xFF68)
    u8 ocps_;  // Object Palette Specification (0xFF6A)
    
    // Sprites visible on current scanline
    std::vector<Sprite> scanline_sprites_;
    
    // Window line counter (tracks which line of window is being drawn)
    u8 window_line_counter_;
    
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
    void render_window();
    void render_sprites();
    
    // OAM search
    void search_oam();
    
    // Sprite helpers
    u8 get_sprite_pixel(u8 tile_num, u8 sprite_flags, u8 pixel_x, u8 pixel_y, bool use_vram_bank_1) const;
    bool is_sprite_priority(u8 sprite_flags, u8 bg_color) const;
    u8 read_vram_bank_1(u16 address) const;
    void read_vram_bank_1_pair(u16 address, u8& first_byte, u8& second_byte) const;
    
    // Tile data
    u8 get_tile_pixel(u16 tile_data_addr, u8 tile_num, u8 x, u8 y) const;

    // CGB Color conversion
    void cgb_rgb555_to_rgba(u16 rgb555, u8& r, u8& g, u8& b) const;

    // STAT interrupt helpers
    // Recompute the combined STAT IRQ OR-gate and fire IF bit 1 only on a low→high edge.
    // Must be called after every mode transition and after every REG_STAT / LY change.
    void update_stat_irq_line();
    void update_lyc_coincidence();  // Sync LYC=LY flag then call update_stat_irq_line()
    void write_stat_mode_bits(Mode mode);
};

} // namespace gbglow
