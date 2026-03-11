// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "emulator.h"
#include "timer.h"
#include <iostream>
#include <fstream>
#include <cstdio>

namespace gbglow {

namespace {
    constexpr u16 WRAM_START = 0xC000;
    constexpr u16 WRAM_END   = 0xE000;
} // anonymous namespace

std::string Emulator::get_save_path() const
{
    // Replace .gb, .gbc extension with .sav
    std::string save_path = rom_path_;
    
    // Find last dot
    size_t dot_pos = save_path.rfind('.');
    if (dot_pos != std::string::npos) {
        save_path.resize(dot_pos);
    }
    
    save_path += ".sav";
    return save_path;
}

bool Emulator::save_state(int slot) {
    if (slot < 0 || slot > 9) {
        return false;
    }
    
    std::string state_path = get_state_path(slot);
    
    std::ofstream file(state_path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write header with version (no null terminator, so load reads exact same bytes)
    const char header[] = "GBGLOW_STATE_V2";
    file.write(header, sizeof(header) - 1);
    
    // Save CPU state (registers)
    const auto& regs = cpu_->registers();
    file.write(reinterpret_cast<const char*>(&regs.a), sizeof(regs.a));
    file.write(reinterpret_cast<const char*>(&regs.f), sizeof(regs.f));
    file.write(reinterpret_cast<const char*>(&regs.b), sizeof(regs.b));
    file.write(reinterpret_cast<const char*>(&regs.c), sizeof(regs.c));
    file.write(reinterpret_cast<const char*>(&regs.d), sizeof(regs.d));
    file.write(reinterpret_cast<const char*>(&regs.e), sizeof(regs.e));
    file.write(reinterpret_cast<const char*>(&regs.h), sizeof(regs.h));
    file.write(reinterpret_cast<const char*>(&regs.l), sizeof(regs.l));
    file.write(reinterpret_cast<const char*>(&regs.sp), sizeof(regs.sp));
    file.write(reinterpret_cast<const char*>(&regs.pc), sizeof(regs.pc));
    
    // Save CPU flags
    bool ime = cpu_->ime();
    bool halted = cpu_->is_halted();
    file.write(reinterpret_cast<const char*>(&ime), sizeof(ime));
    file.write(reinterpret_cast<const char*>(&halted), sizeof(halted));
    
    // Save memory state using Memory's serialize method
    std::vector<u8> mem_data;
    memory_->serialize(mem_data);
    size_t mem_size = mem_data.size();
    file.write(reinterpret_cast<const char*>(&mem_size), sizeof(mem_size));
    file.write(reinterpret_cast<const char*>(mem_data.data()), mem_data.size());
    
    // Save PPU state
    u8 ly = ppu_->scanline();
    u8 mode = static_cast<u8>(ppu_->mode());
    file.write(reinterpret_cast<const char*>(&ly), sizeof(ly));
    file.write(reinterpret_cast<const char*>(&mode), sizeof(mode));
    
    // Save Timer state
    const Timer& timer = memory_->timer();
    u8 div = timer.read_div();
    u8 tima = timer.read_tima();
    u8 tma = timer.read_tma();
    u8 tac = timer.read_tac();
    file.write(reinterpret_cast<const char*>(&div), sizeof(div));
    file.write(reinterpret_cast<const char*>(&tima), sizeof(tima));
    file.write(reinterpret_cast<const char*>(&tma), sizeof(tma));
    file.write(reinterpret_cast<const char*>(&tac), sizeof(tac));
    
    file.close();
    return true;
}

bool Emulator::load_state(int slot) {
    if (slot < 0 || slot > 9) {
        return false;
    }
    
    std::string state_path = get_state_path(slot);
    
    std::ifstream file(state_path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Verify header - support GBGLOW V1/V2 and legacy GBCRUSH V1/V2
    // New format: 15 bytes, no null terminator ("GBGLOW_STATE_V2")
    // Old format: 16 bytes + 1 trailing null written by buggy code ("GBCRUSH_STATE_V2\0")
    char header[18] = {0};
    file.read(header, 15);
    std::string header_str(header, 15);

    if (header_str != "GBGLOW_STATE_V2" && header_str != "GBGLOW_STATE_V1") {
        // Try reading 1 more byte to complete a potential 16-char GBCRUSH magic
        char extra = 0;
        file.read(&extra, 1);
        header_str += extra;

        if (header_str == "GBCRUSH_STATE_V2") {
            file.read(&extra, 1); // consume the stray null from old buggy write
            header_str = "GBGLOW_STATE_V2";
        } else if (header_str == "GBCRUSH_STATE_V1") {
            file.read(&extra, 1); // consume the stray null
            header_str = "GBGLOW_STATE_V1";
        } else {
            file.close();
            return false; // Unrecognized format
        }
    }

    if (header_str == "GBGLOW_STATE_V2") {
        // Load V2 format with full state
        
        // Load CPU state
        auto& regs = cpu_->registers();
        file.read(reinterpret_cast<char*>(&regs.a), sizeof(regs.a));
        file.read(reinterpret_cast<char*>(&regs.f), sizeof(regs.f));
        file.read(reinterpret_cast<char*>(&regs.b), sizeof(regs.b));
        file.read(reinterpret_cast<char*>(&regs.c), sizeof(regs.c));
        file.read(reinterpret_cast<char*>(&regs.d), sizeof(regs.d));
        file.read(reinterpret_cast<char*>(&regs.e), sizeof(regs.e));
        file.read(reinterpret_cast<char*>(&regs.h), sizeof(regs.h));
        file.read(reinterpret_cast<char*>(&regs.l), sizeof(regs.l));
        file.read(reinterpret_cast<char*>(&regs.sp), sizeof(regs.sp));
        file.read(reinterpret_cast<char*>(&regs.pc), sizeof(regs.pc));
        
        // Load CPU flags
        bool ime, halted;
        file.read(reinterpret_cast<char*>(&ime), sizeof(ime));
        file.read(reinterpret_cast<char*>(&halted), sizeof(halted));
        cpu_->set_ime(ime);
        cpu_->set_halted(halted);
        
        // Load memory state
        size_t mem_size;
        file.read(reinterpret_cast<char*>(&mem_size), sizeof(mem_size));
        
        // Validate memory size (expected size is ~25KB for Memory serialization)
        // VRAM(16384) + bank(1) + speed(1) + WRAM(8192) + OAM(160) + HRAM(128) + IO(128) + boot(1) + IE(1)
        constexpr size_t EXPECTED_MEM_SIZE = 16384 + 1 + 1 + 8192 + 160 + 128 + 128 + 1 + 1;
        constexpr size_t MAX_VALID_SIZE = EXPECTED_MEM_SIZE * 2;  // Allow some margin
        if (mem_size == 0 || mem_size > MAX_VALID_SIZE) {
            std::cerr << "Corrupt save state: invalid memory size " << mem_size << std::endl;
            file.close();
            return false;
        }
        
        std::vector<u8> mem_data(mem_size);
        file.read(reinterpret_cast<char*>(mem_data.data()), mem_size);
        
        // Verify we read enough data
        if (!file.good()) {
            std::cerr << "Corrupt save state: truncated memory data" << std::endl;
            file.close();
            return false;
        }
        
        size_t offset = 0;
        memory_->deserialize(mem_data.data(), mem_data.size(), offset);
        
        // Load PPU state
        u8 ly, mode;
        file.read(reinterpret_cast<char*>(&ly), sizeof(ly));
        file.read(reinterpret_cast<char*>(&mode), sizeof(mode));
        // Note: PPU state restoration would require setters in PPU class
        // For now, we rely on memory state which includes LCD registers
        
        // Load Timer state
        u8 div, tima, tma, tac;
        file.read(reinterpret_cast<char*>(&div), sizeof(div));
        file.read(reinterpret_cast<char*>(&tima), sizeof(tima));
        file.read(reinterpret_cast<char*>(&tma), sizeof(tma));
        file.read(reinterpret_cast<char*>(&tac), sizeof(tac));
        
        Timer& timer = memory_->timer();
        timer.write_div(div);
        timer.write_tima(tima);
        timer.write_tma(tma);
        timer.write_tac(tac);
        
    } else if (header_str == "GBGLOW_STATE_V1") {
        // Load V1 format (legacy - incomplete state)
        auto& regs = cpu_->registers();
        file.read(reinterpret_cast<char*>(&regs.a), sizeof(regs.a));
        file.read(reinterpret_cast<char*>(&regs.f), sizeof(regs.f));
        file.read(reinterpret_cast<char*>(&regs.b), sizeof(regs.b));
        file.read(reinterpret_cast<char*>(&regs.c), sizeof(regs.c));
        file.read(reinterpret_cast<char*>(&regs.d), sizeof(regs.d));
        file.read(reinterpret_cast<char*>(&regs.e), sizeof(regs.e));
        file.read(reinterpret_cast<char*>(&regs.h), sizeof(regs.h));
        file.read(reinterpret_cast<char*>(&regs.l), sizeof(regs.l));
        file.read(reinterpret_cast<char*>(&regs.sp), sizeof(regs.sp));
        file.read(reinterpret_cast<char*>(&regs.pc), sizeof(regs.pc));
        
        // Load partial memory state (work RAM only)
        auto& mem = memory();
        for (u16 addr = WRAM_START; addr < WRAM_END; addr++) {
            u8 byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            mem.write(addr, byte);
        }
    } else {
        file.close();
        return false;  // Unrecognized format
    }
    
    file.close();
    return true;
}

std::string Emulator::get_state_path(int slot) const {
    std::string state_path = rom_path_;
    
    // Remove extension
    size_t dot_pos = state_path.rfind('.');
    if (dot_pos != std::string::npos) {
        state_path.resize(dot_pos);
    }
    
    // Add slot number and .state extension
    state_path += ".slot" + std::to_string(slot + 1) + ".state";
    return state_path;
}

bool Emulator::delete_state(int slot) {
    if (slot < 0 || slot > 9) {
        return false;
    }
    
    std::string state_path = get_state_path(slot);
    
    // Remove the file
    if (std::remove(state_path.c_str()) == 0) {
        return true;
    }
    
    return false;  // File doesn't exist or couldn't be deleted
}

}  // namespace gbglow
