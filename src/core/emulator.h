#pragma once

#include "types.h"
#include "cpu.h"
#include "memory.h"
#include "../video/ppu.h"
#include "../cartridge/cartridge.h"
#include <memory>
#include <string>

namespace emugbc {

/**
 * Main Emulator Class
 * 
 * Coordinates CPU, PPU, and other components
 */
class Emulator {
public:
    Emulator();
    
    // Load a ROM file
    bool load_rom(const std::string& path);
    
    // Run for one frame (~70224 cycles)
    void run_frame();
    
    // Run for a specific number of cycles
    void run_cycles(Cycles cycles);
    
    // Get components
    CPU& cpu() { return *cpu_; }
    PPU& ppu() { return *ppu_; }
    Memory& memory() { return *memory_; }
    
    // Reset emulator
    void reset();
    
private:
    std::unique_ptr<Memory> memory_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
};

} // namespace emugbc
