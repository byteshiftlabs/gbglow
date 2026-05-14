// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "mbc1.h"

namespace gbglow {

MBC1::MBC1(std::vector<u8> rom_data, size_t ram_size)
    : Cartridge(std::move(rom_data), ram_size)
    , ram_enabled_(false)
    , rom_bank_(1)
    , ram_bank_(0)
    , banking_mode_(false)
{
}

size_t MBC1::get_rom_bank() const {
    if (banking_mode_) {
        // RAM banking mode - only lower 5 bits used
        return rom_bank_;
    } else {
        // ROM banking mode - combine with upper bits
        return rom_bank_ | (ram_bank_ << ROM_BANK_UPPER_SHIFT);
    }
}

u8 MBC1::read(u16 address) const {
    if (address < ROM_BANK_0_END) {
        // ROM Bank 0 (or bank 0x00/0x20/0x40/0x60 in RAM banking mode)
        size_t bank = banking_mode_ ? (ram_bank_ << ROM_BANK_UPPER_SHIFT) : 0;
        size_t offset = (bank * ROM_BANK_SIZE) + address;
        if (offset < rom_.size()) {
            return rom_[offset];
        }
        return UNMAPPED_VALUE;
    }
    else if (address < ROM_BANK_N_END) {
        // ROM Bank 1-N (switchable)
        size_t bank = get_rom_bank();
        size_t offset = (bank * ROM_BANK_SIZE) + (address - ROM_BANK_0_END);
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
        
        size_t bank = banking_mode_ ? ram_bank_ : 0;
        size_t offset = (bank * RAM_BANK_SIZE) + (address - RAM_START);
        if (offset < ram_.size()) {
            return ram_[offset];
        }
        return UNMAPPED_VALUE;
    }
    
    return UNMAPPED_VALUE;
}

void MBC1::write(u16 address, u8 value) {
    if (address < REG_RAM_ENABLE_END) {
        // RAM Enable
        ram_enabled_ = (value & RAM_ENABLE_MASK) == RAM_ENABLE_VALUE;
    }
    else if (address < REG_ROM_BANK_END) {
        // ROM Bank Number (lower 5 bits)
        rom_bank_ = value & ROM_BANK_MASK;
        if (rom_bank_ == 0) {
            rom_bank_ = 1;  // Bank 0 not accessible here
        }
    }
    else if (address < REG_RAM_BANK_END) {
        // RAM Bank Number / Upper ROM Bank bits
        ram_bank_ = value & RAM_BANK_MASK;
    }
    else if (address < REG_BANKING_MODE_END) {
        // Banking Mode Select
        banking_mode_ = (value & BANKING_MODE_MASK) != 0;
    }
    else if (address >= RAM_START && address < RAM_END) {
        // External RAM write
        if (ram_enabled_ && !ram_.empty()) {
            size_t bank = banking_mode_ ? ram_bank_ : 0;
            size_t offset = (bank * RAM_BANK_SIZE) + (address - RAM_START);
            if (offset < ram_.size()) {
                ram_[offset] = value;
            }
        }
    }
}

void MBC1::serialize(std::vector<u8>& data) const
{
    Cartridge::serialize(data);
    data.push_back(ram_enabled_ ? 1 : 0);
    data.push_back(rom_bank_);
    data.push_back(ram_bank_);
    data.push_back(banking_mode_ ? 1 : 0);
}

void MBC1::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    Cartridge::deserialize(data, data_size, offset);

    constexpr size_t MBC1_STATE_SIZE = 4;
    if (offset + MBC1_STATE_SIZE > data_size) return;

    ram_enabled_  = data[offset++] != 0;
    rom_bank_     = data[offset++];
    ram_bank_     = data[offset++];
    banking_mode_ = data[offset++] != 0;
}

} // namespace gbglow
