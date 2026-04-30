// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "cartridge.h"

#include <fstream>
#include <stdexcept>
#include <cstring>

#include "mbc1.h"
#include "mbc3.h"
#include "mbc5.h"

namespace gbglow {

// Cartridge header offsets per Game Boy cartridge specification
namespace {
    constexpr u16 HEADER_TITLE_OFFSET = 0x0134;
    constexpr u16 HEADER_CGB_FLAG_OFFSET = 0x0143;
    constexpr u16 HEADER_CARTRIDGE_TYPE_OFFSET = 0x0147;
    constexpr u16 HEADER_RAM_SIZE_OFFSET = 0x0149;
    constexpr int TITLE_MAX_LENGTH = 16;
    constexpr size_t MIN_HEADER_SIZE = 0x0150;  // Minimum ROM size for valid header

    // Cartridge type codes from Game Boy hardware specification
    constexpr u8 CART_ROM_ONLY              = 0x00;
    constexpr u8 CART_MBC1                  = 0x01;
    constexpr u8 CART_MBC1_RAM              = 0x02;
    constexpr u8 CART_MBC1_RAM_BATTERY      = 0x03;
    constexpr u8 CART_MBC2_BATTERY          = 0x06;
    constexpr u8 CART_ROM_RAM_BATTERY       = 0x09;
    constexpr u8 CART_MMM01_RAM_BATTERY     = 0x0D;
    constexpr u8 CART_MBC3_TIMER_BATTERY    = 0x0F;
    constexpr u8 CART_MBC3_TIMER_RAM_BAT    = 0x10;
    constexpr u8 CART_MBC3                  = 0x11;
    constexpr u8 CART_MBC3_RAM              = 0x12;
    constexpr u8 CART_MBC3_RAM_BATTERY      = 0x13;
    constexpr u8 CART_MBC5                  = 0x19;
    constexpr u8 CART_MBC5_RAM              = 0x1A;
    constexpr u8 CART_MBC5_RAM_BATTERY      = 0x1B;
    constexpr u8 CART_MBC5_RUMBLE           = 0x1C;
    constexpr u8 CART_MBC5_RUMBLE_RAM       = 0x1D;
    constexpr u8 CART_MBC5_RUMBLE_RAM_BAT   = 0x1E;
    constexpr u8 CART_HUC1_RAM_BATTERY      = 0xFF;
    
    // CGB flag values at 0x0143
    constexpr u8 CGB_FLAG_SUPPORTED = 0x80;  // Supports CGB functions (also works on DMG)
    constexpr u8 CGB_FLAG_ONLY = 0xC0;       // CGB-only game (doesn't work on DMG)
    
    // RAM size codes from cartridge header
    constexpr size_t RAM_SIZE_NONE = 0;
    constexpr size_t RAM_SIZE_2KB = 2 * 1024;
    constexpr size_t RAM_SIZE_8KB = 8 * 1024;
    constexpr size_t RAM_SIZE_32KB = 32 * 1024;
    constexpr size_t RAM_SIZE_128KB = 128 * 1024;
    constexpr size_t RAM_SIZE_64KB = 64 * 1024;
    
    /**
     * Get RAM size from header byte
     * 0x00: No RAM
     * 0x01: 2 KB (unused in MBC1/3)
     * 0x02: 8 KB (1 bank)
     * 0x03: 32 KB (4 banks)
     * 0x04: 128 KB (16 banks)
     * 0x05: 64 KB (8 banks)
     */
    size_t get_ram_size(u8 ram_size_code) {
        switch (ram_size_code) {
            case 0x00: return RAM_SIZE_NONE;
            case 0x01: return RAM_SIZE_2KB;
            case 0x02: return RAM_SIZE_8KB;
            case 0x03: return RAM_SIZE_32KB;
            case 0x04: return RAM_SIZE_128KB;
            case 0x05: return RAM_SIZE_64KB;
            default: return RAM_SIZE_NONE;
        }
    }
}

void Cartridge::parse_header() {
    // Minimum size for header fields (highest offset: 0x0149 RAM size)
    if (rom_.size() < MIN_HEADER_SIZE) return;

    // Extract game title from header - null-terminated ASCII string
    // Located at fixed offset per cartridge format specification
    char title_buf[TITLE_MAX_LENGTH + 1] = {0};
    for (int i = 0; i < TITLE_MAX_LENGTH; i++) {
        char c = rom_[HEADER_TITLE_OFFSET + i];
        if (c == 0) break;
        title_buf[i] = c;
    }
    title_ = title_buf;
    
    // CGB flag indicates Color Game Boy support
    // 0x80 = Game supports CGB functions (backward compatible with DMG)
    // 0xC0 = Game requires CGB hardware (won't work on DMG)
    cgb_flag_ = rom_[HEADER_CGB_FLAG_OFFSET];
    
    // Cartridge type determines MBC chip and capabilities
    cartridge_type_ = rom_[HEADER_CARTRIDGE_TYPE_OFFSET];
    
    // Battery flag determines if external RAM needs persistent storage
    // Check against all known cartridge types with battery support
    has_battery_ = (
        cartridge_type_ == CART_MBC1_RAM_BATTERY   ||
        cartridge_type_ == CART_MBC2_BATTERY        ||
        cartridge_type_ == CART_ROM_RAM_BATTERY     ||
        cartridge_type_ == CART_MMM01_RAM_BATTERY   ||
        cartridge_type_ == CART_MBC3_TIMER_BATTERY  ||
        cartridge_type_ == CART_MBC3_TIMER_RAM_BAT  ||
        cartridge_type_ == CART_MBC3_RAM_BATTERY    ||
        cartridge_type_ == CART_MBC5_RAM_BATTERY    ||
        cartridge_type_ == CART_MBC5_RUMBLE_RAM_BAT ||
        cartridge_type_ == CART_HUC1_RAM_BATTERY
    );
}

std::unique_ptr<Cartridge> Cartridge::load_rom_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open ROM file: " + path);
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<u8> rom_data(size);
    if (!file.read(reinterpret_cast<char*>(rom_data.data()), size)) {
        throw std::runtime_error("Failed to read ROM file");
    }
    
    // Check minimum size
    if (size < static_cast<std::streamsize>(MIN_HEADER_SIZE)) {
        throw std::runtime_error("ROM file too small (invalid header)");
    }
    
    // Read cartridge type and RAM size from header
    const u8 cartridge_type = rom_data[HEADER_CARTRIDGE_TYPE_OFFSET];
    const u8 ram_size_code = rom_data[HEADER_RAM_SIZE_OFFSET];
    const size_t ram_size = get_ram_size(ram_size_code);
    
    // Create appropriate cartridge type based on header
    switch (cartridge_type) {
        case CART_ROM_ONLY:
            return std::make_unique<ROMOnly>(std::move(rom_data));
            
        case CART_MBC1:
        case CART_MBC1_RAM:
        case CART_MBC1_RAM_BATTERY:
            return std::make_unique<MBC1>(std::move(rom_data), ram_size);
            
        case CART_MBC3_TIMER_BATTERY:
        case CART_MBC3_TIMER_RAM_BAT:
            return std::make_unique<MBC3>(std::move(rom_data), ram_size, true);
            
        case CART_MBC3:
        case CART_MBC3_RAM:
        case CART_MBC3_RAM_BATTERY:
            return std::make_unique<MBC3>(std::move(rom_data), ram_size, false);
            
        case CART_MBC5:
        case CART_MBC5_RAM:
        case CART_MBC5_RAM_BATTERY:
            return std::make_unique<MBC5>(std::move(rom_data), ram_size, false);
            
        case CART_MBC5_RUMBLE:
        case CART_MBC5_RUMBLE_RAM:
        case CART_MBC5_RUMBLE_RAM_BAT:
            return std::make_unique<MBC5>(std::move(rom_data), ram_size, true);
            
        default:
            throw std::runtime_error("Unsupported cartridge type: 0x" + 
                                    std::to_string(cartridge_type));
    }
}

const std::string& Cartridge::title() const
{
    return title_;
}

u8 Cartridge::cartridge_type() const
{
    return cartridge_type_;
}

bool Cartridge::has_battery() const
{
    return has_battery_;
}

bool Cartridge::is_cgb_supported() const
{
    // Check if CGB flag indicates CGB support
    // 0x80 = CGB supported (also works on DMG)
    // 0xC0 = CGB only
    return (cgb_flag_ == CGB_FLAG_SUPPORTED) || (cgb_flag_ == CGB_FLAG_ONLY);
}

bool Cartridge::is_cgb_only() const
{
    // Check if game requires CGB hardware
    return cgb_flag_ == CGB_FLAG_ONLY;
}

bool Cartridge::save_ram_to_file(const std::string& path) {
    // Only save if cartridge has battery-backed RAM
    if (!has_battery_ || ram_.empty()) {
        return false;
    }
    
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write RAM data to file
    if (!file.write(reinterpret_cast<const char*>(ram_.data()), ram_.size())) {
        return false;
    }
    
    return true;
}

bool Cartridge::load_ram_from_file(const std::string& path) {
    // Only load if cartridge has battery-backed RAM
    if (!has_battery_ || ram_.empty()) {
        return false;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        // File doesn't exist - not an error, just no save data yet
        return false;
    }
    
    // Read RAM data from file
    if (!file.read(reinterpret_cast<char*>(ram_.data()), ram_.size())) {
        // File exists but wrong size or read error
        return false;
    }
    
    return true;
}

// ROM-only cartridge implementation
ROMOnly::ROMOnly(std::vector<u8> rom_data)
    : Cartridge(std::move(rom_data), 0)
{
}

u8 ROMOnly::read(u16 address) const {
    if (address < 0x8000) {
        // ROM area
        if (address < rom_.size()) {
            return rom_[address];
        }
        return 0xFF;
    } else if (address >= 0xA000 && address < 0xC000) {
        // No external RAM in ROM-only cartridges
        return 0xFF;
    }
    return 0xFF;
}

void ROMOnly::write(u16 address, u8 value) {
    // ROM-only cartridges ignore writes
    (void)address;
    (void)value;
}

} // namespace gbglow
