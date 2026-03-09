// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
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
class MBC5 : public Cartridge {
public:
    explicit MBC5(std::vector<u8> rom_data, size_t ram_size, bool has_rumble = false);
    
    u8 read(u16 address) const override;
    void write(u16 address, u8 value) override;
    
private:
    bool ram_enabled_;
    u16 rom_bank_;          // 9-bit ROM bank (0x000-0x1FF)
    u8 ram_bank_;           // 4-bit RAM bank (0x0-0xF)
    bool has_rumble_;       // Rumble support (affects RAM bank register)
    bool rumble_enabled_;   // Rumble motor state (bit 3 of RAM bank register)
};

} // namespace gbglow
