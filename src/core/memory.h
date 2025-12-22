#pragma once

#include "types.h"
#include <array>
#include <memory>
#include <vector>

namespace emugbc {

// Forward declarations
class Cartridge;
class Joypad;
class Timer;
class APU;
class PPU;

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
    
    // Get APU reference for audio operations
    APU& apu();
    
    // Set PPU reference (called by Emulator after PPU creation)
    void set_ppu(PPU* ppu);
    
    // Get cartridge pointer (may be null)
    Cartridge* cartridge();
    
    // Read/Write operations - hot path functions, compiler will inline with optimizations
    u8 read(u16 address) const;
    void write(u16 address, u8 value);
    
    // 16-bit read/write helpers (little-endian)
    u16 read16(u16 address) const;
    void write16(u16 address, u16 value);
    
    // Serialization for save states
    void serialize(std::vector<u8>& data) const;
    void deserialize(const u8* data, size_t& offset);
    
private:
    // CGB memory size constants
    static constexpr u16 VRAM_TOTAL_SIZE = 0x4000;  // 16KB total (CGB)
    static constexpr u16 VRAM_BANK_SIZE = 0x2000;   // 8KB per bank
    static constexpr u16 WRAM_SIZE = 0x2000;        // 8KB Work RAM
    
    // Internal memory regions
    std::array<u8, VRAM_TOTAL_SIZE> vram_;  // 16KB Video RAM (CGB: 2 banks × 8KB)
    u8 vram_bank_;                          // CGB VRAM bank select (VBK register 0xFF4F)
    u8 speed_switch_;                       // CGB speed switch (KEY1 register 0xFF4D)
    std::array<u8, WRAM_SIZE> wram_;        // 8KB Work RAM
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
    std::unique_ptr<APU> apu_;
    PPU* ppu_;  // Non-owning pointer, set by Emulator
};

} // namespace emugbc
