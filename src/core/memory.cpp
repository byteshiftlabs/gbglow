// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "memory.h"

#include <cstring>
#include <fstream>
#include <algorithm>

#include "../cartridge/cartridge.h"
#include "../input/joypad.h"
#include "../audio/apu.h"
#include "../video/ppu.h"
#include "timer.h"
#include "io_registers.h"

namespace gbglow {

using namespace io_reg;

// Memory map boundaries
namespace {
    constexpr u16 ROM_END = 0x8000;
    constexpr u16 VRAM_END = 0xA000;
    constexpr u16 EXTERNAL_RAM_START = 0xA000;
    constexpr u16 EXTERNAL_RAM_END = 0xC000;
    constexpr u16 ECHO_RAM_START = 0xE000;
    constexpr u16 ECHO_RAM_END = 0xFE00;
    constexpr u16 OAM_END = 0xFEA0;
    constexpr u16 PROHIBITED_START = 0xFEA0;
    constexpr u16 IO_REGISTERS_START = IO_START;  // alias for io_reg::IO_START
    constexpr u16 IO_REGISTERS_END = 0xFF80;
    constexpr u16 HRAM_END = 0xFFFF;
    constexpr u16 INTERRUPT_ENABLE_REG = 0xFFFF;
    constexpr u16 JOYPAD_REG = 0xFF00;
    constexpr u16 TIMER_DIV_REG = 0xFF04;
    constexpr u16 TIMER_TIMA_REG = 0xFF05;
    constexpr u16 TIMER_TMA_REG = 0xFF06;
    constexpr u16 TIMER_TAC_REG = 0xFF07;
    constexpr u16 BOOT_ROM_DISABLE_REG = 0xFF50;
    constexpr u16 BOOT_ROM_SIZE = 0x0100;
    
    // OAM DMA register and constants
    constexpr u16 OAM_DMA_REG = 0xFF46;
    constexpr u16 OAM_SIZE = 160;  // 40 sprites × 4 bytes each
    constexpr u8 OAM_DMA_SOURCE_SHIFT = 8;  // Source address = value << 8
    
    // Sound registers
    constexpr u16 SOUND_REG_START = 0xFF10;  // First audio register (NR10)
    constexpr u16 SOUND_REG_END = 0xFF3F;    // Last audio register (Wave RAM)
    constexpr u16 SOUND_NR52 = 0xFF26;       // Sound on/off
    
    // IO register offsets (relative to IO_REGISTERS_START)
    constexpr u8 IO_LCDC = 0x40;     // LCD Control
    constexpr u8 IO_STAT = 0x41;     // LCD Status
    constexpr u8 IO_SCY  = 0x42;     // Scroll Y
    constexpr u8 IO_SCX  = 0x43;     // Scroll X
    constexpr u8 IO_LY   = 0x44;     // LCD Y coordinate
    constexpr u8 IO_BGP  = 0x47;     // Background palette
    constexpr u8 IO_OBP0 = 0x48;     // Object palette 0
    constexpr u8 IO_OBP1 = 0x49;     // Object palette 1
    
    // Post-boot IO register init values
    constexpr u8 LCDC_INIT = 0x91;   // LCD on, BG on, window off
    constexpr u8 STAT_INIT = 0x85;   // Mode 1 (VBlank)
    constexpr u8 BGP_INIT  = 0xFC;   // Default background palette
    constexpr u8 OBP_INIT  = 0xFF;   // Default object palettes
    constexpr u8 NR52_INIT = 0xF1;   // Sound enabled
    
    // CGB registers
    constexpr u16 CGB_KEY1 = 0xFF4D;  // Speed switch register
    constexpr u16 CGB_VBK = 0xFF4F;   // VRAM bank register
    constexpr u16 CGB_BCPS = 0xFF68;  // Background palette specification
    constexpr u16 CGB_BCPD = 0xFF69;  // Background palette data
    constexpr u16 CGB_OCPS = 0xFF6A;  // Object palette specification
    constexpr u16 CGB_OCPD = 0xFF6B;  // Object palette data
    
    // CGB constants
    constexpr u16 VRAM_BANK_SIZE = 0x2000;  // 8KB per VRAM bank
    constexpr u8 VRAM_BANK_MASK = 0x01;     // Only bit 0 selects bank
    constexpr u8 CGB_SPEED_PREPARE_BIT = 0x01;  // KEY1 bit 0: prepare speed switch
    constexpr u8 CGB_SPEED_CURRENT_BIT = 0x80;  // KEY1 bit 7: current speed
    constexpr u8 CGB_VBK_READ_MASK = 0xFE;      // VBK bits 1-7 always read as 1
}

Memory::Memory() 
    : vram_bank_(0)
    , speed_switch_(0)
    , boot_rom_loaded_(false)
    , boot_rom_enabled_(false)
    , interrupt_enable_(0)
    , ppu_(nullptr) {
    // Initialize memory to zero
    vram_.fill(0);
    wram_.fill(0);
    oam_.fill(0);
    hram_.fill(0);
    io_regs_.fill(0);
    boot_rom_.fill(0);
    
    // Initialize hardware registers to post-boot state
    // These values match what the boot ROM sets before jumping to cartridge
    io_regs_[IO_LCDC] = LCDC_INIT;
    io_regs_[IO_STAT] = STAT_INIT;
    io_regs_[IO_BGP]  = BGP_INIT;
    io_regs_[IO_OBP0] = OBP_INIT;
    io_regs_[IO_OBP1] = OBP_INIT;
    io_regs_[SOUND_NR52 - IO_REGISTERS_START] = NR52_INIT;
    
    // Create joypad controller
    joypad_ = std::make_unique<Joypad>(*this);
    
    // Create timer system
    timer_ = std::make_unique<Timer>(*this);
    
    // Create audio system
    apu_ = std::make_unique<APU>(*this);
    
    // PPU pointer will be set later by Emulator
    ppu_ = nullptr;
}

Memory::~Memory() {
    // Destructor needs to be defined here where Cartridge is complete
}

void Memory::load_cartridge(std::unique_ptr<Cartridge> cartridge) {
    cartridge_ = std::move(cartridge);
    
    // Update PPU with cartridge pointer for CGB mode detection
    if (ppu_) {
        ppu_->set_cartridge(cartridge_.get());
    }
}

Cartridge* Memory::cartridge() {
    return cartridge_.get();
}

const Cartridge* Memory::cartridge() const {
    return cartridge_.get();
}

APU& Memory::apu() {
    return *apu_;
}

void Memory::set_ppu(PPU* ppu) {
    ppu_ = ppu;
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
    
    // Video RAM - accessed by PPU during rendering (CGB: banked)
    if (address < VRAM_END) {
        u16 offset = (vram_bank_ * VRAM_BANK_SIZE) + (address - VRAM_START);
        return vram_[offset];
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
        if (address >= SOUND_REG_START && address <= SOUND_REG_END) {
            return apu_->read_register(address);
        }
        
        // CGB Speed Switch register (KEY1)
        if (address == CGB_KEY1) {
            return speed_switch_;
        }
        
        // CGB VRAM Bank register (VBK)
        if (address == CGB_VBK) {
            return CGB_VBK_READ_MASK | vram_bank_;  // Bit 0 = bank, bits 1-7 always read as 1
        }
        
        // CGB Palette registers - route through PPU
        if (address == CGB_BCPS) {  // BCPS - Background Palette Specification
            return ppu_ ? ppu_->read_bcps() : 0xFF;
        }
        if (address == CGB_BCPD) {
            return ppu_ ? ppu_->read_bcpd() : 0xFF;
        }
        if (address == CGB_OCPS) {
            return ppu_ ? ppu_->read_ocps() : 0xFF;
        }
        if (address == CGB_OCPD) {
            return ppu_ ? ppu_->read_ocpd() : 0xFF;
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
    
    // Video RAM - updated by game, read by PPU (CGB: banked)
    if (address < VRAM_END) {
        u16 offset = (vram_bank_ * VRAM_BANK_SIZE) + (address - VRAM_START);
        vram_[offset] = value;
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
        
        // OAM DMA transfer register (0xFF46)
        // Writing to this register starts a DMA transfer from address (value * 0x100) to OAM
        // This copies 160 bytes (40 sprites × 4 bytes) to 0xFE00-0xFE9F
        if (address == OAM_DMA_REG) {
            u16 source = static_cast<u16>(value) << OAM_DMA_SOURCE_SHIFT;
            
            for (u16 i = 0; i < OAM_SIZE; i++) {
                oam_[i] = read(source + i);
            }
            io_regs_[address - IO_REGISTERS_START] = value;
            return;
        }
        
        // Audio registers - route through APU
        if (address >= SOUND_REG_START && address <= SOUND_REG_END) {
            apu_->write_register(address, value);
            return;
        }
        
        // CGB Speed Switch register (KEY1)
        if (address == CGB_KEY1) {
            // Bit 0: Prepare switch (writable)
            // Bit 7: Current speed (read-only, toggled by STOP instruction)
            speed_switch_ = (speed_switch_ & CGB_SPEED_CURRENT_BIT) | (value & CGB_SPEED_PREPARE_BIT);
            return;
        }
        
        // CGB VRAM Bank register (VBK)
        if (address == CGB_VBK) {
            vram_bank_ = value & VRAM_BANK_MASK;  // Only bit 0 is used
            return;
        }
        
        // CGB Palette registers - route through PPU
        if (address == CGB_BCPS) {  // BCPS - Background Palette Specification
            if (ppu_) ppu_->write_bcps(value);
            return;
        }
        if (address == CGB_BCPD) {
            if (ppu_) ppu_->write_bcpd(value);
            return;
        }
        if (address == CGB_OCPS) {
            if (ppu_) ppu_->write_ocps(value);
            return;
        }
        if (address == CGB_OCPD) {
            if (ppu_) ppu_->write_ocpd(value);
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

// ============================================================================
// Serialization for Save States
// ============================================================================

namespace {
    // Helper to serialize an array
    template<typename T, size_t N>
    void serialize_array(const std::array<T, N>& arr, std::vector<u8>& data) {
        data.insert(data.end(), arr.begin(), arr.end());
    }
    
    // Helper to deserialize an array
    template<typename T, size_t N>
    void deserialize_array(std::array<T, N>& arr, const u8* data, size_t& offset) {
        std::copy(data + offset, data + offset + N, arr.begin());
        offset += N;
    }
}

void Memory::serialize(std::vector<u8>& data) const
{
    // VRAM (16KB for CGB)
    serialize_array(vram_, data);
    
    // VRAM bank selector
    data.push_back(vram_bank_);
    
    // Speed switch register
    data.push_back(speed_switch_);
    
    // WRAM (8KB)
    serialize_array(wram_, data);
    
    // OAM (160 bytes)
    serialize_array(oam_, data);
    
    // HRAM (128 bytes)
    serialize_array(hram_, data);
    
    // IO registers (128 bytes)
    serialize_array(io_regs_, data);
    
    // Boot ROM state
    data.push_back(boot_rom_enabled_ ? 1 : 0);
    
    // Interrupt enable register
    data.push_back(interrupt_enable_);
}

void Memory::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    // Component sizes matching serialize() output
    constexpr size_t VRAM_BYTES = 16384;   // vram_ array (CGB: 2 banks x 8KB)
    constexpr size_t VRAM_BANK_BYTE = 1;   // vram_bank_ selector
    constexpr size_t SPEED_SWITCH_BYTE = 1; // speed_switch_ register
    constexpr size_t WRAM_BYTES = 8192;    // wram_ array
    constexpr size_t OAM_BYTES = 160;      // oam_ array
    constexpr size_t HRAM_BYTES = 128;     // hram_ array
    constexpr size_t IO_BYTES = 128;       // io_regs_ array
    constexpr size_t BOOT_ROM_BYTE = 1;    // boot_rom_enabled_ flag
    constexpr size_t IE_BYTE = 1;          // interrupt_enable_ register
    constexpr size_t MEMORY_STATE_SIZE = VRAM_BYTES + VRAM_BANK_BYTE + SPEED_SWITCH_BYTE
        + WRAM_BYTES + OAM_BYTES + HRAM_BYTES + IO_BYTES + BOOT_ROM_BYTE + IE_BYTE;
    if (offset + MEMORY_STATE_SIZE > data_size) return;

    // VRAM
    deserialize_array(vram_, data, offset);
    
    // VRAM bank selector
    vram_bank_ = data[offset++];
    
    // Speed switch register
    speed_switch_ = data[offset++];
    
    // WRAM
    deserialize_array(wram_, data, offset);
    
    // OAM
    deserialize_array(oam_, data, offset);
    
    // HRAM
    deserialize_array(hram_, data, offset);
    
    // IO registers
    deserialize_array(io_regs_, data, offset);
    
    // Boot ROM state
    boot_rom_enabled_ = data[offset++] != 0;
    
    // Interrupt enable register
    interrupt_enable_ = data[offset++];
}

} // namespace gbglow
