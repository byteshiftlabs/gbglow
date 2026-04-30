// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "cpu.h"
#include "cpu_constants.h"
#include <iostream>

namespace gbglow {

CPU::CPU(Memory& memory)
    : memory_(memory)
    , ime_(false)
    , halted_(false)
    , stopped_(false)
{
    reset();
}

Cycles CPU::step()
{
    // Check and handle interrupts before executing next instruction
    handle_interrupts();
    
    // If CPU is halted, don't execute instructions but still consume cycles
    if (halted_)
    {
        return CYCLES_REGISTER_OP;
    }
    
    // Fetch opcode from memory at current PC
    u8 opcode = fetch_byte();
    
    // Execute the instruction and return cycle count
    return execute_instruction(opcode);
}

void CPU::handle_interrupts()
{
    // Interrupts are only handled if IME (Interrupt Master Enable) is set
    // Exception: HALT mode is exited even if IME is disabled
    
    // Read interrupt flags (IF) and interrupt enable (IE) registers
    u8 interrupt_flags = memory_.read(REG_INTERRUPT_FLAG);
    u8 interrupt_enable = memory_.read(REG_INTERRUPT_ENABLE);
    
    // Calculate which interrupts are both pending (IF) and enabled (IE)
    u8 triggered_interrupts = interrupt_flags & interrupt_enable;
    
    // If any interrupt is triggered, wake from HALT even if IME is disabled
    if (triggered_interrupts != 0 && halted_)
    {
        halted_ = false;
    }
    
    // Only service interrupts if IME is enabled
    if (!ime_ || triggered_interrupts == 0)
    {
        return;
    }
    
    // Check interrupts in priority order (bit 0 = highest priority)
    // Service the highest priority interrupt that is both flagged and enabled
    
    u8 interrupt_bit = 0;
    u16 interrupt_vector = 0;
    
    if (triggered_interrupts & INT_VBLANK_BIT)
    {
        interrupt_bit = INT_VBLANK_BIT;
        interrupt_vector = INT_VBLANK_VECTOR;
    }
    else if (triggered_interrupts & INT_LCD_STAT_BIT)
    {
        interrupt_bit = INT_LCD_STAT_BIT;
        interrupt_vector = INT_LCD_STAT_VECTOR;
    }
    else if (triggered_interrupts & INT_TIMER_BIT)
    {
        interrupt_bit = INT_TIMER_BIT;
        interrupt_vector = INT_TIMER_VECTOR;
    }
    else if (triggered_interrupts & INT_SERIAL_BIT)
    {
        interrupt_bit = INT_SERIAL_BIT;
        interrupt_vector = INT_SERIAL_VECTOR;
    }
    else if (triggered_interrupts & INT_JOYPAD_BIT)
    {
        interrupt_bit = INT_JOYPAD_BIT;
        interrupt_vector = INT_JOYPAD_VECTOR;
    }
    else
    {
        return;  // No interrupt to service
    }
    
    // Service the interrupt:
    // 1. Disable IME to prevent nested interrupts
    ime_ = false;
    
    // 2. Clear the interrupt flag for the serviced interrupt
    memory_.write(REG_INTERRUPT_FLAG, interrupt_flags & ~interrupt_bit);
    
    // 3. Push current PC onto stack
    push_stack(regs_.pc);
    
    // 4. Jump to interrupt vector
    regs_.pc = interrupt_vector;
}

void CPU::reset()
{
    regs_.reset();
    ime_ = false;
    halted_ = false;
    stopped_ = false;
}

const Registers& CPU::registers() const
{
    return regs_;
}

Registers& CPU::registers()
{
    return regs_;
}

bool CPU::ime() const
{
    return ime_;
}

void CPU::set_ime(bool value)
{
    ime_ = value;
}

bool CPU::is_halted() const
{
    return halted_;
}

void CPU::request_interrupt(u8 interrupt_bit)
{
    // Set the corresponding bit in the IF register to request an interrupt
    u8 interrupt_flags = memory_.read(REG_INTERRUPT_FLAG);
    memory_.write(REG_INTERRUPT_FLAG, interrupt_flags | interrupt_bit);
}

// ============================================================================
// Helper Functions
// ============================================================================

u8 CPU::fetch_byte()
{
    u8 byte = memory_.read(regs_.pc);
    regs_.pc++;
    return byte;
}

u16 CPU::fetch_word()
{
    u16 low = fetch_byte();
    u16 high = fetch_byte();
    return (high << 8) | low;
}

void CPU::push_stack(u16 value)
{
    regs_.sp -= 2;
    memory_.write16(regs_.sp, value);
}

u16 CPU::pop_stack()
{
    u16 value = memory_.read16(regs_.sp);
    regs_.sp += 2;
    return value;
}

// ============================================================================
// ALU Operations
// ============================================================================

void CPU::alu_add(u8 value, bool use_carry)
{
    u8 carry = (use_carry && regs_.get_flag(Registers::FLAG_C)) ? 1 : 0;
    u16 result = regs_.a + value + carry;
    
    // Half-carry: check if carry from bit 3 to bit 4
    bool half_carry = ((regs_.a & NIBBLE_MASK) + (value & NIBBLE_MASK) + carry) > NIBBLE_MASK;
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    regs_.set_flag(Registers::FLAG_C, result > 0xFF);
    
    regs_.a = result & 0xFF;
}

void CPU::alu_sub(u8 value, bool use_carry)
{
    u8 carry = (use_carry && regs_.get_flag(Registers::FLAG_C)) ? 1 : 0;
    int result = regs_.a - value - carry;
    
    // Half-carry: check if borrow from bit 4
    bool half_carry = ((regs_.a & NIBBLE_MASK) < ((value & NIBBLE_MASK) + carry));
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    regs_.set_flag(Registers::FLAG_C, result < 0);
    
    regs_.a = result & 0xFF;
}

void CPU::alu_and(u8 value)
{
    regs_.a &= value;
    regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, true);
    regs_.set_flag(Registers::FLAG_C, false);
}

void CPU::alu_or(u8 value)
{
    regs_.a |= value;
    regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, false);
    regs_.set_flag(Registers::FLAG_C, false);
}

void CPU::alu_xor(u8 value)
{
    regs_.a ^= value;
    regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, false);
    regs_.set_flag(Registers::FLAG_C, false);
}

void CPU::alu_cp(u8 value)
{
    // Compare is like subtraction but doesn't store result
    int result = regs_.a - value;
    bool half_carry = ((regs_.a & NIBBLE_MASK) < (value & NIBBLE_MASK));
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    regs_.set_flag(Registers::FLAG_C, result < 0);
}

void CPU::alu_inc(u8& reg)
{
    bool half_carry = ((reg & NIBBLE_MASK) == NIBBLE_MASK);
    reg++;
    
    regs_.set_flag(Registers::FLAG_Z, reg == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    // Carry flag is not affected
}

void CPU::alu_dec(u8& reg)
{
    bool half_carry = ((reg & NIBBLE_MASK) == 0);
    reg--;
    
    regs_.set_flag(Registers::FLAG_Z, reg == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    // Carry flag is not affected
}

void CPU::set_halted(bool value)
{
    halted_ = value;
}

// ============================================================================
// Serialization for Save States
// ============================================================================

void CPU::serialize(std::vector<u8>& data) const
{
    // Registers (12 bytes total)
    data.push_back(regs_.a);
    data.push_back(regs_.f);
    data.push_back(regs_.b);
    data.push_back(regs_.c);
    data.push_back(regs_.d);
    data.push_back(regs_.e);
    data.push_back(regs_.h);
    data.push_back(regs_.l);
    data.push_back(static_cast<u8>(regs_.sp & 0xFF));
    data.push_back(static_cast<u8>((regs_.sp >> 8) & 0xFF));
    data.push_back(static_cast<u8>(regs_.pc & 0xFF));
    data.push_back(static_cast<u8>((regs_.pc >> 8) & 0xFF));
    
    // CPU flags (3 bytes)
    data.push_back(ime_ ? 1 : 0);
    data.push_back(halted_ ? 1 : 0);
    data.push_back(stopped_ ? 1 : 0);
}

void CPU::deserialize(const u8* data, size_t data_size, size_t& offset)
{
    constexpr size_t REGISTER_BYTES = 8;   // a, f, b, c, d, e, h, l
    constexpr size_t SP_BYTES = 2;
    constexpr size_t PC_BYTES = 2;
    constexpr size_t FLAG_BYTES = 3;        // ime, halted, stopped
    constexpr size_t CPU_STATE_SIZE = REGISTER_BYTES + SP_BYTES + PC_BYTES + FLAG_BYTES;
    if (offset + CPU_STATE_SIZE > data_size) return;

    // Registers
    regs_.a = data[offset++];
    regs_.f = data[offset++];
    regs_.b = data[offset++];
    regs_.c = data[offset++];
    regs_.d = data[offset++];
    regs_.e = data[offset++];
    regs_.h = data[offset++];
    regs_.l = data[offset++];
    regs_.sp = static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8);
    offset += 2;
    regs_.pc = static_cast<u16>(data[offset]) | (static_cast<u16>(data[offset + 1]) << 8);
    offset += 2;
    
    // CPU flags
    ime_ = data[offset++] != 0;
    halted_ = data[offset++] != 0;
    stopped_ = data[offset++] != 0;
}

} // namespace gbglow
