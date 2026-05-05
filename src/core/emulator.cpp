// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "emulator.h"
#include "../audio/apu.h"
#include <stdexcept>
#include <iostream>

namespace gbglow {

Emulator::Emulator()
    : memory_(std::make_unique<Memory>())
    , cpu_(std::make_unique<CPU>(*memory_))
    , ppu_(std::make_unique<PPU>(*memory_))
    , debugger_(std::make_unique<Debugger>())
    , recent_roms_(std::make_unique<RecentRoms>())
{
    memory_->set_ppu(ppu_.get());
    
    // Attach debugger to CPU, Memory, and PPU
    debugger_->attach(cpu_.get(), memory_.get(), ppu_.get());
    
    // Note: Joypad is owned by Memory, not Emulator
    
    // Try to load boot ROM (optional - emulator works without it)
    // Common locations: dmg_boot.bin, boot.gb, etc.
    if (memory_->load_boot_rom("dmg_boot.bin")) {
        std::cout << "Boot ROM loaded" << std::endl;
    } else if (memory_->load_boot_rom("boot.gb")) {
        std::cout << "Boot ROM loaded" << std::endl;
    } else {
        std::cout << "No boot ROM found (emulator will skip boot sequence)" << std::endl;
    }
}

bool Emulator::load_rom(const std::string& path) {
    try {
        auto loaded_cartridge = Cartridge::load_rom_from_file(path);
        memory_->load_cartridge(std::move(loaded_cartridge));
        rom_path_ = path;
        
        // Add to recent ROMs list
        recent_roms_->add_rom(path);
        
        // Load save file if it exists
        std::string save_path = get_save_path();
        if (auto* saved_cartridge = memory_->cartridge()) {
            if (saved_cartridge->load_ram_from_file(save_path)) {
                std::cout << "Loaded save file: " << save_path << std::endl;
            }
        }
        
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
        // Execute one CPU instruction (step() handles interrupts internally)
        Cycles cpu_cycles = cpu_->step();
        
        // Run PPU for the same number of cycles
        ppu_->step(cpu_cycles);
        
        // Run APU for audio generation
        memory_->apu().step(cpu_cycles);
        
        remaining = (remaining > cpu_cycles) ? (remaining - cpu_cycles) : 0;
    }
}

Debugger& Emulator::debugger() {
    return *debugger_;
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
    return memory_->joypad();
}

Cartridge* Emulator::cartridge()
{
    return memory_->cartridge();
}

Memory& Emulator::memory()
{
    return *memory_;
}

RecentRoms& Emulator::recent_roms() {
    return *recent_roms_;
}

}  // namespace gbglow
