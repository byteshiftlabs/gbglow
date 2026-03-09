// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "mbc5.h"

namespace gbglow {

/**
 * MBC5 Constructor
 * 
 * Initializes MBC5 with ROM data and optional RAM/rumble support.
 * ROM bank starts at 1 by convention, though MBC5 allows bank 0 selection.
 */
MBC5::MBC5(std::vector<u8> rom_data, size_t ram_size, bool has_rumble)
    : Cartridge(std::move(rom_data), ram_size)
    , ram_enabled_(false)
    , rom_bank_(1)
    , ram_bank_(0)
    , has_rumble_(has_rumble)
    , rumble_enabled_(false)
{
}

/**
 * Read from cartridge address space
 * 
 * 0x0000-0x3FFF: ROM Bank 0 (fixed)
 * 0x4000-0x7FFF: ROM Bank 1-511 (switchable)
 * 0xA000-0xBFFF: External RAM (if enabled)
 */
u8 MBC5::read(u16 address) const {
    if (address < 0x4000) {
        // ROM Bank 0 (always mapped to first 16KB)
        if (address < rom_.size()) {
            return rom_[address];
        }
        return 0xFF;
    }
    else if (address < 0x8000) {
        // ROM Bank 1-511 (switchable)
        // MBC5 uses full 9-bit bank number, no automatic bank adjustment
        size_t offset = (rom_bank_ * 0x4000) + (address - 0x4000);
        if (offset < rom_.size()) {
            return rom_[offset];
        }
        return 0xFF;
    }
    else if (address >= 0xA000 && address < 0xC000) {
        // External RAM
        if (!ram_enabled_ || ram_.empty()) {
            return 0xFF;
        }
        
        // Extract actual RAM bank (bits 0-3, ignoring rumble bit if present)
        u8 actual_ram_bank = ram_bank_ & 0x0F;
        size_t offset = (actual_ram_bank * 0x2000) + (address - 0xA000);
        if (offset < ram_.size()) {
            return ram_[offset];
        }
        return 0xFF;
    }
    
    return 0xFF;
}

/**
 * Write to cartridge address space
 * 
 * 0x0000-0x1FFF: RAM Enable (0x0A enables, anything else disables)
 * 0x2000-0x2FFF: ROM Bank Number - Lower 8 bits
 * 0x3000-0x3FFF: ROM Bank Number - Upper bit (9th bit)
 * 0x4000-0x5FFF: RAM Bank Number (4 bits) / Rumble (bit 3 if rumble present)
 * 0xA000-0xBFFF: External RAM write
 */
void MBC5::write(u16 address, u8 value) {
    if (address < 0x2000) {
        // RAM Enable
        // MBC5 checks lower nibble for 0x0A
        ram_enabled_ = (value & 0x0F) == 0x0A;
    }
    else if (address < 0x3000) {
        // ROM Bank Number - Lower 8 bits
        // Update lower 8 bits, preserve 9th bit
        rom_bank_ = (rom_bank_ & 0x100) | value;
    }
    else if (address < 0x4000) {
        // ROM Bank Number - 9th bit (upper bit)
        // Only bit 0 of value is used for the 9th bit of rom_bank_
        rom_bank_ = (rom_bank_ & 0xFF) | ((value & 0x01) << 8);
    }
    else if (address < 0x6000) {
        // RAM Bank Number (0x0-0xF) and optional rumble
        if (has_rumble_) {
            // Bit 3 controls rumble motor
            rumble_enabled_ = (value & 0x08) != 0;
            // Lower 3 bits for RAM bank (0x0-0x7)
            ram_bank_ = value & 0x07;
        } else {
            // Full 4 bits for RAM bank (0x0-0xF)
            ram_bank_ = value & 0x0F;
        }
    }
    else if (address >= 0xA000 && address < 0xC000) {
        // External RAM write
        if (ram_enabled_ && !ram_.empty()) {
            u8 actual_ram_bank = ram_bank_ & 0x0F;
            size_t offset = (actual_ram_bank * 0x2000) + (address - 0xA000);
            if (offset < ram_.size()) {
                ram_[offset] = value;
            }
        }
    }
}

} // namespace gbglow
