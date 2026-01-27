#pragma once

#include "../core/types.h"
#include <array>
#include <cstdint>

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
    
    explicit PPU(Memory& memory);
    
    // Execute PPU for given number of cycles
    void step(Cycles cycles);
    
    // Get current mode
    Mode mode() const { return mode_; }
    
    // Get current scanline
    u8 scanline() const { return ly_; }
    
    // Check if frame is complete (for rendering)
    bool frame_ready() const { return frame_ready_; }
    void clear_frame_ready() { frame_ready_ = false; }
    
    // Get framebuffer (160x144, grayscale 0-3)
    const std::array<u8, 160 * 144>& framebuffer() const { return framebuffer_; }
    
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
    
    // Render current scanline
    void render_scanline();
    
    // Get tile pixel (returns 0-3)
    u8 get_tile_pixel(u16 tile_data_addr, u8 tile_num, u8 x, u8 y);
};

} // namespace emugbc
