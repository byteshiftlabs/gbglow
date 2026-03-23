// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "cartridge.h"

namespace gbglow {

/**
 * MBC5 (Memory Bank Controller 5)
 * 
 * The most advanced standard MBC, supporting:
 * - Up to 8MB ROM (512 banks of 16KB)
 * - Up to 128KB RAM (16 banks of 8KB)
 * - Optional rumble feature
 * 
 * Used by many Game Boy Color games including Pokémon Crystal,
 * Pokémon Gold/Silver (Red/Blue Spanish versions), and others.
 * 
 * Memory regions:
 * 0x0000-0x1FFF: RAM Enable
 * 0x2000-0x2FFF: ROM Bank Number (lower 8 bits)
 * 0x3000-0x3FFF: ROM Bank Number (9th bit, bit 0 only)
 * 0x4000-0x5FFF: RAM Bank Number (4 bits, or 3 bits + rumble bit)
 * 
 * Key differences from MBC1/MBC3:
 * - 9-bit ROM bank register (0x000-0x1FF) instead of 5-7 bits
 * - 4-bit RAM bank register (0x0-0xF) instead of 2 bits
 * - No special banking modes - straightforward linear banking
 * - ROM bank 0 can be selected for switchable bank (unlike MBC1)
 */
class MBC5 final : public Cartridge {
public:
    explicit MBC5(std::vector<u8> rom_data, size_t ram_size, bool has_rumble = false);
    
    u8 read(u16 address) const override;
    void write(u16 address, u8 value) override;
    
    void serialize(std::vector<u8>& data) const override;
    void deserialize(const u8* data, size_t data_size, size_t& offset) override;
    
private:
    bool ram_enabled_;
    u16 rom_bank_;          // 9-bit ROM bank (0x000-0x1FF)
    u8 ram_bank_;           // 4-bit RAM bank (0x0-0xF)
    bool has_rumble_;       // Rumble support (affects RAM bank register)
    bool rumble_enabled_;   // Rumble motor state (bit 3 of RAM bank register)
    
    // Memory map constants
    static constexpr u16 ROM_BANK_SIZE = 0x4000;       // 16KB per ROM bank
    static constexpr u16 RAM_BANK_SIZE = 0x2000;       // 8KB per RAM bank
    static constexpr u16 ROM_BANK_0_END = 0x4000;
    static constexpr u16 ROM_BANK_N_END = 0x8000;
    static constexpr u16 RAM_START = 0xA000;
    static constexpr u16 RAM_END = 0xC000;
    
    // Register address ranges
    static constexpr u16 REG_RAM_ENABLE_END = 0x2000;
    static constexpr u16 REG_ROM_BANK_LOW_END = 0x3000;
    static constexpr u16 REG_ROM_BANK_HIGH_END = 0x4000;
    static constexpr u16 REG_RAM_BANK_END = 0x6000;
    
    // Bank masks
    static constexpr u16 ROM_BANK_HIGH_BIT = 0x100;    // 9th bit of ROM bank
    static constexpr u8 ROM_BANK_LOW_MASK = 0xFF;      // Lower 8 bits of ROM bank
    static constexpr u8 RAM_BANK_MASK = 0x0F;          // 4-bit RAM bank number
    static constexpr u8 RAM_BANK_RUMBLE_MASK = 0x07;   // 3-bit RAM bank (rumble mode)
    static constexpr u8 ROM_BANK_9TH_BIT_MASK = 0x01;  // Only bit 0 for 9th ROM bank bit
    
    // RAM enable constants
    static constexpr u8 RAM_ENABLE_MASK = 0x0F;
    static constexpr u8 RAM_ENABLE_VALUE = 0x0A;
    
    // Rumble constants
    static constexpr u8 RUMBLE_BIT = 0x08;             // Bit 3 controls rumble motor
    
    // Open bus value
    static constexpr u8 UNMAPPED_VALUE = 0xFF;
};

} // namespace gbglow
