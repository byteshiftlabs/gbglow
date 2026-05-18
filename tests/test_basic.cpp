// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "../src/core/cpu.h"
#include "../src/core/constants.h"
#include "../src/core/emulator.h"
#include "../src/core/memory.h"
#include "../src/core/registers.h"
#include "../src/debug/debugger.h"
#include "../src/debug/debugger_gui.h"
#include "../src/input/gamepad.h"
#include "../src/ui/recent_roms.h"
#include "../src/video/ppu.h"
#include <SDL2/SDL.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gbglow;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond)                                                   \
    do {                                                                    \
        ++tests_run;                                                        \
        if (!(cond)) {                                                      \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__          \
                      << ": " << #cond << "\n";                             \
            return false;                                                   \
        }                                                                   \
        ++tests_passed;                                                     \
    } while (0)

#define TEST_EQ(actual, expected)                                           \
    do {                                                                    \
        ++tests_run;                                                        \
        auto a_ = (actual); auto e_ = (expected);                          \
        if (a_ != e_) {                                                     \
            std::ostringstream os_;                                          \
            os_ << "  FAIL: " << __FILE__ << ":" << __LINE__                \
                << ": " << #actual << " == " << +a_                         \
                << ", expected " << +e_ << "\n";                            \
            std::cerr << os_.str();                                         \
            return false;                                                   \
        }                                                                   \
        ++tests_passed;                                                     \
    } while (0)

bool test_registers() {
    std::cout << "Testing registers...\n";
    
    Registers regs;
    regs.reset();
    
    // Check initial values after reset
    TEST_EQ(regs.a, 0x01);
    TEST_EQ(regs.pc, static_cast<u16>(0x0100));
    TEST_EQ(regs.sp, static_cast<u16>(0xFFFE));
    
    // Test register pairs
    regs.set_bc(0x1234);
    TEST_EQ(regs.b, 0x12);
    TEST_EQ(regs.c, 0x34);
    TEST_EQ(regs.bc(), static_cast<u16>(0x1234));
    
    // Test flags
    regs.set_flag(Registers::FLAG_Z, true);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    regs.set_flag(Registers::FLAG_Z, false);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_Z));
    
    std::cout << "  PASS: Registers working correctly\n";
    return true;
}

bool test_memory() {
    std::cout << "Testing memory...\n";
    
    Memory mem;
    
    // Test WRAM read/write
    mem.write(0xC000, 0x42);
    TEST_EQ(mem.read(0xC000), 0x42);
    
    // Test echo RAM
    mem.write(0xC100, 0xAB);
    TEST_EQ(mem.read(0xE100), 0xAB);
    
    // Test HRAM
    mem.write(0xFF80, 0xCD);
    TEST_EQ(mem.read(0xFF80), 0xCD);
    
    // Test 16-bit operations
    mem.write16(0xC200, 0x1234);
    TEST_EQ(mem.read16(0xC200), static_cast<u16>(0x1234));
    
    std::cout << "  PASS: Memory working correctly\n";
    return true;
}

bool test_cpu_basic() {
    std::cout << "Testing CPU basics...\n";
    
    Memory mem;
    CPU cpu(mem);
    cpu.reset();
    
    // Write a simple program to WRAM: LD A, 0x42 ; HALT
    mem.write(0xC000, 0x3E);  // LD A, n
    mem.write(0xC001, 0x42);  // n = 0x42
    mem.write(0xC002, 0x76);  // HALT
    
    // Set PC to our program
    cpu.registers().pc = 0xC000;
    
    // Execute LD A, 0x42
    Cycles cycles1 = cpu.step();
    TEST_EQ(cycles1, static_cast<Cycles>(2));
    TEST_EQ(cpu.registers().a, 0x42);
    TEST_EQ(cpu.registers().pc, static_cast<u16>(0xC002));
    
    // Execute HALT
    Cycles cycles2 = cpu.step();
    TEST_EQ(cycles2, static_cast<Cycles>(1));
    TEST_ASSERT(cpu.is_halted());
    
    std::cout << "  PASS: CPU basic execution working\n";
    return true;
}

bool test_cpu_flags() {
    std::cout << "Testing CPU ALU flags...\n";
    
    Memory mem;
    CPU cpu(mem);
    cpu.reset();
    
    // XOR A (opcode 0xAF): A ^= A -> A = 0, sets Z, clears N/H/C
    cpu.registers().a = 0x42;
    cpu.registers().f = 0x00;
    mem.write(0xC000, 0xAF);  // XOR A
    mem.write(0xC001, 0xC6);  // ADD A, n
    mem.write(0xC002, 0x01);  // n = 0x01
    cpu.registers().pc = 0xC000;
    
    Cycles cycles = cpu.step();
    TEST_EQ(cycles, static_cast<Cycles>(1));
    TEST_EQ(cpu.registers().a, 0x00);
    TEST_ASSERT(cpu.registers().get_flag(Registers::FLAG_Z));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_N));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_H));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_C));
    
    // ADD A, 0x01: 0 + 1 = 1, clears Z, no carry
    cycles = cpu.step();
    TEST_EQ(cycles, static_cast<Cycles>(2));
    TEST_EQ(cpu.registers().a, 0x01);
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_Z));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_C));
    
    std::cout << "  PASS: CPU ALU flags working\n";
    return true;
}

bool test_recent_roms_round_trip() {
    std::cout << "Testing recent ROMs persistence...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_recent_roms_test";
    const fs::path config_root = temp_root / "config";
    const fs::path rom_path = temp_root / "Pokemon Red.gb";

    fs::remove_all(temp_root);
    fs::create_directories(config_root);

    TEST_ASSERT(setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) == 0);

    {
        std::ofstream rom_file(rom_path);
        TEST_ASSERT(rom_file.is_open());
        rom_file << "rom";
    }

    {
        RecentRoms recent_roms;
        recent_roms.clear();
        recent_roms.add_rom(rom_path.string());
        TEST_ASSERT(!recent_roms.is_empty());
        TEST_EQ(recent_roms.get_roms().size(), static_cast<size_t>(1));
        TEST_ASSERT(recent_roms.get_roms().front().file_path == rom_path.string());
    }

    {
        RecentRoms reloaded_recent_roms;
        TEST_ASSERT(!reloaded_recent_roms.is_empty());
        TEST_EQ(reloaded_recent_roms.get_roms().size(), static_cast<size_t>(1));
        TEST_ASSERT(reloaded_recent_roms.get_roms().front().file_path == rom_path.string());
        TEST_ASSERT(reloaded_recent_roms.get_roms().front().display_name == "Pokemon Red.gb");
    }

    fs::remove_all(temp_root);

    std::cout << "  PASS: Recent ROMs persistence working\n";
    return true;
}

bool test_recent_roms_ignores_malformed_entries() {
    std::cout << "Testing recent ROMs malformed input handling...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_recent_roms_malformed_test";
    const fs::path config_root = temp_root / "config";
    const fs::path app_config_dir = config_root / constants::application::kConfigDirectoryName;
    const fs::path rom_path = temp_root / "Kirby.gb";
    const fs::path config_file = app_config_dir / constants::application::kRecentRomsFileName;

    fs::remove_all(temp_root);
    fs::create_directories(app_config_dir);

    TEST_ASSERT(setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) == 0);

    {
        std::ofstream rom_file(rom_path);
        TEST_ASSERT(rom_file.is_open());
        rom_file << "rom";
    }

    {
        std::ofstream config_stream(config_file);
        TEST_ASSERT(config_stream.is_open());
        config_stream
            << "{\n"
            << "  \"roms\": [\n"
            << "    { \"path\": \"missing_time.gb\" },\n"
            << "    { \"path\": \"" << rom_path.string() << "\", \"time\": 12345 },\n"
            << "    { \"path\": \"" << rom_path.string() << "\", \"time\": 67890 }\n"
            << "  ]\n"
            << "}\n";
    }

    {
        RecentRoms recent_roms;
        TEST_ASSERT(!recent_roms.is_empty());
        TEST_EQ(recent_roms.get_roms().size(), static_cast<size_t>(1));
        TEST_ASSERT(recent_roms.get_roms().front().file_path == rom_path.string());
        TEST_ASSERT(recent_roms.get_roms().front().display_name == "Kirby.gb");
    }

    fs::remove_all(temp_root);

    std::cout << "  PASS: Recent ROMs malformed input handled safely\n";
    return true;
}

bool test_debugger_gui_clears_execution_requests() {
    std::cout << "Testing debugger GUI execution request clearing...\n";

    DebuggerGUI debugger_gui;
    debugger_gui.set_visible(true);

    debugger_gui.continue_execution();
    TEST_ASSERT(debugger_gui.should_continue());
    TEST_ASSERT(!debugger_gui.step_requested());

    debugger_gui.request_step();
    TEST_ASSERT(debugger_gui.step_requested());
    TEST_ASSERT(!debugger_gui.should_continue());

    debugger_gui.continue_execution();
    TEST_ASSERT(debugger_gui.should_continue());
    TEST_ASSERT(!debugger_gui.step_requested());

    debugger_gui.set_visible(false);
    TEST_ASSERT(!debugger_gui.should_continue());
    TEST_ASSERT(!debugger_gui.step_requested());
    TEST_ASSERT(!debugger_gui.is_paused());

    debugger_gui.set_visible(true);
    debugger_gui.continue_execution();
    debugger_gui.toggle_visible();
    TEST_ASSERT(!debugger_gui.is_visible());
    TEST_ASSERT(!debugger_gui.should_continue());
    TEST_ASSERT(!debugger_gui.step_requested());
    TEST_ASSERT(!debugger_gui.is_paused());

    std::cout << "  PASS: Debugger GUI request state resets correctly\n";
    return true;
}

bool test_gamepad_config_ignores_invalid_buttons_and_creates_dirs() {
    std::cout << "Testing gamepad config save/load robustness...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_gamepad_save_test";
    const fs::path config_file = temp_root / "nested" / "gamepad.conf";

    fs::remove_all(temp_root);

    Gamepad gamepad;
    gamepad.save_config(config_file.string());
    TEST_ASSERT(fs::exists(config_file));

    {
        std::ofstream config_stream(config_file);
        TEST_ASSERT(config_stream.is_open());
        config_stream
            << "gamepad_a=INVALID\n"
            << "gamepad_b=X\n"
            << "gamepad_start=BAD\n"
            << "gamepad_select=BACK\n";
    }

    gamepad.reset_default_mapping();
    gamepad.load_config(config_file.string());

    const auto& mapping = gamepad.get_button_mapping();
    TEST_EQ(mapping.gb_a, SDL_CONTROLLER_BUTTON_A);
    TEST_EQ(mapping.gb_b, SDL_CONTROLLER_BUTTON_X);
    TEST_EQ(mapping.gb_start, SDL_CONTROLLER_BUTTON_START);
    TEST_EQ(mapping.gb_select, SDL_CONTROLLER_BUTTON_BACK);

    fs::remove_all(temp_root);

    std::cout << "  PASS: Gamepad config ignores invalid buttons and saves nested paths\n";
    return true;
}

bool test_recent_roms_handles_paths_with_delimiters() {
    std::cout << "Testing recent ROMs paths with JSON delimiters...\n";

    auto escape_json = [](const std::string& value) {
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
    };

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_recent_roms_delimiter_test";
    const fs::path config_root = temp_root / "config";
    const fs::path app_config_dir = config_root / constants::application::kConfigDirectoryName;
    const fs::path rom_path = temp_root / "Legend ] of } Zelda \"DX\".gb";
    const fs::path config_file = app_config_dir / constants::application::kRecentRomsFileName;

    fs::remove_all(temp_root);
    fs::create_directories(app_config_dir);

    TEST_ASSERT(setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) == 0);

    {
        std::ofstream rom_file(rom_path);
        TEST_ASSERT(rom_file.is_open());
        rom_file << "rom";
    }

    {
        std::ofstream config_stream(config_file);
        TEST_ASSERT(config_stream.is_open());
        config_stream
            << "{\n"
            << "  \"roms\": [\n"
            << "    { \"path\": \"" << escape_json(rom_path.string()) << "\", \"time\": 12345 }\n"
            << "  ]\n"
            << "}\n";
    }

    {
        RecentRoms recent_roms;
        TEST_ASSERT(!recent_roms.is_empty());
        TEST_EQ(recent_roms.get_roms().size(), static_cast<size_t>(1));
        TEST_ASSERT(recent_roms.get_roms().front().file_path == rom_path.string());
        TEST_ASSERT(recent_roms.get_roms().front().display_name == "Legend ] of } Zelda \"DX\".gb");
    }

    fs::remove_all(temp_root);

    std::cout << "  PASS: Recent ROMs delimiter-heavy paths load correctly\n";
    return true;
}

bool test_recent_roms_handles_paths_with_key_tokens() {
    std::cout << "Testing recent ROMs paths containing key tokens...\n";

    auto escape_json = [](const std::string& value) {
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
    };

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_recent_roms_key_token_test";
    const fs::path config_root = temp_root / "config";
    const fs::path app_config_dir = config_root / constants::application::kConfigDirectoryName;
    const fs::path rom_path = temp_root / "time";
    const fs::path config_file = app_config_dir / constants::application::kRecentRomsFileName;

    fs::remove_all(temp_root);
    fs::create_directories(app_config_dir);

    TEST_ASSERT(setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) == 0);

    {
        std::ofstream rom_file(rom_path);
        TEST_ASSERT(rom_file.is_open());
        rom_file << "rom";
    }

    {
        std::ofstream config_stream(config_file);
        TEST_ASSERT(config_stream.is_open());
        config_stream
            << "{\n"
            << "  \"roms\": [\n"
            << "    { \"path\": \"" << escape_json(rom_path.string()) << "\", \"time\": 67890 }\n"
            << "  ]\n"
            << "}\n";
    }

    {
        RecentRoms recent_roms;
        TEST_ASSERT(!recent_roms.is_empty());
        TEST_EQ(recent_roms.get_roms().size(), static_cast<size_t>(1));
        TEST_ASSERT(recent_roms.get_roms().front().file_path == rom_path.string());
        TEST_ASSERT(recent_roms.get_roms().front().display_name == "time");
    }

    fs::remove_all(temp_root);

    std::cout << "  PASS: Recent ROMs key-token paths load correctly\n";
    return true;
}

namespace {

void set_xdg_config_home(const std::filesystem::path& config_root) {
    if (setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) != 0) {
        throw std::runtime_error("Failed to set XDG_CONFIG_HOME");
    }
}

std::filesystem::path write_test_rom(const std::filesystem::path& rom_path, const std::string& title) {
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

void append_extra_byte_to_state_blob(const std::filesystem::path& state_path) {
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

}  // namespace

bool test_debugger_prepare_step_over() {
    std::cout << "Testing debugger step-over arming...\n";

    Memory memory;
    CPU cpu(memory);
    PPU ppu(memory);
    Debugger debugger;
    debugger.attach(&cpu, &memory, &ppu);

    cpu.registers().pc = 0xC000;
    memory.write(0xC000, 0xCD);
    memory.write(0xC001, 0x34);
    memory.write(0xC002, 0x12);

    TEST_ASSERT(debugger.prepare_step_over_for_current_instruction());
    TEST_ASSERT(debugger.is_step_over_active());
    TEST_ASSERT(debugger.should_stop_step_over(0xC003));

    debugger.clear_step_over();
    memory.write(0xC000, 0x00);
    TEST_ASSERT(!debugger.prepare_step_over_for_current_instruction());
    TEST_ASSERT(!debugger.is_step_over_active());

    std::cout << "  PASS: Debugger step-over arming working\n";
    return true;
}

bool test_debugger_continue_skips_current_breakpoint_once() {
    std::cout << "Testing debugger continue from breakpoint...\n";

    Memory memory;
    CPU cpu(memory);
    PPU ppu(memory);
    Debugger debugger;
    debugger.attach(&cpu, &memory, &ppu);

    cpu.registers().pc = 0xC123;
    debugger.add_breakpoint(0xC123);

    TEST_ASSERT(debugger.should_break(0xC123));

    debugger.skip_breakpoint_once(0xC123);
    TEST_ASSERT(!debugger.should_break(0xC123));
    TEST_ASSERT(debugger.should_break(0xC123));

    std::cout << "  PASS: Debugger continue skips current breakpoint once\n";
    return true;
}

bool test_save_state_round_trip() {
    std::cout << "Testing save-state round trip...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_savestate_round_trip";
    const fs::path config_root = temp_root / "config";
    const fs::path rom_path = temp_root / "round_trip.gb";

    fs::remove_all(temp_root);
    fs::create_directories(config_root);
    fs::create_directories(temp_root);
    set_xdg_config_home(config_root);
    write_test_rom(rom_path, "ROUNDTRIP");

    Emulator emulator;
    TEST_ASSERT(emulator.load_rom(rom_path.string()));

    emulator.cpu().registers().a = 0x42;
    emulator.cpu().registers().pc = 0xC123;
    emulator.cpu().set_ime(true);
    emulator.memory().write(0xC000, 0x99);
    emulator.memory().write(0xFF05, 0x77);
    emulator.ppu().set_ly(77);
    emulator.ppu().set_mode(static_cast<u8>(PPU::Mode::Transfer));

    TEST_ASSERT(emulator.save_state(0));
    TEST_ASSERT(fs::exists(emulator.get_state_path(0)));

    emulator.cpu().registers().a = 0x10;
    emulator.cpu().registers().pc = 0xC000;
    emulator.cpu().set_ime(false);
    emulator.memory().write(0xC000, 0x11);
    emulator.memory().write(0xFF05, 0x22);
    emulator.ppu().set_ly(12);
    emulator.ppu().set_mode(static_cast<u8>(PPU::Mode::HBlank));

    TEST_ASSERT(emulator.load_state(0));
    TEST_EQ(emulator.cpu().registers().a, 0x42);
    TEST_EQ(emulator.cpu().registers().pc, static_cast<u16>(0xC123));
    TEST_ASSERT(emulator.cpu().ime());
    TEST_EQ(emulator.memory().read(0xC000), 0x99);
    TEST_EQ(emulator.memory().read(0xFF05), 0x77);
    TEST_EQ(emulator.ppu().get_ly(), 77);
    TEST_EQ(emulator.ppu().get_mode(), static_cast<u8>(PPU::Mode::Transfer));

    fs::remove_all(temp_root);

    std::cout << "  PASS: Save-state round trip working\n";
    return true;
}

bool test_corrupt_save_state_does_not_mutate_live_state() {
    std::cout << "Testing corrupt save-state rejection...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_savestate_corrupt";
    const fs::path config_root = temp_root / "config";
    const fs::path rom_path = temp_root / "corrupt.gb";

    fs::remove_all(temp_root);
    fs::create_directories(config_root);
    fs::create_directories(temp_root);
    set_xdg_config_home(config_root);
    write_test_rom(rom_path, "CORRUPT");

    Emulator emulator;
    TEST_ASSERT(emulator.load_rom(rom_path.string()));

    emulator.cpu().registers().a = 0x33;
    emulator.cpu().registers().pc = 0xC200;
    emulator.memory().write(0xC010, 0xAB);
    emulator.memory().write(0xFF05, 0x66);
    emulator.ppu().set_ly(55);
    emulator.ppu().set_mode(static_cast<u8>(PPU::Mode::VBlank));
    TEST_ASSERT(emulator.save_state(0));

    append_extra_byte_to_state_blob(emulator.get_state_path(0));

    emulator.cpu().registers().a = 0x55;
    emulator.cpu().registers().pc = 0xD000;
    emulator.memory().write(0xC010, 0x5A);
    emulator.memory().write(0xFF05, 0x22);
    emulator.ppu().set_ly(12);
    emulator.ppu().set_mode(static_cast<u8>(PPU::Mode::HBlank));

    TEST_ASSERT(!emulator.load_state(0));
    TEST_EQ(emulator.cpu().registers().a, 0x55);
    TEST_EQ(emulator.cpu().registers().pc, static_cast<u16>(0xD000));
    TEST_EQ(emulator.memory().read(0xC010), 0x5A);
    TEST_EQ(emulator.memory().read(0xFF05), 0x22);
    TEST_EQ(emulator.ppu().get_ly(), 12);
    TEST_EQ(emulator.ppu().get_mode(), static_cast<u8>(PPU::Mode::HBlank));

    fs::remove_all(temp_root);

    std::cout << "  PASS: Corrupt save-state rejected without mutating live state\n";
    return true;
}

bool test_save_state_requires_loaded_rom() {
    std::cout << "Testing save-state rejection without a loaded ROM...\n";

    Emulator emulator;

    TEST_ASSERT(!emulator.save_state(0));
    TEST_ASSERT(!emulator.load_state(0));
    TEST_ASSERT(!emulator.delete_state(0));
    TEST_ASSERT(emulator.get_state_path(0).empty());

    std::cout << "  PASS: Save-state operations require a loaded ROM\n";
    return true;
}

bool test_gamepad_config_clamps_deadzone_and_trims_lines() {
    std::cout << "Testing gamepad config parsing...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_gamepad_config_test";
    const fs::path config_path = temp_root / "gamepad.conf";

    fs::remove_all(temp_root);
    fs::create_directories(temp_root);

    {
        std::ofstream config_stream(config_path);
        TEST_ASSERT(config_stream.is_open());
        config_stream
            << "# comment\r\n"
            << "   \r\n"
            << "gamepad_a = LB\r\n"
            << "gamepad_deadzone = 999999\r\n"
            << "   = ignored\r\n";
    }

    Gamepad gamepad;
    gamepad.load_config(config_path.string());
    TEST_EQ(gamepad.get_deadzone(), Gamepad::SDL_AXIS_MAX);
    TEST_EQ(gamepad.get_button_mapping().gb_a, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);

    {
        std::ofstream config_stream(config_path, std::ios::trunc);
        TEST_ASSERT(config_stream.is_open());
        config_stream << "gamepad_deadzone = -12\n";
    }

    gamepad.load_config(config_path.string());
    TEST_EQ(gamepad.get_deadzone(), 0);

    fs::remove_all(temp_root);

    std::cout << "  PASS: Gamepad config parsing clamps deadzone and trims safely\n";
    return true;
}

int main() {
    std::cout << "=================================\n";
    std::cout << "gbglow Basic Component Tests\n";
    std::cout << "=================================\n\n";
    
    bool all_passed = true;
    all_passed &= test_registers();
    all_passed &= test_memory();
    all_passed &= test_cpu_basic();
    all_passed &= test_cpu_flags();
    all_passed &= test_recent_roms_round_trip();
    all_passed &= test_recent_roms_ignores_malformed_entries();
    all_passed &= test_recent_roms_handles_paths_with_delimiters();
    all_passed &= test_recent_roms_handles_paths_with_key_tokens();
    all_passed &= test_debugger_gui_clears_execution_requests();
    all_passed &= test_debugger_prepare_step_over();
    all_passed &= test_debugger_continue_skips_current_breakpoint_once();
    all_passed &= test_save_state_round_trip();
    all_passed &= test_corrupt_save_state_does_not_mutate_live_state();
    all_passed &= test_save_state_requires_loaded_rom();
    all_passed &= test_gamepad_config_clamps_deadzone_and_trims_lines();
    all_passed &= test_gamepad_config_ignores_invalid_buttons_and_creates_dirs();
    
    std::cout << "\n" << tests_passed << "/" << tests_run << " assertions passed.\n";
    
    if (all_passed) {
        std::cout << "All tests passed!\n";
        return 0;
    } else {
        std::cerr << "Some tests FAILED.\n";
        return 1;
    }
}
