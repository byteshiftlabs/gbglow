// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "../src/core/cpu.h"
#include "../src/core/memory.h"
#include "../src/core/registers.h"
#include <iostream>
#include <sstream>
#include <string>

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

int main() {
    std::cout << "=================================\n";
    std::cout << "gbglow Basic Component Tests\n";
    std::cout << "=================================\n\n";
    
    bool all_passed = true;
    all_passed &= test_registers();
    all_passed &= test_memory();
    all_passed &= test_cpu_basic();
    all_passed &= test_cpu_flags();
    
    std::cout << "\n" << tests_passed << "/" << tests_run << " assertions passed.\n";
    
    if (all_passed) {
        std::cout << "All tests passed!\n";
        return 0;
    } else {
        std::cerr << "Some tests FAILED.\n";
        return 1;
    }
}
