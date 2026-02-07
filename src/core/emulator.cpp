#include "emulator.h"
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>

namespace emugbc {

Emulator::Emulator() {
    memory_ = std::make_unique<Memory>();
    cpu_ = std::make_unique<CPU>(*memory_);
    ppu_ = std::make_unique<PPU>(*memory_);
    joypad_ = std::make_unique<Joypad>(*memory_);
}

bool Emulator::load_rom(const std::string& path) {
    try {
        auto cartridge = Cartridge::load_rom_from_file(path);
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
    run_cycles(CYCLES_PER_FRAME);
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

void Emulator::run(const std::string& window_title, int scale_factor) {
    // Initialize display
    Display display;
    if (!display.initialize(window_title, scale_factor)) {
        std::cerr << "Failed to initialize display" << std::endl;
        return;
    }
    
    std::cout << "Emulator running. Press ESC to exit." << std::endl;
    std::cout << "Controls: Arrow keys = D-pad, Z = A, X = B, Enter = Start, Shift = Select" << std::endl;
    
    // Main game loop
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double, std::milli>;
    
    auto last_frame_time = Clock::now();
    
    while (!display.should_close()) {
        auto frame_start = Clock::now();
        
        // Handle input events
        display.poll_events(joypad_.get());
        
        // Run one frame of emulation
        run_frame();
        
        // Update display if frame is ready
        if (ppu_->frame_ready()) {
            const auto& framebuffer = ppu_->get_rgba_framebuffer();
            display.update(framebuffer);
            ppu_->clear_frame_ready();
        }
        
        // Frame timing - maintain ~59.73 Hz
        auto frame_end = Clock::now();
        Duration frame_duration = frame_end - frame_start;
        
        // Sleep if we finished early
        double sleep_time = FRAME_TIME_MS - frame_duration.count();
        if (sleep_time > 0.0) {
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(sleep_time));
        }
        
        last_frame_time = Clock::now();
    }
    
    std::cout << "Emulator stopped." << std::endl;
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

Joypad& Emulator::joypad()
{
    return *joypad_;
}

} // namespace emugbc
