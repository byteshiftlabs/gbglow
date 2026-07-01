// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "cpu.h"
#include "constants.h"
#include <stdexcept>

namespace gbglow {

using namespace constants::cpu;

/**
 * Execute CB-prefixed instruction
 * 
 * These are bit manipulation instructions:
 * - RLC/RRC: Rotate left/right
 * - RL/RR: Rotate left/right through carry
 * - SLA/SRA/SRL: Shift left/right arithmetic/logical
 * - SWAP: Swap nibbles
 * - BIT: Test bit
 * - RES: Reset bit
 * - SET: Set bit
 */
Cycles CPU::execute_cb_instruction(u8 opcode)
{
    const u8 bit = (opcode >> kBitPositionShift) & kRegisterIndexMask;  // Extract bit position
    const u8 reg_index = opcode & kRegisterIndexMask;                   // Extract register index
    
    // Helper lambda to get register reference
    auto get_reg = [this](u8 idx) -> u8& {
        switch (idx)
        {
            case 0: return regs_.b;
            case 1: return regs_.c;
            case 2: return regs_.d;
            case 3: return regs_.e;
            case 4: return regs_.h;
            case 5: return regs_.l;
            case 7: return regs_.a;
            default:
                throw std::runtime_error("Invalid register index");
        }
    };
    
    // RLC - Rotate Left Circular
    if (opcode <= 0x07)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & kBit7) != 0;
            value = (value << kBitShift) | (carry ? kBit0 : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & kBit7) != 0;
            reg = (reg << kBitShift) | (carry ? kBit0 : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesMemoryRead;
        }
    }
    
    // RRC - Rotate Right Circular
    if (opcode <= 0x0F)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & kBit0) != 0;
            value = (value >> kBitShift) | (carry ? kBit7 : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & kBit0) != 0;
            reg = (reg >> kBitShift) | (carry ? kBit7 : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesMemoryRead;
        }
    }
    
    // RL - Rotate Left through Carry
    if (opcode <= 0x17)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (value & kBit7) != 0;
            value = (value << kBitShift) | (old_carry ? kBit0 : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (reg & kBit7) != 0;
            reg = (reg << kBitShift) | (old_carry ? kBit0 : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return kCyclesMemoryRead;
        }
    }
    
    // RR - Rotate Right through Carry
    if (opcode <= 0x1F)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (value & kBit0) != 0;
            value = (value >> kBitShift) | (old_carry ? kBit7 : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (reg & kBit0) != 0;
            reg = (reg >> kBitShift) | (old_carry ? kBit7 : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return kCyclesMemoryRead;
        }
    }
    
    // SLA - Shift Left Arithmetic
    if (opcode <= 0x27)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & kBit7) != 0;
            value <<= kBitShift;
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & kBit7) != 0;
            reg <<= kBitShift;
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesMemoryRead;
        }
    }
    
    // SRA - Shift Right Arithmetic (preserve sign bit)
    if (opcode <= 0x2F)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & kBit0) != 0;
            value = (value >> kBitShift) | (value & kBit7);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & kBit0) != 0;
            reg = (reg >> kBitShift) | (reg & kBit7);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesMemoryRead;
        }
    }
    
    // SWAP - Swap nibbles
    if (opcode <= 0x37)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            value = ((value & kNibbleMask) << kNibbleShift) | ((value & (kNibbleMask << kNibbleShift)) >> kNibbleShift);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, false);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            reg = ((reg & kNibbleMask) << kNibbleShift) | ((reg & (kNibbleMask << kNibbleShift)) >> kNibbleShift);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, false);
            return kCyclesMemoryRead;
        }
    }
    
    // SRL - Shift Right Logical
    if (opcode <= 0x3F)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & kBit0) != 0;
            value >>= kBitShift;
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & kBit0) != 0;
            reg >>= kBitShift;
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return kCyclesMemoryRead;
        }
    }
    
    // BIT - Test bit
    if (opcode <= 0x7F)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            bool bit_set = (value & (kBitShift << bit)) != 0;
            regs_.set_flag(Registers::FLAG_Z, !bit_set);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, true);
            return kCyclesImmediateWord;
        }
        else
        {
            u8 reg = get_reg(reg_index);
            bool bit_set = (reg & (kBitShift << bit)) != 0;
            regs_.set_flag(Registers::FLAG_Z, !bit_set);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, true);
            return kCyclesMemoryRead;
        }
    }
    
    // RES - Reset bit
    if (opcode <= 0xBF)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            value &= ~(kBitShift << bit);
            memory_.write(regs_.hl(), value);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            reg &= ~(kBitShift << bit);
            return kCyclesMemoryRead;
        }
    }
    
    // SET - Set bit (opcode 0xC0-0xFF)
    {
        if (reg_index == kMemoryHlRegisterIndex)
        {
            u8 value = memory_.read(regs_.hl());
            value |= (kBitShift << bit);
            memory_.write(regs_.hl(), value);
            return kCyclesPush;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            reg |= (kBitShift << bit);
            return kCyclesMemoryRead;
        }
    }
    
    return kCyclesMemoryRead;  // Shouldn't reach here
}

} // namespace gbglow
