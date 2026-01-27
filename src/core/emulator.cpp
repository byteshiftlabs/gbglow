#include "emulator.h"
#include <stdexcept>

namespace emugbc {

Emulator::Emulator() {
    memory_ = std::make_unique<Memory>();
    cpu_ = std::make_unique<CPU>(*memory_);
    ppu_ = std::make_unique<PPU>(*memory_);
}

bool Emulator::load_rom(const std::string& path) {
    try {
        auto cartridge = Cartridge::load_from_file(path);
        memory_->load_cartridge(std::move(cartridge));
        reset();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void Emulator::reset() {
    cpu_->reset();
}

void Emulator::run_frame() {
    // One frame = ~70224 cycles (59.7 fps)
    // 154 scanlines × 456 dots = 70224 cycles
    run_cycles(70224);
}

void Emulator::run_cycles(Cycles cycles) {
    Cycles remaining = cycles;
    
    while (remaining > 0) {
        // Execute one CPU instruction
        Cycles cpu_cycles = cpu_->step();
        
        // Handle interrupts
        cpu_->handle_interrupts();
        
        // Run PPU for the same number of cycles
        ppu_->step(cpu_cycles);
        
        remaining = (remaining > cpu_cycles) ? (remaining - cpu_cycles) : 0;
    }
}

const PPU& Emulator::ppu() const
{
    return *ppu_;
}

PPU& Emulator::ppu()
{
    return *ppu_;
}

const CPU& Emulator::cpu() const
{
    return *cpu_;
}

CPU& Emulator::cpu()
{
    return *cpu_;
}

} // namespace emugbc
