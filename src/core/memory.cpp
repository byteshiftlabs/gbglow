#include "memory.h"

#include <cstring>
#include <fstream>

#include "../cartridge/cartridge.h"
#include "../input/joypad.h"
#include "../audio/apu.h"
#include "timer.h"

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
    constexpr u16 TIMER_DIV_REG = 0xFF04;
    constexpr u16 TIMER_TIMA_REG = 0xFF05;
    constexpr u16 TIMER_TMA_REG = 0xFF06;
    constexpr u16 TIMER_TAC_REG = 0xFF07;
    constexpr u16 BOOT_ROM_DISABLE_REG = 0xFF50;
    constexpr u16 BOOT_ROM_SIZE = 0x0100;
    
    // Sound registers
    constexpr u16 SOUND_NR52 = 0xFF26;  // Sound on/off
}

Memory::Memory() 
    : boot_rom_loaded_(false)
    , boot_rom_enabled_(false)
    , interrupt_enable_(0) {
    // Initialize memory to zero
    vram_.fill(0);
    wram_.fill(0);
    oam_.fill(0);
    hram_.fill(0);
    io_regs_.fill(0);
    boot_rom_.fill(0);
    
    // Initialize hardware registers to post-boot state
    // These values match what the boot ROM sets before jumping to cartridge
    
    // LCD Control (0xFF40) - LCD enabled, all features on
    io_regs_[0x40] = 0x91;  // LCDC: LCD on, BG on, window off
    
    // LCD Status (0xFF41) - Mode 1 (VBlank)
    io_regs_[0x41] = 0x85;
    
    // Scroll registers
    io_regs_[0x42] = 0x00;  // SCY
    io_regs_[0x43] = 0x00;  // SCX
    
    // LY register (scanline)
    io_regs_[0x44] = 0x00;
    
    // Background palette (0xFF47)
    io_regs_[0x47] = 0xFC;
    
    // Object palettes
    io_regs_[0x48] = 0xFF;  // OBP0
    io_regs_[0x49] = 0xFF;  // OBP1
    
    // Sound control register (NR52) - bit 7 = sound enabled
    io_regs_[SOUND_NR52 - IO_REGISTERS_START] = 0xF1;
    
    // Create joypad controller
    joypad_ = std::make_unique<Joypad>(*this);
    
    // Create timer system
    timer_ = std::make_unique<Timer>(*this);
    
    // Create audio system
    apu_ = std::make_unique<APU>(*this);
}

Memory::~Memory() {
    // Destructor needs to be defined here where Cartridge is complete
}

void Memory::load_cartridge(std::unique_ptr<Cartridge> cart) {
    cartridge_ = std::move(cart);
}

Cartridge* Memory::cartridge() {
    return cartridge_.get();
}

APU& Memory::apu() {
    return *apu_;
}

bool Memory::load_boot_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Read boot ROM (should be exactly 256 bytes)
    file.read(reinterpret_cast<char*>(boot_rom_.data()), BOOT_ROM_SIZE);
    if (!file || file.gcount() != BOOT_ROM_SIZE) {
        return false;
    }
    
    boot_rom_loaded_ = true;
    boot_rom_enabled_ = true;  // Enable boot ROM on load
    return true;
}

u8 Memory::read(u16 address) const {
    // Boot ROM overlay - takes priority over cartridge ROM when enabled
    if (boot_rom_enabled_ && boot_rom_loaded_ && address < BOOT_ROM_SIZE) {
        return boot_rom_[address];
    }
    
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
        
        // Timer registers - route through timer system
        if (address == TIMER_DIV_REG) {
            return timer_->read_div();
        }
        if (address == TIMER_TIMA_REG) {
            return timer_->read_tima();
        }
        if (address == TIMER_TMA_REG) {
            return timer_->read_tma();
        }
        if (address == TIMER_TAC_REG) {
            return timer_->read_tac();
        }
        // Audio registers - route through APU
        if (address >= 0xFF10 && address <= 0xFF3F) {
            return apu_->read_register(address);
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
        
        // Timer registers - route through timer system
        if (address == TIMER_DIV_REG) {
            timer_->write_div(value);
            return;
        }
        if (address == TIMER_TIMA_REG) {
            timer_->write_tima(value);
            return;
        }
        if (address == TIMER_TMA_REG) {
            timer_->write_tma(value);
            return;
        }
        if (address == TIMER_TAC_REG) {
            timer_->write_tac(value);
            return;
        }
        // Audio registers - route through APU
        if (address >= 0xFF10 && address <= 0xFF3F) {
            apu_->write_register(address, value);
            return;
        }
        
        // Boot ROM disable register - write any value to disable boot ROM
        if (address == BOOT_ROM_DISABLE_REG) {
            if (value != 0) {
                boot_rom_enabled_ = false;  // Permanently disable until reset
            }
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

Timer& Memory::timer()
{
    return *timer_;
}

} // namespace emugbc
