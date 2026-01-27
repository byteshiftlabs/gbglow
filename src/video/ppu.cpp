#include "ppu.h"

#include <iostream>
#include <iomanip>

#include "../core/memory.h"

namespace emugbc {

// Hardware constants
namespace {
    constexpr int SCREEN_WIDTH = 160;
    constexpr int SCREEN_HEIGHT = 144;
    constexpr int TILE_SIZE = 8;
    constexpr int TILEMAP_WIDTH = 32;  // Background map is 32x32 tiles
    constexpr int BYTES_PER_TILE = 16; // Each tile is 16 bytes (8x8 pixels, 2bpp)
}

PPU::PPU(Memory& memory) 
    : memory_(memory)
    , mode_(Mode::OAMSearch)
    , dots_(0)
    , ly_(0)
    , frame_ready_(false) {
    framebuffer_.fill(0);
}

void PPU::step(Cycles cycles) {
    // Each cycle advances the PPU by 1 dot
    for (Cycles i = 0; i < cycles; i++) {
        dots_++;
        
        switch (mode_) {
            case Mode::OAMSearch:
                if (dots_ >= 80) {
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
                        
                        // Set VBlank flag in IF register (0xFF0F)
                        u8 if_reg = memory_.read(0xFF0F);
                        memory_.write(0xFF0F, if_reg | 0x01);  // Set bit 0 (VBlank)
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
                        mode_ = Mode::OAMSearch;
                    }
                }
                break;
        }
        
        // Update LY register (0xFF44)
        memory_.write(0xFF44, ly_);
        
        // Update STAT register (0xFF41) with current mode
        u8 stat = memory_.read(0xFF41);
        stat = (stat & 0xFC) | static_cast<u8>(mode_);
        memory_.write(0xFF41, stat);
    }
}

void PPU::render_scanline() {
    if (ly_ >= 144) return;  // Only render visible scanlines
    
    // Read LCD control register (0xFF40)
    u8 lcdc = memory_.read(0xFF40);
    bool bg_enabled = (lcdc & 0x01) != 0;
    
    if (!bg_enabled) {
        // Background disabled, fill with white
        for (int x = 0; x < 160; x++) {
            framebuffer_[ly_ * 160 + x] = 0;
        }
        return;
    }
    
    // Hardware scroll registers allow background to pan
    u8 scy = memory_.read(0xFF42);
    u8 scx = memory_.read(0xFF43);
    
    // LCDC bit 3 selects which of two tile maps to use
    u16 bg_map = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    
    // LCDC bit 4 selects tile data addressing mode (signed vs unsigned)
    u16 tile_data = (lcdc & 0x10) ? 0x8000 : 0x8800;
    
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
        u8 palette = memory_.read(0xFF47);
        u8 color = (palette >> (pixel * 2)) & 0x03;
        
        framebuffer_[ly_ * SCREEN_WIDTH + x] = color;
    }
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

u8 PPU::get_tile_pixel(u16 tile_data_addr, u8 tile_num, u8 x, u8 y) {
    // Each tile is 16 bytes (8x8 pixels, 2 bits per pixel)
    // Tiles in 0x8800 mode use signed tile numbers
    u16 tile_addr;
    if (tile_data_addr == 0x8000) {
        // Unsigned mode: tiles 0-255
        tile_addr = 0x8000 + (tile_num * BYTES_PER_TILE);
    } else {
        // Signed mode: tiles -128 to 127, base at 0x9000
        i8 signed_tile = static_cast<i8>(tile_num);
        tile_addr = 0x9000 + (signed_tile * BYTES_PER_TILE);
    }
    
    // Each row of the tile is 2 bytes
    tile_addr += y * 2;
    
    u8 byte1 = memory_.read(tile_addr);
    u8 byte2 = memory_.read(tile_addr + 1);
    
    // Extract the 2-bit pixel value
    u8 bit_pos = 7 - x;
    u8 pixel = ((byte2 >> bit_pos) & 1) << 1 | ((byte1 >> bit_pos) & 1);
    
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

} // namespace emugbc
