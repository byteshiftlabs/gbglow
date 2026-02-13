#pragma once

#include "types.h"
#include <array>
#include <memory>

namespace emugbc {

// Forward declarations
class Cartridge;
class Joypad;
class Timer;

/**
 * Memory Management Unit (MMU)
 * 
 * Game Boy Memory Map:
 * 0x0000-0x3FFF : ROM Bank 0 (16KB)
 * 0x4000-0x7FFF : ROM Bank 1-N (16KB, switchable)
 * 0x8000-0x9FFF : Video RAM (8KB)
 * 0xA000-0xBFFF : External RAM (8KB, cartridge)
 * 0xC000-0xCFFF : Work RAM Bank 0 (4KB)
 * 0xD000-0xDFFF : Work RAM Bank 1 (4KB)
 * 0xE000-0xFDFF : Echo RAM (mirror of C000-DDFF)
 * 0xFE00-0xFE9F : Object Attribute Memory (OAM)
 * 0xFEA0-0xFEFF : Unusable
 * 0xFF00-0xFF7F : I/O Registers
 * 0xFF80-0xFFFE : High RAM (HRAM)
 * 0xFFFF        : Interrupt Enable Register
 */
class Memory {
public:
    Memory();
    ~Memory();  // Defined in .cpp where Cartridge is complete
    
    // Load a cartridge into memory
    void load_cartridge(std::unique_ptr<Cartridge> cart);
    
    // Load boot ROM (256 bytes, optional)
    bool load_boot_rom(const std::string& path);
    
    // Get joypad reference for input handling
    Joypad& joypad();
    
    // Get timer reference for timing operations
    Timer& timer();
    
    // Read/Write operations
    u8 read(u16 address) const;
    void write(u16 address, u8 value);
    
    // 16-bit read/write helpers (little-endian)
    u16 read16(u16 address) const;
    void write16(u16 address, u16 value);
    
private:
    // Internal memory regions
    std::array<u8, 0x2000> vram_;      // 8KB Video RAM
    std::array<u8, 0x2000> wram_;      // 8KB Work RAM
    std::array<u8, 0x00A0> oam_;       // Object Attribute Memory
    std::array<u8, 0x0080> hram_;      // High RAM
    std::array<u8, 0x0080> io_regs_;   // I/O registers
    
    // Boot ROM (256 bytes at 0x0000-0x00FF)
    std::array<u8, 0x0100> boot_rom_;
    bool boot_rom_loaded_;
    bool boot_rom_enabled_;
    
    u8 interrupt_enable_;  // 0xFFFF register
    
    std::unique_ptr<Cartridge> cartridge_;
    std::unique_ptr<Joypad> joypad_;
    std::unique_ptr<Timer> timer_;
};

} // namespace emugbc
