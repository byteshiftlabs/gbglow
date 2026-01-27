#include "memory.h"
#include "../cartridge/cartridge.h"
#include <cstring>

namespace emugbc {

Memory::Memory() 
    : interrupt_enable_(0) {
    // Initialize memory to zero
    vram_.fill(0);
    wram_.fill(0);
    oam_.fill(0);
    hram_.fill(0);
    io_regs_.fill(0);
}

Memory::~Memory() {
    // Destructor needs to be defined here where Cartridge is complete
}

void Memory::load_cartridge(std::unique_ptr<Cartridge> cart) {
    cartridge_ = std::move(cart);
}

u8 Memory::read(u16 address) const {
    // ROM: 0x0000-0x7FFF
    if (address < 0x8000) {
        if (cartridge_) {
            return cartridge_->read(address);
        }
        return 0xFF;  // Open bus
    }
    
    // VRAM: 0x8000-0x9FFF
    if (address < 0xA000) {
        return vram_[address - 0x8000];
    }
    
    // External RAM: 0xA000-0xBFFF
    if (address < 0xC000) {
        if (cartridge_) {
            return cartridge_->read(address);
        }
        return 0xFF;
    }
    
    // WRAM: 0xC000-0xDFFF
    if (address < 0xE000) {
        return wram_[address - 0xC000];
    }
    
    // Echo RAM: 0xE000-0xFDFF (mirror of 0xC000-0xDDFF)
    if (address < 0xFE00) {
        return wram_[address - 0xE000];
    }
    
    // OAM: 0xFE00-0xFE9F
    if (address < 0xFEA0) {
        return oam_[address - 0xFE00];
    }
    
    // Unusable: 0xFEA0-0xFEFF
    if (address < 0xFF00) {
        return 0xFF;
    }
    
    // I/O Registers: 0xFF00-0xFF7F
    if (address < 0xFF80) {
        return io_regs_[address - 0xFF00];
    }
    
    // HRAM: 0xFF80-0xFFFE
    if (address < 0xFFFF) {
        return hram_[address - 0xFF80];
    }
    
    // IE Register: 0xFFFF
    if (address == 0xFFFF) {
        return interrupt_enable_;
    }
    
    return 0xFF;
}

void Memory::write(u16 address, u8 value) {
    // ROM: 0x0000-0x7FFF (writes go to cartridge for MBC control)
    if (address < 0x8000) {
        if (cartridge_) {
            cartridge_->write(address, value);
        }
        return;
    }
    
    // VRAM: 0x8000-0x9FFF
    if (address < 0xA000) {
        vram_[address - 0x8000] = value;
        return;
    }
    
    // External RAM: 0xA000-0xBFFF
    if (address < 0xC000) {
        if (cartridge_) {
            cartridge_->write(address, value);
        }
        return;
    }
    
    // WRAM: 0xC000-0xDFFF
    if (address < 0xE000) {
        wram_[address - 0xC000] = value;
        return;
    }
    
    // Echo RAM: 0xE000-0xFDFF
    if (address < 0xFE00) {
        wram_[address - 0xE000] = value;
        return;
    }
    
    // OAM: 0xFE00-0xFE9F
    if (address < 0xFEA0) {
        oam_[address - 0xFE00] = value;
        return;
    }
    
    // Unusable: 0xFEA0-0xFEFF
    if (address < 0xFF00) {
        return;  // Writes ignored
    }
    
    // I/O Registers: 0xFF00-0xFF7F
    if (address < 0xFF80) {
        io_regs_[address - 0xFF00] = value;
        return;
    }
    
    // HRAM: 0xFF80-0xFFFE
    if (address < 0xFFFF) {
        hram_[address - 0xFF80] = value;
        return;
    }
    
    // IE Register: 0xFFFF
    if (address == 0xFFFF) {
        interrupt_enable_ = value;
    }
}

} // namespace emugbc
