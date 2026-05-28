// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

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
class MBC1 final : public Cartridge {
public:
    explicit MBC1(std::vector<u8> rom_data, size_t ram_size);
    
    u8 read(u16 address) const override;
    void write(u16 address, u8 value) override;
    
    void serialize(std::vector<u8>& data) const override;
    void deserialize(const u8* data, size_t data_size, size_t& offset) override;
    
private:
    bool ram_enabled_;
    u8 rom_bank_;       // 5-bit ROM bank (0x01-0x1F)
    u8 ram_bank_;       // 2-bit RAM bank or upper ROM bits
    bool banking_mode_; // false = ROM mode, true = RAM mode
    
    size_t get_rom_bank() const;
    
    // Memory map constants
    static constexpr u16 ROM_BANK_SIZE = 0x4000;       // 16KB per ROM bank
    static constexpr u16 RAM_BANK_SIZE = 0x2000;       // 8KB per RAM bank
    static constexpr u16 ROM_BANK_0_END = 0x4000;
    static constexpr u16 ROM_BANK_N_END = 0x8000;
    static constexpr u16 RAM_START = 0xA000;
    static constexpr u16 RAM_END = 0xC000;
    
    // Register address ranges
    static constexpr u16 REG_RAM_ENABLE_END = 0x2000;
    static constexpr u16 REG_ROM_BANK_END = 0x4000;
    static constexpr u16 REG_RAM_BANK_END = 0x6000;
    static constexpr u16 REG_BANKING_MODE_END = 0x8000;
    
    // Bank masks
    static constexpr u8 ROM_BANK_MASK = 0x1F;          // 5-bit ROM bank number
    static constexpr u8 RAM_BANK_MASK = 0x03;          // 2-bit RAM bank number
    static constexpr u8 BANKING_MODE_MASK = 0x01;
    
    // RAM enable constants
    static constexpr u8 RAM_ENABLE_MASK = 0x0F;
    static constexpr u8 RAM_ENABLE_VALUE = 0x0A;
    
    // ROM bank upper bit shift
    static constexpr int ROM_BANK_UPPER_SHIFT = 5;
    
    // Open bus value
    static constexpr u8 UNMAPPED_VALUE = 0xFF;
};

} // namespace gbglow
