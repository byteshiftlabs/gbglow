// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "../src/core/cpu.h"
#include "../src/core/constants.h"
#include "../src/core/emulator.h"
#include "../src/core/io_registers.h"
#include "../src/core/memory.h"
#include "../src/core/registers.h"
#include "../src/core/timer.h"
#include "../src/audio/apu.h"
#include "../src/cartridge/cartridge.h"
#include "../src/cartridge/mbc1.h"
#include "../src/cartridge/mbc3.h"
#include "../src/cartridge/mbc5.h"
#include "../src/debug/debugger.h"
#include "../src/debug/debugger_gui.h"
#include "../src/input/gamepad.h"
#include "../src/ui/recent_roms.h"
#include "../src/video/ppu.h"

#include <SDL2/SDL.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace gbglow;

namespace test_support {

inline int tests_run = 0;
inline int tests_passed = 0;

#define TEST_ASSERT(cond)                                                   \
    do {                                                                    \
        ++test_support::tests_run;                                          \
        if (!(cond)) {                                                      \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__        \
                      << ": " << #cond << "\n";                           \
            return false;                                                   \
        }                                                                   \
        ++test_support::tests_passed;                                       \
    } while (0)

#define TEST_EQ(actual, expected)                                           \
    do {                                                                    \
        ++test_support::tests_run;                                          \
        auto a_ = (actual);                                                 \
        auto e_ = (expected);                                               \
        if (a_ != e_) {                                                     \
            std::ostringstream os_;                                         \
            os_ << "  FAIL: " << __FILE__ << ":" << __LINE__              \
                << ": " << #actual << " == " << +a_                       \
                << ", expected " << +e_ << "\n";                         \
            std::cerr << os_.str();                                         \
            return false;                                                   \
        }                                                                   \
        ++test_support::tests_passed;                                       \
    } while (0)

using TestCase = std::pair<const char*, bool (*)()>;

inline int run_suite(const char* suite_name, std::initializer_list<TestCase> tests) {
    std::cout << "=================================\n";
    std::cout << suite_name << "\n";
    std::cout << "=================================\n\n";

    bool all_passed = true;
    for (const auto& [name, test] : tests) {
        (void)name;
        all_passed &= test();
    }

    std::cout << "\n" << tests_passed << "/" << tests_run << " assertions passed.\n";
    if (all_passed) {
        std::cout << "All tests passed!\n";
        return 0;
    }

    std::cerr << "Some tests FAILED.\n";
    return 1;
}

inline void set_xdg_config_home(const std::filesystem::path& config_root) {
    if (setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) != 0) {
        throw std::runtime_error("Failed to set XDG_CONFIG_HOME");
    }
}

inline std::string escape_json(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

inline std::filesystem::path write_test_rom(const std::filesystem::path& rom_path, const std::string& title) {
    std::vector<unsigned char> rom(0x8000, 0x00);
    for (size_t i = 0; i < title.size() && i < 16; ++i) {
        rom[0x0134 + i] = static_cast<unsigned char>(title[i]);
    }
    rom[0x0147] = 0x00;
    rom[0x0149] = 0x00;

    std::ofstream rom_file(rom_path, std::ios::binary);
    if (!rom_file.is_open()) {
        throw std::runtime_error("Failed to create test ROM");
    }
    rom_file.write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
    return rom_path;
}

inline void append_extra_byte_to_state_blob(const std::filesystem::path& state_path) {
    const std::string header = constants::savestate::kHeader;
    std::ifstream input(state_path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open state file for corruption test");
    }

    std::vector<unsigned char> state_bytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());

    if (state_bytes.size() < header.size() + 4) {
        throw std::runtime_error("State file too small for corruption test");
    }

    const size_t size_offset = header.size();
    const unsigned int original_size =
        static_cast<unsigned int>(state_bytes[size_offset]) |
        (static_cast<unsigned int>(state_bytes[size_offset + 1]) << 8) |
        (static_cast<unsigned int>(state_bytes[size_offset + 2]) << 16) |
        (static_cast<unsigned int>(state_bytes[size_offset + 3]) << 24);
    const unsigned int corrupted_size = original_size + 1;

    state_bytes[size_offset] = static_cast<unsigned char>(corrupted_size & 0xFF);
    state_bytes[size_offset + 1] = static_cast<unsigned char>((corrupted_size >> 8) & 0xFF);
    state_bytes[size_offset + 2] = static_cast<unsigned char>((corrupted_size >> 16) & 0xFF);
    state_bytes[size_offset + 3] = static_cast<unsigned char>((corrupted_size >> 24) & 0xFF);
    state_bytes.push_back(0xAA);

    std::ofstream output(state_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to rewrite state file for corruption test");
    }
    output.write(reinterpret_cast<const char*>(state_bytes.data()), static_cast<std::streamsize>(state_bytes.size()));
}

inline void overwrite_state_payload_byte(const std::filesystem::path& state_path, size_t payload_offset, unsigned char value) {
    const size_t data_offset = constants::savestate::kHeaderLength + 4;
    std::fstream state_file(state_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!state_file.is_open()) {
        throw std::runtime_error("Failed to open state file for mutation test");
    }

    state_file.seekp(static_cast<std::streamoff>(data_offset + payload_offset));
    state_file.put(static_cast<char>(value));
}

inline std::vector<u8> make_mbc3_test_rom() {
    std::vector<u8> rom(0x8000, 0x00);
    rom[0x0147] = 0x0F;
    rom[0x0149] = 0x00;
    return rom;
}

inline std::vector<u8> make_mbc1_test_rom() {
    std::vector<u8> rom(0x8000, 0x00);
    rom[0x0147] = 0x01;
    rom[0x0149] = 0x00;
    return rom;
}

inline std::vector<u8> make_mbc5_test_rom() {
    std::vector<u8> rom(0x8000, 0x00);
    rom[0x0147] = 0x1C;
    rom[0x0149] = 0x00;
    return rom;
}

inline void overwrite_mbc3_base_time(std::vector<u8>& state, std::time_t base_time) {
    const size_t rtc_base_time_offset = 4 + 5 + 5;
    const int64_t encoded = static_cast<int64_t>(base_time);
    for (size_t i = 0; i < 8; ++i) {
        state[rtc_base_time_offset + i] = static_cast<u8>(encoded >> (i * 8));
    }
}

}  // namespace test_support