// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "test_common.h"

using namespace test_support;

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

bool test_recent_roms_failed_load_does_not_overwrite_existing_file() {
    std::cout << "Testing recent ROMs failed-load preservation...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_recent_roms_failed_load_test";
    const fs::path config_root = temp_root / "config";
    const fs::path app_config_dir = config_root / constants::application::kConfigDirectoryName;
    const fs::path config_file = app_config_dir / constants::application::kRecentRomsFileName;
    const std::string malformed_contents = "{\n  \"roms\": [\n    { \"path\": \"broken.gb\"";

    fs::remove_all(temp_root);
    fs::create_directories(app_config_dir);
    TEST_ASSERT(setenv("XDG_CONFIG_HOME", config_root.string().c_str(), 1) == 0);

    {
        std::ofstream config_stream(config_file, std::ios::trunc);
        TEST_ASSERT(config_stream.is_open());
        config_stream << malformed_contents;
    }

    {
        RecentRoms recent_roms;
        TEST_ASSERT(recent_roms.is_empty());
    }

    std::ifstream preserved_stream(config_file);
    TEST_ASSERT(preserved_stream.is_open());
    const std::string preserved_contents(
        (std::istreambuf_iterator<char>(preserved_stream)),
        std::istreambuf_iterator<char>());
    TEST_ASSERT(preserved_contents == malformed_contents);

    fs::remove_all(temp_root);

    std::cout << "  PASS: Recent ROMs failed load does not overwrite existing file\n";
    return true;
}

bool test_recent_roms_handles_paths_with_delimiters() {
    std::cout << "Testing recent ROMs paths with JSON delimiters...\n";

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

    emulator.cpu_for_testing().registers().a = 0x42;
    emulator.cpu_for_testing().registers().pc = 0xC123;
    emulator.cpu_for_testing().set_ime(true);
    emulator.memory_for_testing().write(0xC000, 0x99);
    emulator.memory_for_testing().write(0xFF05, 0x77);
    emulator.ppu_for_testing().set_scanline(77);
    emulator.ppu_for_testing().set_mode(PPU::Mode::Transfer);

    TEST_ASSERT(emulator.save_state(0));
    TEST_ASSERT(fs::exists(emulator.get_state_path(0)));

    emulator.cpu_for_testing().registers().a = 0x10;
    emulator.cpu_for_testing().registers().pc = 0xC000;
    emulator.cpu_for_testing().set_ime(false);
    emulator.memory_for_testing().write(0xC000, 0x11);
    emulator.memory_for_testing().write(0xFF05, 0x22);
    emulator.ppu_for_testing().set_scanline(12);
    emulator.ppu_for_testing().set_mode(PPU::Mode::HBlank);

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

bool test_save_state_load_sanitizes_ppu_state_end_to_end() {
    std::cout << "Testing save-state PPU sanitization end to end...\n";

    namespace fs = std::filesystem;
    const fs::path temp_root = fs::temp_directory_path() / "gbglow_savestate_ppu_sanitize";
    const fs::path config_root = temp_root / "config";
    const fs::path rom_path = temp_root / "sanitize.gb";

    fs::remove_all(temp_root);
    fs::create_directories(config_root);
    fs::create_directories(temp_root);
    set_xdg_config_home(config_root);
    write_test_rom(rom_path, "SANITIZE");

    Emulator emulator;
    TEST_ASSERT(emulator.load_rom(rom_path.string()));
    emulator.memory_for_testing().write(io_reg::REG_LYC, 153);
    TEST_ASSERT(emulator.save_state(0));

    std::vector<u8> prefix;
    emulator.cpu().serialize(prefix);
    emulator.memory().serialize(prefix);
    const size_t ppu_offset = prefix.size();

    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 0, static_cast<unsigned char>(PPU::Mode::VBlank));
    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 1, 0xFF);
    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 2, 0xFF);
    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 3, 0xFF);
    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 5, 0xFF);
    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 6, 0xFF);
    overwrite_state_payload_byte(emulator.get_state_path(0), ppu_offset + 7, 0xFF);

    TEST_ASSERT(emulator.load_state(0));
    TEST_EQ(emulator.ppu().get_mode(), static_cast<u8>(PPU::Mode::VBlank));
    TEST_EQ(emulator.ppu().get_ly(), 153);
    TEST_EQ(emulator.memory().read(io_reg::REG_LY), 153);
    TEST_EQ(emulator.memory().read(io_reg::REG_STAT) & 0x03, static_cast<u8>(PPU::Mode::VBlank));
    TEST_EQ(emulator.memory().read(io_reg::REG_STAT) & 0x04, 0x04);

    fs::remove_all(temp_root);

    std::cout << "  PASS: Save-state load sanitizes PPU state end to end\n";
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

    emulator.cpu_for_testing().registers().a = 0x33;
    emulator.cpu_for_testing().registers().pc = 0xC200;
    emulator.memory_for_testing().write(0xC010, 0xAB);
    emulator.memory_for_testing().write(0xFF05, 0x66);
    emulator.ppu_for_testing().set_scanline(55);
    emulator.ppu_for_testing().set_mode(PPU::Mode::VBlank);
    TEST_ASSERT(emulator.save_state(0));

    append_extra_byte_to_state_blob(emulator.get_state_path(0));

    emulator.cpu_for_testing().registers().a = 0x55;
    emulator.cpu_for_testing().registers().pc = 0xD000;
    emulator.memory_for_testing().write(0xC010, 0x5A);
    emulator.memory_for_testing().write(0xFF05, 0x22);
    emulator.ppu_for_testing().set_scanline(12);
    emulator.ppu_for_testing().set_mode(PPU::Mode::HBlank);

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

bool test_memory_deserialize_masks_vram_bank() {
    std::cout << "Testing memory state sanitizes invalid VRAM bank values...\n";

    constexpr size_t kVramBankIndex = 16384;

    Memory restored_bank_0;
    std::vector<u8> state_bank_0;
    restored_bank_0.serialize(state_bank_0);
    state_bank_0[kVramBankIndex] = 0x02;

    size_t offset = 0;
    restored_bank_0.deserialize(state_bank_0.data(), state_bank_0.size(), offset);

    std::vector<u8> reserialized_bank_0;
    restored_bank_0.serialize(reserialized_bank_0);
    TEST_EQ(reserialized_bank_0[kVramBankIndex], 0x00);

    Memory restored_bank_1;
    std::vector<u8> state_bank_1;
    restored_bank_1.serialize(state_bank_1);
    state_bank_1[kVramBankIndex] = 0x03;

    offset = 0;
    restored_bank_1.deserialize(state_bank_1.data(), state_bank_1.size(), offset);

    std::vector<u8> reserialized_bank_1;
    restored_bank_1.serialize(reserialized_bank_1);
    TEST_EQ(reserialized_bank_1[kVramBankIndex], 0x01);

    std::cout << "  PASS: Memory state masks invalid VRAM bank values\n";
    return true;
}

bool test_cartridge_deserialize_rejects_mismatched_ram_size() {
    std::cout << "Testing cartridge state rejects mismatched RAM payload sizes...\n";

    MBC3 cartridge(make_mbc3_test_rom(), 0, true);
    cartridge.write(0x2000, 0x05);

    std::vector<u8> state;
    cartridge.serialize(state);
    state[0] = 0x01;
    state[1] = 0x00;
    state[2] = 0x00;
    state[3] = 0x00;
    state[5] = 0x07;

    size_t offset = 0;
    cartridge.deserialize(state.data(), state.size(), offset);

    std::vector<u8> reserialized;
    cartridge.serialize(reserialized);
    TEST_EQ(reserialized[0], 0x00);
    TEST_EQ(reserialized[1], 0x00);
    TEST_EQ(reserialized[2], 0x00);
    TEST_EQ(reserialized[3], 0x00);
    TEST_EQ(reserialized[5], 0x05);

    std::cout << "  PASS: Cartridge state rejects mismatched RAM payload sizes\n";
    return true;
}

bool test_mbc1_deserialize_sanitizes_banks() {
    std::cout << "Testing MBC1 state sanitizes invalid bank values...\n";

    MBC1 cartridge(make_mbc1_test_rom(), 0);
    std::vector<u8> state;
    cartridge.serialize(state);
    state[4] = 0x01;
    state[5] = 0x00;
    state[6] = 0xFF;
    state[7] = 0x01;

    size_t offset = 0;
    cartridge.deserialize(state.data(), state.size(), offset);

    std::vector<u8> reserialized;
    cartridge.serialize(reserialized);
    TEST_EQ(reserialized[4], 0x01);
    TEST_EQ(reserialized[5], 0x01);
    TEST_EQ(reserialized[6], 0x03);
    TEST_EQ(reserialized[7], 0x01);

    std::cout << "  PASS: MBC1 state sanitizes invalid bank values\n";
    return true;
}

bool test_mbc3_deserialize_sanitizes_mapper_state() {
    std::cout << "Testing MBC3 state sanitizes invalid mapper values...\n";

    MBC3 cartridge(make_mbc3_test_rom(), 0, true);
    std::vector<u8> state;
    cartridge.serialize(state);
    state[4] = 0x01;
    state[5] = 0x00;
    state[6] = 0xFF;
    state[7] = 0x01;
    state[8] = 0xAA;
    state[9] = 99;
    state[10] = 99;
    state[11] = 99;
    state[12] = 0x12;
    state[13] = 0xFF;

    size_t offset = 0;
    cartridge.deserialize(state.data(), state.size(), offset);

    std::vector<u8> reserialized;
    cartridge.serialize(reserialized);
    TEST_EQ(reserialized[4], 0x01);
    TEST_EQ(reserialized[5], 0x01);
    TEST_EQ(reserialized[6], 0x00);
    TEST_EQ(reserialized[7], 0x01);
    TEST_EQ(reserialized[8], 0xAA);
    TEST_EQ(reserialized[9], 59);
    TEST_EQ(reserialized[10], 59);
    TEST_EQ(reserialized[11], 23);
    TEST_EQ(reserialized[12], 0x12);
    TEST_EQ(reserialized[13], 0xC1);

    std::cout << "  PASS: MBC3 state sanitizes invalid mapper values\n";
    return true;
}

bool test_mbc5_deserialize_sanitizes_banks() {
    std::cout << "Testing MBC5 state sanitizes invalid bank values...\n";

    MBC5 cartridge(make_mbc5_test_rom(), 0, true);
    std::vector<u8> state;
    cartridge.serialize(state);
    state[4] = 0x01;
    state[5] = 0xFF;
    state[6] = 0xFF;
    state[7] = 0xFF;
    state[8] = 0x01;

    size_t offset = 0;
    cartridge.deserialize(state.data(), state.size(), offset);

    std::vector<u8> reserialized;
    cartridge.serialize(reserialized);
    TEST_EQ(reserialized[4], 0x01);
    TEST_EQ(reserialized[5], 0xFF);
    TEST_EQ(reserialized[6], 0x01);
    TEST_EQ(reserialized[7], 0x07);
    TEST_EQ(reserialized[8], 0x01);

    std::cout << "  PASS: MBC5 state sanitizes invalid bank values\n";
    return true;
}

bool test_timer_deserialize_sanitizes_control_state() {
    std::cout << "Testing timer state sanitizes invalid control values...\n";

    Memory memory;
    Timer timer(memory);
    std::vector<u8> state;
    timer.serialize(state);
    state[3] = 0xFF;
    state[4] = 0x34;
    state[5] = 0x12;
    state[6] = 0xFF;
    state[7] = 0xFF;

    size_t offset = 0;
    timer.deserialize(state.data(), state.size(), offset);

    std::vector<u8> reserialized;
    timer.serialize(reserialized);
    TEST_EQ(reserialized[3], 0x07);
    TEST_EQ(reserialized[4], 0x34);
    TEST_EQ(reserialized[5], 0x00);
    TEST_EQ(reserialized[6], 0xFF);
    TEST_EQ(reserialized[7], 0x00);

    std::cout << "  PASS: Timer state sanitizes invalid control values\n";
    return true;
}

bool test_apu_deserialize_sanitizes_runtime_state() {
    std::cout << "Testing APU state sanitizes invalid runtime values...\n";

    Memory memory;
    APU apu(memory);
    std::vector<u8> state;
    apu.serialize(state);

    state[2] = 0xFF;
    state[19] = 0xFF;
    state[20] = 0xFF;
    state[21] = 0xFF;
    state[22] = 0xFF;
    state[26] = 0xFF;
    state[33] = 0x00;
    state[34] = 0x01;
    state[79] = 0xFF;
    state[120] = 0xFF;
    state[126] = 0xFF;
    state[127] = 0xFF;
    state[128] = 0xFF;
    state[129] = 0xFF;
    state[145] = 0xFF;
    state[146] = 0xFF;
    state[147] = 0xFF;

    size_t offset = 0;
    apu.deserialize(state.data(), state.size(), offset);

    std::vector<u8> reserialized;
    apu.serialize(reserialized);

    TEST_EQ(reserialized[2], 0x80);
    TEST_EQ(reserialized[19], 0x00);
    TEST_EQ(reserialized[20], 0x00);
    TEST_EQ(reserialized[21], 0x00);
    TEST_EQ(reserialized[22], 0x00);
    TEST_EQ(reserialized[26], 0x03);
    TEST_EQ(reserialized[34], 0x00);
    TEST_EQ(reserialized[79], 0x03);
    TEST_EQ(reserialized[120], 0x03);
    TEST_EQ(reserialized[126], 0x00);
    TEST_EQ(reserialized[127], 0x00);
    TEST_EQ(reserialized[128], 0x00);
    TEST_EQ(reserialized[129], 0x00);
    TEST_EQ(reserialized[145], 0x0F);
    TEST_EQ(reserialized[146], 0x01);
    TEST_EQ(reserialized[147], 0x07);

    std::cout << "  PASS: APU state sanitizes invalid runtime values\n";
    return true;
}

bool test_cpu_deserialize_masks_flag_low_nibble() {
    std::cout << "Testing CPU state masks invalid flag bits...\n";

    Memory memory;
    CPU cpu(memory);
    std::vector<u8> state;
    cpu.serialize(state);
    state[1] = 0xFF;

    size_t offset = 0;
    cpu.deserialize(state.data(), state.size(), offset);

    TEST_EQ(cpu.registers().f, 0xF0);

    std::vector<u8> reserialized;
    cpu.serialize(reserialized);
    TEST_EQ(reserialized[1], 0xF0);

    std::cout << "  PASS: CPU state masks invalid flag bits\n";
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

bool test_timer_preserves_progress_on_redundant_tac_write() {
    std::cout << "Testing timer TAC rewrite behavior...\n";

    Memory memory;
    Timer timer(memory);

    timer.write_tac(0x05);
    timer.step(8);
    timer.write_tac(0x05);
    timer.step(8);

    TEST_EQ(timer.read_tima(), 1);

    std::cout << "  PASS: Redundant TAC writes preserve timer progress\n";
    return true;
}

bool test_mbc3_repeated_latch_refreshes_rtc() {
    std::cout << "Testing MBC3 repeated RTC latch refresh...\n";

    MBC3 cartridge(make_mbc3_test_rom(), 0, true);
    cartridge.write(0x0000, 0x0A);
    cartridge.write(0x4000, 0x08);
    cartridge.write(0xA000, 0x00);

    std::vector<u8> state;
    cartridge.serialize(state);
    overwrite_mbc3_base_time(state, std::time(nullptr) - 2);

    size_t offset = 0;
    cartridge.deserialize(state.data(), state.size(), offset);

    cartridge.write(0x6000, 0x00);
    cartridge.write(0x6000, 0x01);
    const u8 first_latched_seconds = cartridge.read(0xA000);
    TEST_ASSERT(first_latched_seconds >= 1);

    state.clear();
    cartridge.serialize(state);
    overwrite_mbc3_base_time(state, std::time(nullptr) - 5);
    offset = 0;
    cartridge.deserialize(state.data(), state.size(), offset);

    cartridge.write(0x6000, 0x00);
    cartridge.write(0x6000, 0x01);
    const u8 second_latched_seconds = cartridge.read(0xA000);

    TEST_ASSERT(second_latched_seconds > first_latched_seconds);

    std::cout << "  PASS: MBC3 latch sequence refreshes RTC values repeatedly\n";
    return true;
}

int main() {
    return run_suite("gbglow Persistence Tests", {
        {"recent_roms_round_trip", test_recent_roms_round_trip},
        {"recent_roms_malformed", test_recent_roms_ignores_malformed_entries},
        {"recent_roms_failed_load", test_recent_roms_failed_load_does_not_overwrite_existing_file},
        {"recent_roms_delimiters", test_recent_roms_handles_paths_with_delimiters},
        {"recent_roms_key_tokens", test_recent_roms_handles_paths_with_key_tokens},
        {"gamepad_invalid_buttons", test_gamepad_config_ignores_invalid_buttons_and_creates_dirs},
        {"save_state_round_trip", test_save_state_round_trip},
        {"save_state_ppu_sanitize", test_save_state_load_sanitizes_ppu_state_end_to_end},
        {"save_state_corrupt", test_corrupt_save_state_does_not_mutate_live_state},
        {"save_state_requires_rom", test_save_state_requires_loaded_rom},
        {"memory_deserialize_vram", test_memory_deserialize_masks_vram_bank},
        {"cartridge_ram_size", test_cartridge_deserialize_rejects_mismatched_ram_size},
        {"mbc1_sanitize", test_mbc1_deserialize_sanitizes_banks},
        {"mbc3_sanitize", test_mbc3_deserialize_sanitizes_mapper_state},
        {"mbc5_sanitize", test_mbc5_deserialize_sanitizes_banks},
        {"timer_deserialize", test_timer_deserialize_sanitizes_control_state},
        {"apu_deserialize", test_apu_deserialize_sanitizes_runtime_state},
        {"cpu_deserialize_flags", test_cpu_deserialize_masks_flag_low_nibble},
        {"gamepad_deadzone", test_gamepad_config_clamps_deadzone_and_trims_lines},
        {"timer_tac_rewrite", test_timer_preserves_progress_on_redundant_tac_write},
        {"mbc3_latch_refresh", test_mbc3_repeated_latch_refreshes_rtc},
    });
}