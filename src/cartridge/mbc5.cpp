// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
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
    if (address < ROM_BANK_0_END) {
        // ROM Bank 0 (always mapped to first 16KB)
        if (address < rom_.size()) {
            return rom_[address];
        }
        return UNMAPPED_VALUE;
    }
    else if (address < ROM_BANK_N_END) {
        // ROM Bank 1-511 (switchable)
        // MBC5 uses full 9-bit bank number, no automatic bank adjustment
        size_t offset = (rom_bank_ * ROM_BANK_SIZE) + (address - ROM_BANK_0_END);
        if (offset < rom_.size()) {
            return rom_[offset];
        }
        return UNMAPPED_VALUE;
    }
    else if (address >= RAM_START && address < RAM_END) {
        // External RAM
        if (!ram_enabled_ || ram_.empty()) {
            return UNMAPPED_VALUE;
        }
        
        // Extract actual RAM bank (bits 0-3, ignoring rumble bit if present)
        u8 actual_ram_bank = ram_bank_ & RAM_BANK_MASK;
        size_t offset = (actual_ram_bank * RAM_BANK_SIZE) + (address - RAM_START);
        if (offset < ram_.size()) {
            return ram_[offset];
        }
        return UNMAPPED_VALUE;
    }
    
    return UNMAPPED_VALUE;
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
    if (address < REG_RAM_ENABLE_END) {
        // RAM Enable
        // MBC5 checks lower nibble for 0x0A
        ram_enabled_ = (value & RAM_ENABLE_MASK) == RAM_ENABLE_VALUE;
    }
    else if (address < REG_ROM_BANK_LOW_END) {
        // ROM Bank Number - Lower 8 bits
        // Update lower 8 bits, preserve 9th bit
        rom_bank_ = (rom_bank_ & ROM_BANK_HIGH_BIT) | value;
    }
    else if (address < REG_ROM_BANK_HIGH_END) {
        // ROM Bank Number - 9th bit (upper bit)
        // Only bit 0 of value is used for the 9th bit of rom_bank_
        rom_bank_ = (rom_bank_ & ROM_BANK_LOW_MASK) | ((value & ROM_BANK_9TH_BIT_MASK) << 8);
    }
    else if (address < REG_RAM_BANK_END) {
        // RAM Bank Number (0x0-0xF) and optional rumble
        if (has_rumble_) {
            // Bit 3 controls rumble motor
            rumble_enabled_ = (value & RUMBLE_BIT) != 0;
            // Lower 3 bits for RAM bank (0x0-0x7)
            ram_bank_ = value & RAM_BANK_RUMBLE_MASK;
        } else {
            // Full 4 bits for RAM bank (0x0-0xF)
            ram_bank_ = value & RAM_BANK_MASK;
        }
    }
    else if (address >= RAM_START && address < RAM_END) {
        // External RAM write
        if (ram_enabled_ && !ram_.empty()) {
            u8 actual_ram_bank = ram_bank_ & RAM_BANK_MASK;
            size_t offset = (actual_ram_bank * RAM_BANK_SIZE) + (address - RAM_START);
            if (offset < ram_.size()) {
                ram_[offset] = value;
            }
        }
    }
}

void MBC5::serialize(std::vector<u8>& data) const
{
    Cartridge::serialize(data);
    data.push_back(ram_enabled_ ? 1 : 0);
    // rom_bank_ is u16 — save as 2-byte LE
    data.push_back(static_cast<u8>(rom_bank_ & 0xFF));
    data.push_back(static_cast<u8>((rom_bank_ >> 8) & 0xFF));
    data.push_back(ram_bank_);
    data.push_back(rumble_enabled_ ? 1 : 0);
}

void MBC5::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    Cartridge::deserialize(data, data_size, offset);

    constexpr size_t MBC5_STATE_SIZE = 5;  // 1 + 2 + 1 + 1
    if (offset + MBC5_STATE_SIZE > data_size) return;

    ram_enabled_    = data[offset++] != 0;
    rom_bank_       = (static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8))
        & (ROM_BANK_HIGH_BIT | ROM_BANK_LOW_MASK);
    offset += 2;
    ram_bank_       = data[offset++] & (has_rumble_ ? RAM_BANK_RUMBLE_MASK : RAM_BANK_MASK);
    rumble_enabled_ = data[offset++] != 0;
}

} // namespace gbglow
