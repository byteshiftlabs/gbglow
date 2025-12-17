#pragma once

#include "types.h"
#include "registers.h"
#include "memory.h"

namespace emugbc {

/**
 * Sharp LR35902 CPU Emulation
 * 
 * A modified Z80 CPU running at 4.194304 MHz (DMG) or 8.388608 MHz (CGB in double-speed mode)
 * 
 * Key differences from Z80:
 * - No shadow registers
 * - Different interrupt handling
 * - Some instructions removed/modified
 * - 256 base opcodes + 256 CB-prefixed opcodes
 */
class CPU {
public:
    explicit CPU(Memory& memory);
    
    /**
     * Execute one instruction and return the number of cycles (M-cycles) it took.
     * 1 M-cycle = 4 T-states (clock cycles)
     */
    Cycles step();
    
    /**
     * Handle interrupts
     * Checks if any interrupts are pending and enabled, then services them
     */
    void handle_interrupts();
    
    /**
     * Reset CPU to initial state
     */
    void reset();
    
    // Access registers (for debugging)
    const Registers& registers() const { return regs_; }
    Registers& registers() { return regs_; }
    
    // Interrupt Master Enable flag
    bool ime() const { return ime_; }
    void set_ime(bool value) { ime_ = value; }
    
    // Check if CPU is halted
    bool is_halted() const { return halted_; }
    
private:
    Registers regs_;
    Memory& memory_;
    
    bool ime_;      // Interrupt Master Enable
    bool halted_;   // CPU halted waiting for interrupt
    bool stopped_;  // CPU stopped (very low power)
    
    // Instruction execution
    Cycles execute_instruction(u8 opcode);
    Cycles execute_cb_instruction(u8 opcode);
    
    // Helper functions for common operations
    u8 fetch_byte();
    u16 fetch_word();
    void push_stack(u16 value);
    u16 pop_stack();
    
    // ALU operations
    void alu_add(u8 value, bool use_carry = false);
    void alu_sub(u8 value, bool use_carry = false);
    void alu_and(u8 value);
    void alu_or(u8 value);
    void alu_xor(u8 value);
    void alu_cp(u8 value);
    void alu_inc(u8& reg);
    void alu_dec(u8& reg);
};

} // namespace emugbc
