// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "test_common.h"

bool test_registers() {
    std::cout << "Testing registers...\n";

    Registers regs;
    regs.reset();

    TEST_EQ(regs.a, 0x01);
    TEST_EQ(regs.pc, static_cast<u16>(0x0100));
    TEST_EQ(regs.sp, static_cast<u16>(0xFFFE));

    regs.set_bc(0x1234);
    TEST_EQ(regs.b, 0x12);
    TEST_EQ(regs.c, 0x34);
    TEST_EQ(regs.bc(), static_cast<u16>(0x1234));

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

    mem.write(0xC000, 0x42);
    TEST_EQ(mem.read(0xC000), 0x42);

    mem.write(0xC100, 0xAB);
    TEST_EQ(mem.read(0xE100), 0xAB);

    mem.write(0xFF80, 0xCD);
    TEST_EQ(mem.read(0xFF80), 0xCD);

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

    mem.write(0xC000, 0x3E);
    mem.write(0xC001, 0x42);
    mem.write(0xC002, 0x76);

    cpu.registers().pc = 0xC000;

    Cycles cycles1 = cpu.step();
    TEST_EQ(cycles1, static_cast<Cycles>(2));
    TEST_EQ(cpu.registers().a, 0x42);
    TEST_EQ(cpu.registers().pc, static_cast<u16>(0xC002));

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

    cpu.registers().a = 0x42;
    cpu.registers().f = 0x00;
    mem.write(0xC000, 0xAF);
    mem.write(0xC001, 0xC6);
    mem.write(0xC002, 0x01);
    cpu.registers().pc = 0xC000;

    Cycles cycles = cpu.step();
    TEST_EQ(cycles, static_cast<Cycles>(1));
    TEST_EQ(cpu.registers().a, 0x00);
    TEST_ASSERT(cpu.registers().get_flag(Registers::FLAG_Z));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_N));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_H));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_C));

    cycles = cpu.step();
    TEST_EQ(cycles, static_cast<Cycles>(2));
    TEST_EQ(cpu.registers().a, 0x01);
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_Z));
    TEST_ASSERT(!cpu.registers().get_flag(Registers::FLAG_C));

    std::cout << "  PASS: CPU ALU flags working\n";
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

bool test_debugger_prepare_step_over() {
    std::cout << "Testing debugger step-over arming...\n";

    Memory memory;
    CPU cpu(memory);
    PPU ppu(memory);
    Debugger debugger;
    debugger.attach(cpu, memory, ppu);

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
    debugger.attach(cpu, memory, ppu);

    cpu.registers().pc = 0xC123;
    debugger.add_breakpoint(0xC123);

    TEST_ASSERT(debugger.should_break(0xC123));

    debugger.skip_breakpoint_once(0xC123);
    TEST_ASSERT(!debugger.should_break(0xC123));
    TEST_ASSERT(debugger.should_break(0xC123));

    std::cout << "  PASS: Debugger continue skips current breakpoint once\n";
    return true;
}

// A ROM only has to clear MIN_HEADER_SIZE (0x150 bytes) to load, which is far
// short of the 0x4000-byte fixed bank every mapper maps at 0x0000. Reading the
// whole of bank 0 from such a ROM must stay inside the buffer.
bool test_cartridge_undersized_rom_bank_0_reads_are_bounded() {
    std::cout << "Testing mapper bank 0 reads on an undersized ROM...\n";

    constexpr size_t kUndersizedRomSize = 0x150;
    constexpr u16 kRomBank0End = 0x4000;
    constexpr u8 kUnmapped = 0xFF;
    constexpr u8 kSeed = 0xAB;

    auto make_undersized = [&](u8 cartridge_type) {
        std::vector<u8> rom(kUndersizedRomSize, kSeed);
        rom[0x0147] = cartridge_type;
        rom[0x0149] = 0x00;
        return rom;
    };

    const std::vector<u8> mbc1_rom = make_undersized(0x01);
    const std::vector<u8> mbc3_rom = make_undersized(0x0F);
    const std::vector<u8> mbc5_rom = make_undersized(0x1C);

    MBC1 mbc1(mbc1_rom, 0);
    MBC3 mbc3(mbc3_rom, 0, false);
    MBC5 mbc5(mbc5_rom, 0);

    for (u16 address = 0; address < kRomBank0End; ++address) {
        const bool in_rom = address < kUndersizedRomSize;
        TEST_EQ(mbc1.read(address), in_rom ? mbc1_rom[address] : kUnmapped);
        TEST_EQ(mbc3.read(address), in_rom ? mbc3_rom[address] : kUnmapped);
        TEST_EQ(mbc5.read(address), in_rom ? mbc5_rom[address] : kUnmapped);
    }

    std::cout << "  PASS: Bank 0 reads stay within an undersized ROM buffer\n";
    return true;
}

int main() {
    return test_support::run_suite("gbglow Core Tests", {
        {"registers", test_registers},
        {"memory", test_memory},
        {"cpu_basic", test_cpu_basic},
        {"cpu_flags", test_cpu_flags},
        {"cartridge_undersized_rom", test_cartridge_undersized_rom_bank_0_reads_are_bounded},
        {"debugger_gui", test_debugger_gui_clears_execution_requests},
        {"debugger_step_over", test_debugger_prepare_step_over},
        {"debugger_continue", test_debugger_continue_skips_current_breakpoint_once},
    });
}