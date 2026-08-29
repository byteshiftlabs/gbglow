// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "test_common.h"

// Instruction-level coverage for the parts of the decoder where an emulator is
// most often subtly wrong: the flag side effects of the ALU, and the CB-prefixed
// bit block. Expected values here are derived from the documented behaviour of
// the LR35902, not read back from the implementation, so a wrong result is a
// real disagreement rather than a restatement.

namespace {

constexpr u16 kCodeStart = 0xC000;

struct AluCase {
    const char* name;
    u8 opcode;       // immediate-operand form, so the operand is inline
    u8 operand;
    u8 initial_a;
    bool initial_carry;
    u8 expected_a;
    bool z, n, h, c;
};

// Runs one instruction at kCodeStart and returns the resulting registers.
Registers run_one(std::initializer_list<u8> code, u8 initial_a, bool initial_carry) {
    Memory memory;
    CPU cpu(memory);
    cpu.reset();

    u16 address = kCodeStart;
    for (u8 byte : code) {
        memory.write(address++, byte);
    }

    cpu.registers().a = initial_a;
    cpu.registers().f = 0;
    cpu.registers().set_flag(Registers::FLAG_C, initial_carry);
    cpu.registers().pc = kCodeStart;

    cpu.step();
    return cpu.registers();
}

bool check_flags(const char* name, const Registers& regs, u8 expected_a,
                 bool z, bool n, bool h, bool c) {
    bool ok = true;
    auto report = [&](const char* flag, bool actual, bool expected) {
        if (actual != expected) {
            std::cerr << "  FAIL: " << name << ": flag " << flag << " was "
                      << actual << ", expected " << expected << "\n";
            ok = false;
        }
    };

    if (regs.a != expected_a) {
        std::cerr << "  FAIL: " << name << ": A was 0x" << std::hex << +regs.a
                  << ", expected 0x" << +expected_a << std::dec << "\n";
        ok = false;
    }
    report("Z", regs.get_flag(Registers::FLAG_Z), z);
    report("N", regs.get_flag(Registers::FLAG_N), n);
    report("H", regs.get_flag(Registers::FLAG_H), h);
    report("C", regs.get_flag(Registers::FLAG_C), c);
    return ok;
}

}  // namespace

// The half-carry flag is the one most often got wrong: it reports a carry out of
// bit 3, which is independent of the carry out of bit 7.
bool test_alu_immediate_flag_matrix() {
    std::cout << "Testing ALU immediate flag matrix...\n";

    const AluCase cases[] = {
        // ADD A,d8 (0xC6)
        {"ADD 0x0F+0x01 half-carry", 0xC6, 0x01, 0x0F, false, 0x10, false, false, true,  false},
        {"ADD 0xFF+0x01 wraps",      0xC6, 0x01, 0xFF, false, 0x00, true,  false, true,  true },
        {"ADD 0x00+0x00 zero",       0xC6, 0x00, 0x00, false, 0x00, true,  false, false, false},
        {"ADD carry-in ignored",     0xC6, 0x01, 0x01, true,  0x02, false, false, false, false},
        // ADC A,d8 (0xCE) - carry-in participates
        {"ADC 0x0F+0x00+C",          0xCE, 0x00, 0x0F, true,  0x10, false, false, true,  false},
        {"ADC 0xFF+0x00+C wraps",    0xCE, 0x00, 0xFF, true,  0x00, true,  false, true,  true },
        // SUB d8 (0xD6) - N is set, H is a borrow out of bit 4
        {"SUB 0x10-0x01 borrow",     0xD6, 0x01, 0x10, false, 0x0F, false, true,  true,  false},
        {"SUB 0x00-0x01 underflow",  0xD6, 0x01, 0x00, false, 0xFF, false, true,  true,  true },
        {"SUB equal gives zero",     0xD6, 0x42, 0x42, false, 0x00, true,  true,  false, false},
        // SBC A,d8 (0xDE)
        {"SBC 0x10-0x00-C",          0xDE, 0x00, 0x10, true,  0x0F, false, true,  true,  false},
        // AND/OR/XOR fix H and C to constants, which is easy to get wrong
        {"AND sets H clears C",      0xE6, 0x0F, 0xF0, true,  0x00, true,  false, true,  false},
        {"AND non-zero",             0xE6, 0x3C, 0xFF, false, 0x3C, false, false, true,  false},
        {"OR clears H and C",        0xF6, 0x0F, 0xF0, true,  0xFF, false, false, false, false},
        {"XOR self is zero",         0xEE, 0xFF, 0xFF, true,  0x00, true,  false, false, false},
        // CP d8 (0xFE) - a SUB that discards the result but keeps the flags.
        // 0x01 - 0x10 borrows out of the byte but not out of bit 3, since the
        // low nibbles are 1 and 0, so C is set while H stays clear.
        {"CP equal",                 0xFE, 0x42, 0x42, false, 0x42, true,  true,  false, false},
        {"CP smaller operand",       0xFE, 0x01, 0x10, false, 0x10, false, true,  true,  false},
        {"CP larger operand",        0xFE, 0x10, 0x01, false, 0x01, false, true,  false, true },
    };

    bool all_ok = true;
    for (const AluCase& c : cases) {
        const Registers regs = run_one({c.opcode, c.operand}, c.initial_a, c.initial_carry);
        ++test_support::tests_run;
        if (check_flags(c.name, regs, c.expected_a, c.z, c.n, c.h, c.c)) {
            ++test_support::tests_passed;
        } else {
            all_ok = false;
        }
    }

    if (!all_ok) {
        return false;
    }
    std::cout << "  PASS: ALU immediate forms set Z/N/H/C correctly\n";
    return true;
}

// INC and DEC touch Z, N and H but must leave the carry flag exactly as it was.
bool test_inc_dec_preserve_carry() {
    std::cout << "Testing INC/DEC flag behaviour...\n";

    // INC A (0x3C) on 0x0F carries out of bit 3.
    Registers regs = run_one({0x3C}, 0x0F, true);
    TEST_ASSERT(regs.a == 0x10);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_N));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_H));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));   // preserved

    // INC A on 0xFF wraps to zero without touching carry.
    regs = run_one({0x3C}, 0xFF, false);
    TEST_ASSERT(regs.a == 0x00);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_H));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_C));  // preserved

    // DEC A (0x3D) on 0x10 borrows into bit 4 and sets N.
    regs = run_one({0x3D}, 0x10, false);
    TEST_ASSERT(regs.a == 0x0F);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_N));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_H));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_C));

    // DEC A on 0x01 reaches zero.
    regs = run_one({0x3D}, 0x01, true);
    TEST_ASSERT(regs.a == 0x00);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_N));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_H));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));   // preserved

    std::cout << "  PASS: INC/DEC set Z/N/H and leave C untouched\n";
    return true;
}

// The CB block is 256 opcodes of rotate, shift and bit work. These cases pin the
// boundary behaviour of each family rather than every register variant.
bool test_cb_rotate_shift_and_bit() {
    std::cout << "Testing CB-prefixed rotate, shift and bit operations...\n";

    // SWAP A (CB 0x37) exchanges the nibbles and clears N, H and C.
    Registers regs = run_one({0xCB, 0x37}, 0xAB, true);
    TEST_ASSERT(regs.a == 0xBA);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_C));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_H));

    // SWAP of zero is still zero, and sets Z.
    regs = run_one({0xCB, 0x37}, 0x00, false);
    TEST_ASSERT(regs.a == 0x00);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));

    // RL A (CB 0x17) rotates through carry: bit 7 leaves, old carry enters bit 0.
    regs = run_one({0xCB, 0x17}, 0x80, false);
    TEST_ASSERT(regs.a == 0x00);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));

    regs = run_one({0xCB, 0x17}, 0x80, true);
    TEST_ASSERT(regs.a == 0x01);          // carry rotated in
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));

    // RLC A (CB 0x07) rotates bit 7 straight back into bit 0, not through carry.
    regs = run_one({0xCB, 0x07}, 0x80, false);
    TEST_ASSERT(regs.a == 0x01);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));

    // SRL A (CB 0x3F) is a logical shift: bit 7 becomes zero.
    regs = run_one({0xCB, 0x3F}, 0x01, false);
    TEST_ASSERT(regs.a == 0x00);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));

    regs = run_one({0xCB, 0x3F}, 0x80, false);
    TEST_ASSERT(regs.a == 0x40);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_C));

    // SRA A (CB 0x2F) is arithmetic: bit 7 is preserved.
    regs = run_one({0xCB, 0x2F}, 0x80, false);
    TEST_ASSERT(regs.a == 0xC0);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_C));

    // SLA A (CB 0x27) shifts left, zero into bit 0.
    regs = run_one({0xCB, 0x27}, 0x80, false);
    TEST_ASSERT(regs.a == 0x00);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));

    // BIT 7,A (CB 0x7F) sets Z from the complement of the bit, always sets H,
    // always clears N, and leaves C alone.
    regs = run_one({0xCB, 0x7F}, 0x80, true);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_H));
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_N));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));   // preserved
    TEST_ASSERT(regs.a == 0x80);                     // BIT does not modify A

    regs = run_one({0xCB, 0x7F}, 0x00, false);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_Z));
    TEST_ASSERT(regs.get_flag(Registers::FLAG_H));

    // RES 0,A (CB 0x87) and SET 0,A (CB 0xC7) touch no flags.
    regs = run_one({0xCB, 0x87}, 0xFF, true);
    TEST_ASSERT(regs.a == 0xFE);
    TEST_ASSERT(regs.get_flag(Registers::FLAG_C));

    regs = run_one({0xCB, 0xC7}, 0x00, false);
    TEST_ASSERT(regs.a == 0x01);
    TEST_ASSERT(!regs.get_flag(Registers::FLAG_C));

    std::cout << "  PASS: CB rotate, shift and bit operations behave correctly\n";
    return true;
}

int main() {
    return test_support::run_suite("gbglow Instruction Tests", {
        {"alu_immediate_flags", test_alu_immediate_flag_matrix},
        {"inc_dec_flags", test_inc_dec_preserve_carry},
        {"cb_rotate_shift_bit", test_cb_rotate_shift_and_bit},
    });
}
