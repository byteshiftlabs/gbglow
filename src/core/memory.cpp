#include "memory.h"

#include <cstring>

#include "../cartridge/cartridge.h"
#include "../input/joypad.h"

namespace emugbc {

// Memory map boundaries
namespace {
    constexpr u16 ROM_END = 0x8000;
    constexpr u16 VRAM_START = 0x8000;
    constexpr u16 VRAM_END = 0xA000;
    constexpr u16 EXTERNAL_RAM_START = 0xA000;
    constexpr u16 EXTERNAL_RAM_END = 0xC000;
    constexpr u16 WRAM_START = 0xC000;
    constexpr u16 WRAM_END = 0xE000;
    constexpr u16 ECHO_RAM_START = 0xE000;
    constexpr u16 ECHO_RAM_END = 0xFE00;
    constexpr u16 OAM_START = 0xFE00;
    constexpr u16 OAM_END = 0xFEA0;
    constexpr u16 PROHIBITED_START = 0xFEA0;
    constexpr u16 IO_REGISTERS_START = 0xFF00;
    constexpr u16 IO_REGISTERS_END = 0xFF80;
    constexpr u16 HRAM_START = 0xFF80;
    constexpr u16 HRAM_END = 0xFFFF;
    constexpr u16 INTERRUPT_ENABLE_REG = 0xFFFF;
    constexpr u16 JOYPAD_REG = 0xFF00;
}

Memory::Memory() 
    : interrupt_enable_(0) {
    // Initialize memory to zero
    vram_.fill(0);
    wram_.fill(0);
    oam_.fill(0);
    hram_.fill(0);
    io_regs_.fill(0);
    
    // Create joypad controller
    joypad_ = std::make_unique<Joypad>(*this);
}

Memory::~Memory() {
    // Destructor needs to be defined here where Cartridge is complete
}

void Memory::load_cartridge(std::unique_ptr<Cartridge> cart) {
    cartridge_ = std::move(cart);
}

u8 Memory::read(u16 address) const {
    // Cartridge ROM space - delegate to MBC for bank switching
    if (address < ROM_END) {
        if (cartridge_) {
            return cartridge_->read(address);
        }
        return 0xFF;  // Open bus behavior when no cartridge
    }
    
    // Video RAM - accessed by PPU during rendering
    if (address < VRAM_END) {
        return vram_[address - VRAM_START];
    }
    
    // External RAM - cartridge SRAM/battery-backed save data
    if (address < EXTERNAL_RAM_END) {
        if (cartridge_) {
            return cartridge_->read(address);
        }
        return 0xFF;
    }
    
    // Work RAM - main system memory
    if (address < WRAM_END) {
        return wram_[address - WRAM_START];
    }
    
    // Echo RAM - hardware mirrors WRAM for compatibility
    if (address < ECHO_RAM_END) {
        return wram_[address - ECHO_RAM_START];
    }
    
    // Object Attribute Memory - sprite data for PPU
    if (address < OAM_END) {
        return oam_[address - OAM_START];
    }
    
    // Prohibited region - hardware doesn't decode this range
    if (address < IO_REGISTERS_START) {
        return 0xFF;
    }
    
    // I/O registers - hardware device control
    if (address < IO_REGISTERS_END) {
        // Joypad register - route through joypad controller
        if (address == JOYPAD_REG) {
            return joypad_->read_register();
        }
        return io_regs_[address - IO_REGISTERS_START];
    }
    
    // High RAM - fast zero-page memory
    if (address < HRAM_END) {
        return hram_[address - HRAM_START];
    }
    
    // Interrupt Enable register - separate from HRAM
    if (address == INTERRUPT_ENABLE_REG) {
        return interrupt_enable_;
    }
    
    return 0xFF;
}

void Memory::write(u16 address, u8 value) {
    // ROM space writes - MBC uses these for bank switching control
    if (address < ROM_END) {
        if (cartridge_) {
            cartridge_->write(address, value);
        }
        return;
    }
    
    // Video RAM - updated by game, read by PPU
    if (address < VRAM_END) {
        vram_[address - VRAM_START] = value;
        return;
    }
    
    // External RAM - cartridge SRAM for save data
    if (address < EXTERNAL_RAM_END) {
        if (cartridge_) {
            cartridge_->write(address, value);
        }
        return;
    }
    
    // Work RAM - main system memory
    if (address < WRAM_END) {
        wram_[address - WRAM_START] = value;
        return;
    }
    
    // Echo RAM - writes mirror to WRAM
    if (address < ECHO_RAM_END) {
        wram_[address - ECHO_RAM_START] = value;
        return;
    }
    
    // Object Attribute Memory - sprite data
    if (address < OAM_END) {
        oam_[address - OAM_START] = value;
        return;
    }
    
    // Prohibited region - writes are ignored
    if (address < IO_REGISTERS_START) {
        return;  // Writes ignored
    }
    
    // I/O registers - hardware device control
    if (address < IO_REGISTERS_END) {
        // Joypad register - route through joypad controller
        if (address == JOYPAD_REG) {
            joypad_->write_register(value);
            return;
        }
        io_regs_[address - IO_REGISTERS_START] = value;
        return;
    }
    
    // High RAM - fast zero-page memory
    if (address < HRAM_END) {
        hram_[address - HRAM_START] = value;
        return;
    }
    
    // Interrupt Enable register - separate from HRAM
    if (address == INTERRUPT_ENABLE_REG) {
        interrupt_enable_ = value;
    }
}

u16 Memory::read16(u16 address) const
{
    // Game Boy is little-endian: low byte first, then high byte
    u8 low = read(address);
    u8 high = read(address + 1);
    return (static_cast<u16>(high) << 8) | low;
}

void Memory::write16(u16 address, u16 value)
{
    // Write low byte first (little-endian)
    write(address, value & 0xFF);
    write(address + 1, value >> 8);
}

Joypad& Memory::joypad()
{
    return *joypad_;
}

} // namespace emugbc
