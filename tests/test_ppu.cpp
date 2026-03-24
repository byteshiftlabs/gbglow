// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "test_common.h"

bool test_ppu_deserialize_clamps_invalid_mode() {
    std::cout << "Testing PPU state sanitizes invalid mode values...\n";

    Memory memory;
    PPU restored(memory);
    std::vector<u8> state;
    restored.serialize(state);
    state[0] = 0xFF;

    size_t offset = 0;
    restored.deserialize(state.data(), state.size(), offset);

    TEST_EQ(restored.get_mode(), static_cast<u8>(PPU::Mode::HBlank));

    std::vector<u8> reserialized;
    restored.serialize(reserialized);
    TEST_EQ(reserialized[0], static_cast<u8>(PPU::Mode::HBlank));

    restored.set_mode(static_cast<PPU::Mode>(0xFF));
    TEST_EQ(restored.get_mode(), static_cast<u8>(PPU::Mode::HBlank));

    std::cout << "  PASS: PPU state clamps invalid mode values\n";
    return true;
}

bool test_ppu_deserialize_sanitizes_timing_state_and_syncs_registers() {
    std::cout << "Testing PPU state sanitizes timing state and syncs MMIO registers...\n";

    {
        Memory memory;
        PPU restored(memory);
        memory.set_ppu(&restored);
        memory.write(io_reg::REG_LYC, 153);

        std::vector<u8> state;
        restored.serialize(state);
        state[0] = static_cast<u8>(PPU::Mode::VBlank);
        state[1] = 0xFF;
        state[2] = 0xFF;
        state[3] = 0xFF;
        state[5] = 0xFF;
        state[6] = 0xFF;
        state[7] = 0xFF;

        size_t offset = 0;
        restored.deserialize(state.data(), state.size(), offset);

        std::vector<u8> reserialized;
        restored.serialize(reserialized);
        TEST_EQ(reserialized[0], static_cast<u8>(PPU::Mode::VBlank));
        TEST_EQ(reserialized[1], 0xC7);
        TEST_EQ(reserialized[2], 0x01);
        TEST_EQ(reserialized[3], 153);
        TEST_EQ(reserialized[5], 143);
        TEST_EQ(reserialized[6], 0xBF);
        TEST_EQ(reserialized[7], 0xBF);
        TEST_EQ(memory.read(io_reg::REG_LY), 153);
        TEST_EQ(memory.read(io_reg::REG_STAT) & 0x03, static_cast<u8>(PPU::Mode::VBlank));
        TEST_EQ(memory.read(io_reg::REG_STAT) & 0x04, 0x04);
    }

    {
        Memory memory;
        PPU restored(memory);
        memory.set_ppu(&restored);

        std::vector<u8> state;
        restored.serialize(state);
        state[0] = static_cast<u8>(PPU::Mode::VBlank);
        state[1] = 10;
        state[2] = 0;
        state[3] = 10;

        size_t offset = 0;
        restored.deserialize(state.data(), state.size(), offset);

        std::vector<u8> reserialized;
        restored.serialize(reserialized);
        TEST_EQ(reserialized[0], static_cast<u8>(PPU::Mode::OAMSearch));
        TEST_EQ(reserialized[1], 10);
        TEST_EQ(reserialized[2], 0);
        TEST_EQ(reserialized[3], 10);
        TEST_EQ(memory.read(io_reg::REG_STAT) & 0x03, static_cast<u8>(PPU::Mode::OAMSearch));
    }

    std::cout << "  PASS: PPU state sanitizes timing state and syncs MMIO registers\n";
    return true;
}

bool test_ppu_live_mmio_writes_preserve_hardware_owned_bits() {
    std::cout << "Testing live PPU MMIO writes preserve hardware-owned bits...\n";

    Memory memory;
    PPU ppu(memory);
    memory.set_ppu(&ppu);

    memory.write(io_reg::REG_LY, 99);
    TEST_EQ(memory.read(io_reg::REG_LY), 0);

    memory.write(io_reg::REG_LYC, 1);
    memory.write(io_reg::REG_STAT, 0x03);
    TEST_EQ(memory.read(io_reg::REG_STAT) & 0x03, static_cast<u8>(PPU::Mode::OAMSearch));
    TEST_EQ(memory.read(io_reg::REG_STAT) & 0x04, 0x00);

    memory.write(io_reg::REG_STAT, 0x40);
    TEST_EQ(memory.read(io_reg::REG_IF) & 0x02, 0x00);

    memory.write(io_reg::REG_LYC, 0);
    TEST_EQ(memory.read(io_reg::REG_STAT) & 0x04, 0x04);
    TEST_EQ(memory.read(io_reg::REG_IF) & 0x02, 0x02);

    std::cout << "  PASS: Live PPU MMIO writes preserve hardware-owned bits\n";
    return true;
}

bool test_ppu_8x16_y_flip_uses_bottom_tile_first() {
    std::cout << "Testing PPU 8x16 sprite Y-flip tile selection...\n";

    Memory memory;
    PPU ppu(memory);
    memory.set_ppu(&ppu);

    memory.write(io_reg::REG_LCDC, 0x80 | 0x02 | 0x04);
    memory.write(io_reg::REG_OBP0, 0x24);

    memory.write(0x8000 + (7 * 2), 0xFF);
    memory.write(0x8000 + (7 * 2) + 1, 0x00);

    memory.write(0x8010 + (7 * 2), 0x00);
    memory.write(0x8010 + (7 * 2) + 1, 0xFF);

    memory.write(0xFE00, 16);
    memory.write(0xFE01, 8);
    memory.write(0xFE02, 0x00);
    memory.write(0xFE03, 0x40);

    ppu.step(252);

    TEST_EQ(ppu.framebuffer()[0], 2);

    std::cout << "  PASS: 8x16 Y-flip selects the bottom tile on the first scanline\n";
    return true;
}

bool test_ppu_cgb_window_uses_palette_attributes() {
    std::cout << "Testing PPU CGB window palette attributes...\n";

    std::vector<u8> rom(0x8000, 0x00);
    rom[0x0143] = 0x80;
    rom[0x0147] = 0x00;
    ROMOnly cartridge(std::move(rom));

    Memory memory;
    PPU ppu(memory);
    memory.set_ppu(&ppu);
    ppu.set_cartridge(&cartridge);

    memory.write(io_reg::REG_LCDC, 0x80 | 0x20 | 0x10 | 0x01);
    memory.write(io_reg::REG_WY, 0);
    memory.write(io_reg::REG_WX, 7);

    memory.write(io_reg::REG_VBK, 0);
    memory.write(0x9800, 0x00);

    memory.write(io_reg::REG_VBK, 1);
    memory.write(0x9800, 0x03);
    memory.write(io_reg::REG_VBK, 0);

    memory.write(0x8000, 0x80);
    memory.write(0x8001, 0x00);

    ppu.step(252);

    TEST_EQ(ppu.framebuffer()[0], static_cast<u8>((3 << 2) | 1));

    std::cout << "  PASS: CGB window rendering preserves palette attributes\n";
    return true;
}

bool test_ppu_cgb_sprite_uses_obj_palette_priority_and_vram_bank() {
    std::cout << "Testing PPU CGB sprite palette, priority, and VRAM bank handling...\n";

    std::vector<u8> rom(0x8000, 0x00);
    rom[0x0143] = 0xC0;
    rom[0x0147] = 0x00;
    ROMOnly cartridge(std::move(rom));

    Memory memory;
    PPU ppu(memory);
    memory.set_ppu(&ppu);
    ppu.set_cartridge(&cartridge);

    memory.write(io_reg::REG_LCDC, 0x80 | 0x10 | 0x02 | 0x01);
    memory.write(io_reg::REG_VBK, 0);
    memory.write(0x9800, 0x00);

    memory.write(io_reg::REG_VBK, 1);
    memory.write(0x8010, 0x80);
    memory.write(0x8011, 0x00);
    memory.write(io_reg::REG_VBK, 0);

    memory.write(0xFE00, 16);
    memory.write(0xFE01, 8);
    memory.write(0xFE02, 0x01);
    memory.write(0xFE03, 0x80 | 0x08 | 0x05);

    ppu.step(252);

    TEST_EQ(ppu.framebuffer()[0], static_cast<u8>(0x20 | (5 << 2) | 1));

    const std::vector<u8> rgba = ppu.get_rgba_framebuffer();
    TEST_EQ(rgba[0], 0x00);
    TEST_EQ(rgba[1], 0x00);
    TEST_EQ(rgba[2], 0x00);

    std::cout << "  PASS: CGB sprite rendering uses OBJ palette, priority, and VRAM bank correctly\n";
    return true;
}

int main() {
    return test_support::run_suite("gbglow PPU Tests", {
        {"ppu_mode_clamp", test_ppu_deserialize_clamps_invalid_mode},
        {"ppu_timing_sanitize", test_ppu_deserialize_sanitizes_timing_state_and_syncs_registers},
        {"ppu_mmio_live_writes", test_ppu_live_mmio_writes_preserve_hardware_owned_bits},
        {"ppu_8x16_y_flip", test_ppu_8x16_y_flip_uses_bottom_tile_first},
        {"ppu_cgb_window_palette", test_ppu_cgb_window_uses_palette_attributes},
        {"ppu_cgb_sprite_palette", test_ppu_cgb_sprite_uses_obj_palette_priority_and_vram_bank},
    });
}