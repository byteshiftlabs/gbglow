#include "cartridge.h"
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace emugbc {

void Cartridge::parse_header() {
    // Title is at 0x0134-0x0143 (16 bytes)
    char title_buf[17] = {0};
    for (int i = 0; i < 16; i++) {
        char c = rom_[0x0134 + i];
        if (c == 0) break;
        title_buf[i] = c;
    }
    title_ = title_buf;
    
    // Cartridge type at 0x0147
    cartridge_type_ = rom_[0x0147];
    
    // Check for battery
    has_battery_ = (cartridge_type_ == 0x03 || cartridge_type_ == 0x06 ||
                    cartridge_type_ == 0x09 || cartridge_type_ == 0x0D ||
                    cartridge_type_ == 0x0F || cartridge_type_ == 0x10 ||
                    cartridge_type_ == 0x13 || cartridge_type_ == 0x1B ||
                    cartridge_type_ == 0x1E || cartridge_type_ == 0xFF);
}

std::unique_ptr<Cartridge> Cartridge::load_from_file(const std::string& path) {
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
    if (size < 0x0150) {
        throw std::runtime_error("ROM file too small (invalid header)");
    }
    
    // Check cartridge type and create appropriate cartridge
    u8 cartridge_type = rom_data[0x0147];
    
    if (cartridge_type == 0x00) {
        // ROM only
        return std::make_unique<ROMOnly>(std::move(rom_data));
    } else {
        // TODO: Implement other cartridge types (MBC1, MBC3, MBC5, etc.)
        throw std::runtime_error("Cartridge type not yet implemented: " + 
                                std::to_string(cartridge_type));
    }
}

// ROM-only cartridge implementation
ROMOnly::ROMOnly(std::vector<u8> rom_data) {
    rom_ = std::move(rom_data);
    parse_header();
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

} // namespace emugbc
