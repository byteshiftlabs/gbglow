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
    if (size < 0x0150) {
        throw std::runtime_error("ROM file too small (invalid header)");
    }
    
    // Read cartridge type and RAM size from header
    const u8 cartridge_type = rom_data[HEADER_CARTRIDGE_TYPE_OFFSET];
    const u8 ram_size_code = rom_data[HEADER_RAM_SIZE_OFFSET];
    const size_t ram_size = get_ram_size(ram_size_code);
    
    // Create appropriate cartridge type based on header
    switch (cartridge_type) {
        case 0x00: // ROM ONLY
            return std::make_unique<ROMOnly>(std::move(rom_data));
            
        case 0x01: // MBC1
        case 0x02: // MBC1+RAM
        case 0x03: // MBC1+RAM+BATTERY
            return std::make_unique<MBC1>(std::move(rom_data), ram_size);
            
        case 0x0F: // MBC3+TIMER+BATTERY
        case 0x10: // MBC3+TIMER+RAM+BATTERY
            return std::make_unique<MBC3>(std::move(rom_data), ram_size, true);
            
        case 0x11: // MBC3
        case 0x12: // MBC3+RAM
        case 0x13: // MBC3+RAM+BATTERY
            return std::make_unique<MBC3>(std::move(rom_data), ram_size, false);
            
        case 0x19: // MBC5
        case 0x1A: // MBC5+RAM
        case 0x1B: // MBC5+RAM+BATTERY
            return std::make_unique<MBC5>(std::move(rom_data), ram_size, false);
            
        case 0x1C: // MBC5+RUMBLE
        case 0x1D: // MBC5+RUMBLE+RAM
        case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
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
