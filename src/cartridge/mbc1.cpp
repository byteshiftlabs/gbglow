#include "mbc1.h"

namespace gbcrush {

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
        return rom_bank_ | (ram_bank_ << 5);
    }
}

u8 MBC1::read(u16 address) const {
    if (address < 0x4000) {
        // ROM Bank 0 (or bank 0x00/0x20/0x40/0x60 in RAM banking mode)
        size_t bank = banking_mode_ ? (ram_bank_ << 5) : 0;
        size_t offset = (bank * 0x4000) + address;
        if (offset < rom_.size()) {
            return rom_[offset];
        }
        return 0xFF;
    }
    else if (address < 0x8000) {
        // ROM Bank 1-N (switchable)
        size_t bank = get_rom_bank();
        size_t offset = (bank * 0x4000) + (address - 0x4000);
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
        
        size_t bank = banking_mode_ ? ram_bank_ : 0;
        size_t offset = (bank * 0x2000) + (address - 0xA000);
        if (offset < ram_.size()) {
            return ram_[offset];
        }
        return 0xFF;
    }
    
    return 0xFF;
}

void MBC1::write(u16 address, u8 value) {
    if (address < 0x2000) {
        // RAM Enable
        ram_enabled_ = (value & 0x0F) == 0x0A;
    }
    else if (address < 0x4000) {
        // ROM Bank Number (lower 5 bits)
        rom_bank_ = value & 0x1F;
        if (rom_bank_ == 0) {
            rom_bank_ = 1;  // Bank 0 not accessible here
        }
    }
    else if (address < 0x6000) {
        // RAM Bank Number / Upper ROM Bank bits
        ram_bank_ = value & 0x03;
    }
    else if (address < 0x8000) {
        // Banking Mode Select
        banking_mode_ = (value & 0x01) != 0;
    }
    else if (address >= 0xA000 && address < 0xC000) {
        // External RAM write
        if (ram_enabled_ && !ram_.empty()) {
            size_t bank = banking_mode_ ? ram_bank_ : 0;
            size_t offset = (bank * 0x2000) + (address - 0xA000);
            if (offset < ram_.size()) {
                ram_[offset] = value;
            }
        }
    }
}

} // namespace gbcrush
