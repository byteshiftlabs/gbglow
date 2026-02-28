#pragma once

#include "cartridge.h"

namespace gbglow {

/**
 * MBC1 (Memory Bank Controller 1)
 * 
 * The most common MBC, supporting:
 * - Up to 2MB ROM (125 banks of 16KB)
 * - Up to 32KB RAM (4 banks of 8KB)
 * - Two banking modes
 * 
 * Memory regions:
 * 0x0000-0x1FFF: RAM Enable
 * 0x2000-0x3FFF: ROM Bank Number (lower 5 bits)
 * 0x4000-0x5FFF: RAM Bank Number / Upper ROM Bank bits
 * 0x6000-0x7FFF: Banking Mode Select
 */
class MBC1 : public Cartridge {
public:
    explicit MBC1(std::vector<u8> rom_data, size_t ram_size);
    
    u8 read(u16 address) const override;
    void write(u16 address, u8 value) override;
    
private:
    bool ram_enabled_;
    u8 rom_bank_;       // 5-bit ROM bank (0x01-0x1F)
    u8 ram_bank_;       // 2-bit RAM bank or upper ROM bits
    bool banking_mode_; // false = ROM mode, true = RAM mode
    
    size_t get_rom_bank() const;
};

} // namespace gbglow
