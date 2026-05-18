// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "emulator.h"
#include "constants.h"
#include "timer.h"
#include "io_registers.h"
#include "../audio/apu.h"
#include <iostream>
#include <fstream>
#include <cstdio>

namespace gbglow {

using namespace io_reg;
using namespace constants::savestate;

std::string Emulator::get_save_path() const
{
    // Replace .gb/.gbc extension with .sav
    std::string save_path = rom_path_;
    size_t dot_pos = save_path.rfind('.');
    if (dot_pos != std::string::npos) {
        save_path.resize(dot_pos);
    }
    save_path += ".sav";
    return save_path;
}

bool Emulator::save_state(int slot) {
    if (rom_path_.empty() || slot < kMinSlot || slot > kMaxSlot) {
        return false;
    }

    std::ofstream file(get_state_path(slot), std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(kHeader, static_cast<std::streamsize>(kHeaderLength));

    std::vector<u8> state_data;
    cpu_->serialize(state_data);
    memory_->serialize(state_data);
    ppu_->serialize(state_data);
    memory_->apu().serialize(state_data);
    memory_->timer().serialize(state_data);
    if (auto* cart = memory_->cartridge()) {
        cart->serialize(state_data);
    }

    // Serialise blob size as a 4-byte little-endian field so the format
    // is portable across 32-bit/64-bit targets and host endianness.
    const u32 data_size = static_cast<u32>(state_data.size());
    const u8 size_bytes[4] = {
        static_cast<u8>(data_size),
        static_cast<u8>(data_size >> 8),
        static_cast<u8>(data_size >> 16),
        static_cast<u8>(data_size >> 24)
    };
    file.write(reinterpret_cast<const char*>(size_bytes), sizeof(size_bytes));
    file.write(reinterpret_cast<const char*>(state_data.data()), data_size);

    return file.good();
}

bool Emulator::load_state(int slot) {
    if (rom_path_.empty() || slot < kMinSlot || slot > kMaxSlot) {
        return false;
    }

    std::ifstream file(get_state_path(slot), std::ios::binary);
    if (!file) {
        return false;
    }

    std::string header(kHeaderLength, '\0');
    file.read(header.data(), static_cast<std::streamsize>(kHeaderLength));
    if (header != kHeader) {
        std::cerr << "Corrupt save state: invalid header" << std::endl;
        return false;
    }

    // Read 4-byte little-endian blob size
    u8 size_bytes[4] = {};
    file.read(reinterpret_cast<char*>(size_bytes), sizeof(size_bytes));
    const u32 data_size =
        static_cast<u32>(size_bytes[0])         |
        (static_cast<u32>(size_bytes[1]) << 8)  |
        (static_cast<u32>(size_bytes[2]) << 16) |
        (static_cast<u32>(size_bytes[3]) << 24);
    if (data_size == 0 || data_size > kMaxBytes) {
        std::cerr << "Corrupt save state: invalid data size " << data_size << std::endl;
        return false;
    }

    std::vector<u8> state_data(data_size);
    file.read(reinterpret_cast<char*>(state_data.data()), data_size);
    if (file.fail()) {
        std::cerr << "Corrupt save state: truncated data" << std::endl;
        return false;
    }

    // Validate the blob size against the current component layout before
    // deserializing anything into the live emulator. This prevents partial
    // state mutation when a blob is truncated or built against a different
    // component layout.
    std::vector<u8> expected_state;
    cpu_->serialize(expected_state);
    memory_->serialize(expected_state);
    ppu_->serialize(expected_state);
    memory_->apu().serialize(expected_state);
    memory_->timer().serialize(expected_state);
    if (auto* cart = memory_->cartridge()) {
        cart->serialize(expected_state);
    }

    if (expected_state.size() != static_cast<size_t>(data_size)) {
        std::cerr << "Corrupt save state: component layout mismatch" << std::endl;
        return false;
    }

    size_t offset = 0;
    cpu_->deserialize(state_data.data(), data_size, offset);
    memory_->deserialize(state_data.data(), data_size, offset);
    ppu_->deserialize(state_data.data(), data_size, offset);
    memory_->apu().deserialize(state_data.data(), data_size, offset);
    memory_->timer().deserialize(state_data.data(), data_size, offset);
    if (auto* cart = memory_->cartridge()) {
        cart->deserialize(state_data.data(), data_size, offset);
    }

    // Each deserializer silently guards on size and returns early on underflow.
    // If offset did not advance to data_size the blob is corrupt or the build
    // layout changed — reject rather than running with half-initialised state.
    if (offset != static_cast<size_t>(data_size)) {
        std::cerr << "Corrupt save state: component data size mismatch" << std::endl;
        return false;
    }

    return true;
}

std::string Emulator::get_state_path(int slot) const {
    if (rom_path_.empty()) {
        return {};
    }

    std::string state_path = rom_path_;
    size_t dot_pos = state_path.rfind('.');
    if (dot_pos != std::string::npos) {
        state_path.resize(dot_pos);
    }
    state_path += ".slot" + std::to_string(slot + 1) + ".state";
    return state_path;
}

bool Emulator::delete_state(int slot) {
    if (rom_path_.empty() || slot < kMinSlot || slot > kMaxSlot) {
        return false;
    }
    return std::remove(get_state_path(slot).c_str()) == 0;
}

}  // namespace gbglow
