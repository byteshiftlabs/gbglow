#include "cpu.h"

namespace emugbc {

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
    // If CPU is halted, don't execute instructions but still consume cycles
    if (halted_)
    {
        return 4;
    }
    
    // Fetch opcode from memory at current PC
    u8 opcode = fetch_byte();
    
    // Execute the instruction and return cycle count
    return execute_instruction(opcode);
}

void CPU::handle_interrupts()
{
    // TODO: Implement interrupt handling mechanism
    // Priority order (highest to lowest):
    // 1. VBlank  (INT 0x40) - triggered when entering VBlank period
    // 2. LCD STAT (INT 0x48) - LCD controller status triggers
    // 3. Timer   (INT 0x50) - timer overflow
    // 4. Serial  (INT 0x58) - serial transfer complete
    // 5. Joypad  (INT 0x60) - button press
    //
    // Process:
    // - Check IME (Interrupt Master Enable)
    // - Read IF (0xFF0F) and IE (0xFFFF) registers
    // - If interrupt enabled and pending: push PC, jump to handler, clear IME
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
    bool half_carry = ((regs_.a & 0x0F) + (value & 0x0F) + carry) > 0x0F;
    
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
    bool half_carry = ((regs_.a & 0x0F) < ((value & 0x0F) + carry));
    
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
    bool half_carry = ((regs_.a & 0x0F) < (value & 0x0F));
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    regs_.set_flag(Registers::FLAG_C, result < 0);
}

void CPU::alu_inc(u8& reg)
{
    bool half_carry = ((reg & 0x0F) == 0x0F);
    reg++;
    
    regs_.set_flag(Registers::FLAG_Z, reg == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    // Carry flag is not affected
}

void CPU::alu_dec(u8& reg)
{
    bool half_carry = ((reg & 0x0F) == 0);
    reg--;
    
    regs_.set_flag(Registers::FLAG_Z, reg == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, half_carry);
    // Carry flag is not affected
}

} // namespace emugbc
