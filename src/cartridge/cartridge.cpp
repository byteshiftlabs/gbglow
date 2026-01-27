#include "cartridge.h"

#include <fstream>
#include <stdexcept>
#include <cstring>

#include "mbc1.h"

namespace emugbc {

// Cartridge header offsets per Game Boy cartridge specification
namespace {
    constexpr u16 HEADER_TITLE_OFFSET = 0x0134;
    constexpr u16 HEADER_CARTRIDGE_TYPE_OFFSET = 0x0147;
    constexpr int TITLE_MAX_LENGTH = 16;
}

void Cartridge::parse_header() {
    // Extract game title from header - null-terminated ASCII string
    // Located at fixed offset per cartridge format specification
    char title_buf[TITLE_MAX_LENGTH + 1] = {0};
    for (int i = 0; i < TITLE_MAX_LENGTH; i++) {
        char c = rom_[HEADER_TITLE_OFFSET + i];
        if (c == 0) break;
        title_buf[i] = c;
    }
    title_ = title_buf;
    
    // Cartridge type determines MBC chip and capabilities
    cartridge_type_ = rom_[HEADER_CARTRIDGE_TYPE_OFFSET];
    
    // Battery flag determines if external RAM needs persistent storage
    // Check against all known cartridge types with battery support
    has_battery_ = (
        cartridge_type_ == 0x03 ||  // MBC1+RAM+BATTERY
        cartridge_type_ == 0x06 ||  // MBC2+BATTERY
        cartridge_type_ == 0x09 ||  // ROM+RAM+BATTERY
        cartridge_type_ == 0x0D ||  // MMM01+RAM+BATTERY
        cartridge_type_ == 0x0F ||  // MBC3+TIMER+BATTERY
        cartridge_type_ == 0x10 ||  // MBC3+TIMER+RAM+BATTERY
        cartridge_type_ == 0x13 ||  // MBC3+RAM+BATTERY
        cartridge_type_ == 0x1B ||  // MBC5+RAM+BATTERY
        cartridge_type_ == 0x1E ||  // MBC5+RUMBLE+RAM+BATTERY
        cartridge_type_ == 0xFF     // HuC1+RAM+BATTERY
    );
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
