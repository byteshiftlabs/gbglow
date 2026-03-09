// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "cpu.h"
#include "cpu_constants.h"
#include <stdexcept>

namespace gbglow {

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
    const u8 bit = (opcode >> BIT_POSITION_SHIFT) & REGISTER_INDEX_MASK;  // Extract bit position
    const u8 reg_index = opcode & REGISTER_INDEX_MASK;                     // Extract register index
    
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
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & BIT_7) != 0;
            value = (value << BIT_SHIFT) | (carry ? BIT_SHIFT : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & BIT_7) != 0;
            reg = (reg << BIT_SHIFT) | (carry ? BIT_SHIFT : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // RRC - Rotate Right Circular
    if (opcode >= 0x08 && opcode <= 0x0F)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & BIT_0) != 0;
            value = (value >> BIT_SHIFT) | (carry ? BIT_7 : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & BIT_0) != 0;
            reg = (reg >> BIT_SHIFT) | (carry ? BIT_7 : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // RL - Rotate Left through Carry
    if (opcode >= 0x10 && opcode <= 0x17)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (value & BIT_7) != 0;
            value = (value << BIT_SHIFT) | (old_carry ? BIT_SHIFT : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (reg & BIT_7) != 0;
            reg = (reg << BIT_SHIFT) | (old_carry ? BIT_SHIFT : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // RR - Rotate Right through Carry
    if (opcode >= 0x18 && opcode <= 0x1F)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (value & BIT_0) != 0;
            value = (value >> BIT_SHIFT) | (old_carry ? BIT_7 : 0);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (reg & BIT_0) != 0;
            reg = (reg >> BIT_SHIFT) | (old_carry ? BIT_7 : 0);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // SLA - Shift Left Arithmetic
    if (opcode >= 0x20 && opcode <= 0x27)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & BIT_7) != 0;
            value <<= BIT_SHIFT;
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & BIT_7) != 0;
            reg <<= BIT_SHIFT;
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // SRA - Shift Right Arithmetic (preserve sign bit)
    if (opcode >= 0x28 && opcode <= 0x2F)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & BIT_0) != 0;
            value = (value >> BIT_SHIFT) | (value & BIT_7);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & BIT_0) != 0;
            reg = (reg >> BIT_SHIFT) | (reg & BIT_7);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // SWAP - Swap nibbles
    if (opcode >= 0x30 && opcode <= 0x37)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            value = ((value & NIBBLE_MASK) << NIBBLE_SHIFT) | ((value & (NIBBLE_MASK << NIBBLE_SHIFT)) >> NIBBLE_SHIFT);
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, false);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            reg = ((reg & NIBBLE_MASK) << NIBBLE_SHIFT) | ((reg & (NIBBLE_MASK << NIBBLE_SHIFT)) >> NIBBLE_SHIFT);
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, false);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // SRL - Shift Right Logical
    if (opcode >= 0x38 && opcode <= 0x3F)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool carry = (value & BIT_0) != 0;
            value >>= BIT_SHIFT;
            memory_.write(regs_.hl(), value);
            regs_.set_flag(Registers::FLAG_Z, value == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            bool carry = (reg & BIT_0) != 0;
            reg >>= BIT_SHIFT;
            regs_.set_flag(Registers::FLAG_Z, reg == 0);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // BIT - Test bit
    if (opcode >= 0x40 && opcode <= 0x7F)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            bool bit_set = (value & (BIT_SHIFT << bit)) != 0;
            regs_.set_flag(Registers::FLAG_Z, !bit_set);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, true);
            return CYCLES_IMMEDIATE_WORD;
        }
        else
        {
            u8 reg = get_reg(reg_index);
            bool bit_set = (reg & (BIT_SHIFT << bit)) != 0;
            regs_.set_flag(Registers::FLAG_Z, !bit_set);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, true);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // RES - Reset bit
    if (opcode >= 0x80 && opcode <= 0xBF)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            value &= ~(BIT_SHIFT << bit);
            memory_.write(regs_.hl(), value);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            reg &= ~(BIT_SHIFT << bit);
            return CYCLES_MEMORY_READ;
        }
    }
    
    // SET - Set bit
    if (opcode >= 0xC0)
    {
        if (reg_index == MEMORY_HL_REGISTER_INDEX)
        {
            u8 value = memory_.read(regs_.hl());
            value |= (BIT_SHIFT << bit);
            memory_.write(regs_.hl(), value);
            return CYCLES_PUSH;
        }
        else
        {
            u8& reg = get_reg(reg_index);
            reg |= (BIT_SHIFT << bit);
            return CYCLES_MEMORY_READ;
        }
    }
    
    return CYCLES_MEMORY_READ;  // Shouldn't reach here
}

} // namespace gbglow
