// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "cpu.h"
#include "cpu_constants.h"
#include <stdexcept>

namespace gbglow {

// CGB register addresses
namespace {
    constexpr u16 CGB_KEY1 = 0xFF4D;  // Speed switch register
    constexpr u8 CGB_SPEED_PREPARE_BIT = 0x01;  // KEY1 bit 0: prepare speed switch
    constexpr u8 CGB_SPEED_TOGGLE_BIT = 0x80;   // KEY1 bit 7: current speed (toggle)
}

/**
 * Execute a single CPU instruction
 * 
 * This implements all 256 base opcodes for the Sharp LR35902 CPU.
 * Returns the number of M-cycles the instruction took.
 * 
 * Instruction groups:
 * - 0x00-0x3F: Misc, loads, ALU
 * - 0x40-0x7F: 8-bit loads
 * - 0x80-0xBF: 8-bit ALU
 * - 0xC0-0xFF: Control flow, stack ops, I/O
 */
Cycles CPU::execute_instruction(u8 opcode)
{
    switch (opcode)
    {
        // ====================================================================
        // Misc / Control
        // ====================================================================
        
        case 0x00:  // NOP
            return 1;
            
        case 0x10:  // STOP
            stopped_ = true;
            fetch_byte();  // STOP is 2 bytes
            
            // CGB: Handle speed switch if KEY1 bit 0 is set
            {
                u8 key1 = memory_.read(CGB_KEY1);
                if (key1 & CGB_SPEED_PREPARE_BIT) {
                    // Toggle speed (bit 7) and clear prepare bit (bit 0)
                    u8 new_speed = (key1 ^ CGB_SPEED_TOGGLE_BIT) & CGB_SPEED_TOGGLE_BIT;
                    memory_.write(CGB_KEY1, new_speed);
                }
            }
            
            return 1;
            
        case 0x76:  // HALT
            halted_ = true;
            return 1;
            
        case 0xF3:  // DI - Disable interrupts
            ime_ = false;
            return 1;
            
        case 0xFB:  // EI - Enable interrupts
            ime_ = true;
            return 1;
            
        // ====================================================================
        // 8-bit Loads: LD r,r (reg to reg)
        // ====================================================================
        
        // LD B,r
        // cppcheck-suppress selfAssignment  ; LD B,B is a hardware NOP
        case 0x40: regs_.b = regs_.b; return 1;
        case 0x41: regs_.b = regs_.c; return 1;
        case 0x42: regs_.b = regs_.d; return 1;
        case 0x43: regs_.b = regs_.e; return 1;
        case 0x44: regs_.b = regs_.h; return 1;
        case 0x45: regs_.b = regs_.l; return 1;
        case 0x47: regs_.b = regs_.a; return 1;
        
        // LD C,r
        case 0x48: regs_.c = regs_.b; return 1;
        // cppcheck-suppress selfAssignment  ; LD C,C is a hardware NOP
        case 0x49: regs_.c = regs_.c; return 1;
        case 0x4A: regs_.c = regs_.d; return 1;
        case 0x4B: regs_.c = regs_.e; return 1;
        case 0x4C: regs_.c = regs_.h; return 1;
        case 0x4D: regs_.c = regs_.l; return 1;
        case 0x4F: regs_.c = regs_.a; return 1;
        
        // LD D,r
        case 0x50: regs_.d = regs_.b; return 1;
        case 0x51: regs_.d = regs_.c; return 1;
        // cppcheck-suppress selfAssignment  ; LD D,D is a hardware NOP
        case 0x52: regs_.d = regs_.d; return 1;
        case 0x53: regs_.d = regs_.e; return 1;
        case 0x54: regs_.d = regs_.h; return 1;
        case 0x55: regs_.d = regs_.l; return 1;
        case 0x57: regs_.d = regs_.a; return 1;
        
        // LD E,r
        case 0x58: regs_.e = regs_.b; return 1;
        case 0x59: regs_.e = regs_.c; return 1;
        case 0x5A: regs_.e = regs_.d; return 1;
        // cppcheck-suppress selfAssignment  ; LD E,E is a hardware NOP
        case 0x5B: regs_.e = regs_.e; return 1;
        case 0x5C: regs_.e = regs_.h; return 1;
        case 0x5D: regs_.e = regs_.l; return 1;
        case 0x5F: regs_.e = regs_.a; return 1;
        
        // LD H,r
        case 0x60: regs_.h = regs_.b; return 1;
        case 0x61: regs_.h = regs_.c; return 1;
        case 0x62: regs_.h = regs_.d; return 1;
        case 0x63: regs_.h = regs_.e; return 1;
        // cppcheck-suppress selfAssignment  ; LD H,H is a hardware NOP
        case 0x64: regs_.h = regs_.h; return 1;
        case 0x65: regs_.h = regs_.l; return 1;
        case 0x67: regs_.h = regs_.a; return 1;
        
        // LD L,r
        case 0x68: regs_.l = regs_.b; return 1;
        case 0x69: regs_.l = regs_.c; return 1;
        case 0x6A: regs_.l = regs_.d; return 1;
        case 0x6B: regs_.l = regs_.e; return 1;
        case 0x6C: regs_.l = regs_.h; return 1;
        // cppcheck-suppress selfAssignment  ; LD L,L is a hardware NOP
        case 0x6D: regs_.l = regs_.l; return 1;
        case 0x6F: regs_.l = regs_.a; return 1;
        
        // LD A,r
        case 0x78: regs_.a = regs_.b; return 1;
        case 0x79: regs_.a = regs_.c; return 1;
        case 0x7A: regs_.a = regs_.d; return 1;
        case 0x7B: regs_.a = regs_.e; return 1;
        case 0x7C: regs_.a = regs_.h; return 1;
        case 0x7D: regs_.a = regs_.l; return 1;
        // cppcheck-suppress selfAssignment  ; LD A,A is a hardware NOP
        case 0x7F: regs_.a = regs_.a; return 1;
        
        // ====================================================================
        // 8-bit Loads: LD r,(HL) - Load from memory
        // ====================================================================
        
        case 0x46: regs_.b = memory_.read(regs_.hl()); return 2;
        case 0x4E: regs_.c = memory_.read(regs_.hl()); return 2;
        case 0x56: regs_.d = memory_.read(regs_.hl()); return 2;
        case 0x5E: regs_.e = memory_.read(regs_.hl()); return 2;
        case 0x66: regs_.h = memory_.read(regs_.hl()); return 2;
        case 0x6E: regs_.l = memory_.read(regs_.hl()); return 2;
        case 0x7E: regs_.a = memory_.read(regs_.hl()); return 2;
        
        // ====================================================================
        // 8-bit Loads: LD (HL),r - Store to memory
        // ====================================================================
        
        case 0x70: memory_.write(regs_.hl(), regs_.b); return 2;
        case 0x71: memory_.write(regs_.hl(), regs_.c); return 2;
        case 0x72: memory_.write(regs_.hl(), regs_.d); return 2;
        case 0x73: memory_.write(regs_.hl(), regs_.e); return 2;
        case 0x74: memory_.write(regs_.hl(), regs_.h); return 2;
        case 0x75: memory_.write(regs_.hl(), regs_.l); return 2;
        case 0x77: memory_.write(regs_.hl(), regs_.a); return 2;
        
        // ====================================================================
        // 8-bit Loads: LD r,n - Load immediate
        // ====================================================================
        
        case 0x06: regs_.b = fetch_byte(); return 2;
        case 0x0E: regs_.c = fetch_byte(); return 2;
        case 0x16: regs_.d = fetch_byte(); return 2;
        case 0x1E: regs_.e = fetch_byte(); return 2;
        case 0x26: regs_.h = fetch_byte(); return 2;
        case 0x2E: regs_.l = fetch_byte(); return 2;
        case 0x3E: regs_.a = fetch_byte(); return 2;
        
        case 0x36:  // LD (HL),n
            memory_.write(regs_.hl(), fetch_byte());
            return 3;
            
        // ====================================================================
        // 16-bit Loads: LD rr,nn
        // ====================================================================
        
        case 0x01: regs_.set_bc(fetch_word()); return 3;
        case 0x11: regs_.set_de(fetch_word()); return 3;
        case 0x21: regs_.set_hl(fetch_word()); return 3;
        case 0x31: regs_.sp = fetch_word(); return 3;
        
        // ====================================================================
        // Stack Operations: PUSH/POP
        // ====================================================================
        
        case 0xC5: push_stack(regs_.bc()); return 4;
        case 0xD5: push_stack(regs_.de()); return 4;
        case 0xE5: push_stack(regs_.hl()); return 4;
        case 0xF5: push_stack(regs_.af()); return 4;
        
        case 0xC1: regs_.set_bc(pop_stack()); return 3;
        case 0xD1: regs_.set_de(pop_stack()); return 3;
        case 0xE1: regs_.set_hl(pop_stack()); return 3;
        case 0xF1: regs_.set_af(pop_stack()); return 3;
        
        // ====================================================================
        // 8-bit ALU: ADD A,r
        // ====================================================================
        
        case 0x80: alu_add(regs_.b); return 1;
        case 0x81: alu_add(regs_.c); return 1;
        case 0x82: alu_add(regs_.d); return 1;
        case 0x83: alu_add(regs_.e); return 1;
        case 0x84: alu_add(regs_.h); return 1;
        case 0x85: alu_add(regs_.l); return 1;
        case 0x87: alu_add(regs_.a); return 1;
        case 0x86: alu_add(memory_.read(regs_.hl())); return 2;
        case 0xC6: alu_add(fetch_byte()); return 2;
        
        // ADC A,r (Add with carry)
        case 0x88: alu_add(regs_.b, true); return 1;
        case 0x89: alu_add(regs_.c, true); return 1;
        case 0x8A: alu_add(regs_.d, true); return 1;
        case 0x8B: alu_add(regs_.e, true); return 1;
        case 0x8C: alu_add(regs_.h, true); return 1;
        case 0x8D: alu_add(regs_.l, true); return 1;
        case 0x8F: alu_add(regs_.a, true); return 1;
        case 0x8E: alu_add(memory_.read(regs_.hl()), true); return 2;
        case 0xCE: alu_add(fetch_byte(), true); return 2;
        
        // ====================================================================
        // 8-bit ALU: SUB A,r
        // ====================================================================
        
        case 0x90: alu_sub(regs_.b); return 1;
        case 0x91: alu_sub(regs_.c); return 1;
        case 0x92: alu_sub(regs_.d); return 1;
        case 0x93: alu_sub(regs_.e); return 1;
        case 0x94: alu_sub(regs_.h); return 1;
        case 0x95: alu_sub(regs_.l); return 1;
        case 0x97: alu_sub(regs_.a); return 1;
        case 0x96: alu_sub(memory_.read(regs_.hl())); return 2;
        case 0xD6: alu_sub(fetch_byte()); return 2;
        
        // SBC A,r (Subtract with carry)
        case 0x98: alu_sub(regs_.b, true); return 1;
        case 0x99: alu_sub(regs_.c, true); return 1;
        case 0x9A: alu_sub(regs_.d, true); return 1;
        case 0x9B: alu_sub(regs_.e, true); return 1;
        case 0x9C: alu_sub(regs_.h, true); return 1;
        case 0x9D: alu_sub(regs_.l, true); return 1;
        case 0x9F: alu_sub(regs_.a, true); return 1;
        case 0x9E: alu_sub(memory_.read(regs_.hl()), true); return 2;
        case 0xDE: alu_sub(fetch_byte(), true); return 2;
        
        // ====================================================================
        // 8-bit ALU: AND, OR, XOR, CP
        // ====================================================================
        
        // AND
        case 0xA0: alu_and(regs_.b); return 1;
        case 0xA1: alu_and(regs_.c); return 1;
        case 0xA2: alu_and(regs_.d); return 1;
        case 0xA3: alu_and(regs_.e); return 1;
        case 0xA4: alu_and(regs_.h); return 1;
        case 0xA5: alu_and(regs_.l); return 1;
        case 0xA7: alu_and(regs_.a); return 1;
        case 0xA6: alu_and(memory_.read(regs_.hl())); return 2;
        case 0xE6: alu_and(fetch_byte()); return 2;
        
        // XOR
        case 0xA8: alu_xor(regs_.b); return 1;
        case 0xA9: alu_xor(regs_.c); return 1;
        case 0xAA: alu_xor(regs_.d); return 1;
        case 0xAB: alu_xor(regs_.e); return 1;
        case 0xAC: alu_xor(regs_.h); return 1;
        case 0xAD: alu_xor(regs_.l); return 1;
        case 0xAF: alu_xor(regs_.a); return 1;
        case 0xAE: alu_xor(memory_.read(regs_.hl())); return 2;
        case 0xEE: alu_xor(fetch_byte()); return 2;
        
        // OR
        case 0xB0: alu_or(regs_.b); return 1;
        case 0xB1: alu_or(regs_.c); return 1;
        case 0xB2: alu_or(regs_.d); return 1;
        case 0xB3: alu_or(regs_.e); return 1;
        case 0xB4: alu_or(regs_.h); return 1;
        case 0xB5: alu_or(regs_.l); return 1;
        case 0xB7: alu_or(regs_.a); return 1;
        case 0xB6: alu_or(memory_.read(regs_.hl())); return 2;
        case 0xF6: alu_or(fetch_byte()); return 2;
        
        // CP (Compare)
        case 0xB8: alu_cp(regs_.b); return 1;
        case 0xB9: alu_cp(regs_.c); return 1;
        case 0xBA: alu_cp(regs_.d); return 1;
        case 0xBB: alu_cp(regs_.e); return 1;
        case 0xBC: alu_cp(regs_.h); return 1;
        case 0xBD: alu_cp(regs_.l); return 1;
        case 0xBF: alu_cp(regs_.a); return 1;
        case 0xBE: alu_cp(memory_.read(regs_.hl())); return 2;
        case 0xFE: alu_cp(fetch_byte()); return 2;
        
        // ====================================================================
        // 8-bit INC/DEC
        // ====================================================================
        
        // INC r
        case 0x04: alu_inc(regs_.b); return 1;
        case 0x0C: alu_inc(regs_.c); return 1;
        case 0x14: alu_inc(regs_.d); return 1;
        case 0x1C: alu_inc(regs_.e); return 1;
        case 0x24: alu_inc(regs_.h); return 1;
        case 0x2C: alu_inc(regs_.l); return 1;
        case 0x3C: alu_inc(regs_.a); return 1;
        
        case 0x34:  // INC (HL)
        {
            u8 value = memory_.read(regs_.hl());
            alu_inc(value);
            memory_.write(regs_.hl(), value);
            return 3;
        }
        
        // DEC r
        case 0x05: alu_dec(regs_.b); return 1;
        case 0x0D: alu_dec(regs_.c); return 1;
        case 0x15: alu_dec(regs_.d); return 1;
        case 0x1D: alu_dec(regs_.e); return 1;
        case 0x25: alu_dec(regs_.h); return 1;
        case 0x2D: alu_dec(regs_.l); return 1;
        case 0x3D: alu_dec(regs_.a); return 1;
        
        case 0x35:  // DEC (HL)
        {
            u8 value = memory_.read(regs_.hl());
            alu_dec(value);
            memory_.write(regs_.hl(), value);
            return 3;
        }
        
        // ====================================================================
        // 16-bit INC/DEC
        // ====================================================================
        
        case 0x03: regs_.set_bc(regs_.bc() + 1); return 2;
        case 0x13: regs_.set_de(regs_.de() + 1); return 2;
        case 0x23: regs_.set_hl(regs_.hl() + 1); return 2;
        case 0x33: regs_.sp++; return 2;
        
        case 0x0B: regs_.set_bc(regs_.bc() - 1); return 2;
        case 0x1B: regs_.set_de(regs_.de() - 1); return 2;
        case 0x2B: regs_.set_hl(regs_.hl() - 1); return 2;
        case 0x3B: regs_.sp--; return 2;
        
        // ====================================================================
        // Jumps: JP
        // ====================================================================
        
        case 0xC3:  // JP nn
            regs_.pc = fetch_word();
            return 4;
            
        case 0xE9:  // JP (HL)
            regs_.pc = regs_.hl();
            return 1;
            
        // JP cc,nn (conditional)
        case 0xC2:  // JP NZ,nn
        {
            u16 addr = fetch_word();
            if (!regs_.get_flag(Registers::FLAG_Z))
            {
                regs_.pc = addr;
                return 4;
            }
            return 3;
        }
        
        case 0xCA:  // JP Z,nn
        {
            u16 addr = fetch_word();
            if (regs_.get_flag(Registers::FLAG_Z))
            {
                regs_.pc = addr;
                return 4;
            }
            return 3;
        }
        
        case 0xD2:  // JP NC,nn
        {
            u16 addr = fetch_word();
            if (!regs_.get_flag(Registers::FLAG_C))
            {
                regs_.pc = addr;
                return 4;
            }
            return 3;
        }
        
        case 0xDA:  // JP C,nn
        {
            u16 addr = fetch_word();
            if (regs_.get_flag(Registers::FLAG_C))
            {
                regs_.pc = addr;
                return 4;
            }
            return 3;
        }
        
        // ====================================================================
        // Jumps: JR (relative)
        // ====================================================================
        
        case 0x18:  // JR n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            regs_.pc += offset;
            return 3;
        }
        
        case 0x20:  // JR NZ,n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            if (!regs_.get_flag(Registers::FLAG_Z))
            {
                regs_.pc += offset;
                return 3;
            }
            return 2;
        }
        
        case 0x28:  // JR Z,n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            if (regs_.get_flag(Registers::FLAG_Z))
            {
                regs_.pc += offset;
                return 3;
            }
            return 2;
        }
        
        case 0x30:  // JR NC,n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            if (!regs_.get_flag(Registers::FLAG_C))
            {
                regs_.pc += offset;
                return 3;
            }
            return 2;
        }
        
        case 0x38:  // JR C,n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            if (regs_.get_flag(Registers::FLAG_C))
            {
                regs_.pc += offset;
                return 3;
            }
            return 2;
        }
        
        // ====================================================================
        // Calls and Returns
        // ====================================================================
        
        case 0xCD:  // CALL nn
        {
            u16 addr = fetch_word();
            push_stack(regs_.pc);
            regs_.pc = addr;
            return 6;
        }
        
        case 0xC9:  // RET
            regs_.pc = pop_stack();
            return 4;
            
        case 0xD9:  // RETI (Return and enable interrupts)
            regs_.pc = pop_stack();
            ime_ = true;
            return 4;
            
        // CALL cc,nn (conditional)
        case 0xC4:  // CALL NZ,nn
        {
            u16 addr = fetch_word();
            if (!regs_.get_flag(Registers::FLAG_Z))
            {
                push_stack(regs_.pc);
                regs_.pc = addr;
                return 6;
            }
            return 3;
        }
        
        case 0xCC:  // CALL Z,nn
        {
            u16 addr = fetch_word();
            if (regs_.get_flag(Registers::FLAG_Z))
            {
                push_stack(regs_.pc);
                regs_.pc = addr;
                return 6;
            }
            return 3;
        }
        
        case 0xD4:  // CALL NC,nn
        {
            u16 addr = fetch_word();
            if (!regs_.get_flag(Registers::FLAG_C))
            {
                push_stack(regs_.pc);
                regs_.pc = addr;
                return 6;
            }
            return 3;
        }
        
        case 0xDC:  // CALL C,nn
        {
            u16 addr = fetch_word();
            if (regs_.get_flag(Registers::FLAG_C))
            {
                push_stack(regs_.pc);
                regs_.pc = addr;
                return 6;
            }
            return 3;
        }
        
        // RET cc (conditional)
        case 0xC0:  // RET NZ
            if (!regs_.get_flag(Registers::FLAG_Z))
            {
                regs_.pc = pop_stack();
                return 5;
            }
            return 2;
            
        case 0xC8:  // RET Z
            if (regs_.get_flag(Registers::FLAG_Z))
            {
                regs_.pc = pop_stack();
                return 5;
            }
            return 2;
            
        case 0xD0:  // RET NC
            if (!regs_.get_flag(Registers::FLAG_C))
            {
                regs_.pc = pop_stack();
                return 5;
            }
            return 2;
            
        case 0xD8:  // RET C
            if (regs_.get_flag(Registers::FLAG_C))
            {
                regs_.pc = pop_stack();
                return 5;
            }
            return 2;
        
        // ====================================================================
        // RST (Restart - call to fixed address)
        // ====================================================================
        
        case 0xC7: push_stack(regs_.pc); regs_.pc = RST_00; return CYCLES_RST;
        case 0xCF: push_stack(regs_.pc); regs_.pc = RST_08; return CYCLES_RST;
        case 0xD7: push_stack(regs_.pc); regs_.pc = RST_10; return CYCLES_RST;
        case 0xDF: push_stack(regs_.pc); regs_.pc = RST_18; return CYCLES_RST;
        case 0xE7: push_stack(regs_.pc); regs_.pc = RST_20; return CYCLES_RST;
        case 0xEF: push_stack(regs_.pc); regs_.pc = RST_28; return CYCLES_RST;
        case 0xF7: push_stack(regs_.pc); regs_.pc = RST_30; return CYCLES_RST;
        case 0xFF: push_stack(regs_.pc); regs_.pc = RST_38; return CYCLES_RST;
        
        // ====================================================================
        // CB Prefix - Extended instructions
        // ====================================================================
        
        case 0xCB:
            return execute_cb_instruction(fetch_byte());
        
        // ====================================================================
        // Special Loads and Misc
        // ====================================================================
        
        case 0x02:  // LD (BC),A
            memory_.write(regs_.bc(), regs_.a);
            return 2;
            
        case 0x12:  // LD (DE),A
            memory_.write(regs_.de(), regs_.a);
            return 2;
            
        case 0x22:  // LD (HL+),A / LDI (HL),A
            memory_.write(regs_.hl(), regs_.a);
            regs_.set_hl(regs_.hl() + HL_INCREMENT);
            return CYCLES_MEMORY_WRITE;
            
        case 0x32:  // LD (HL-),A / LDD (HL),A
            memory_.write(regs_.hl(), regs_.a);
            regs_.set_hl(regs_.hl() - HL_DECREMENT);
            return CYCLES_MEMORY_WRITE;
            
        case 0x0A:  // LD A,(BC)
            regs_.a = memory_.read(regs_.bc());
            return 2;
            
        case 0x1A:  // LD A,(DE)
            regs_.a = memory_.read(regs_.de());
            return 2;
            
        case 0x2A:  // LD A,(HL+) / LDI A,(HL)
            regs_.a = memory_.read(regs_.hl());
            regs_.set_hl(regs_.hl() + HL_INCREMENT);
            return CYCLES_MEMORY_READ;
            
        case 0x3A:  // LD A,(HL-) / LDD A,(HL)
            regs_.a = memory_.read(regs_.hl());
            regs_.set_hl(regs_.hl() - HL_DECREMENT);
            return CYCLES_MEMORY_READ;
            
        case 0xE0:  // LDH (n),A - Store A to I/O registers + offset
        {
            u8 offset = fetch_byte();
            memory_.write(IO_REGISTERS_BASE + offset, regs_.a);
            return CYCLES_IMMEDIATE_BYTE + CYCLES_REGISTER_OP;
        }
        
        case 0xF0:  // LDH A,(n) - Load A from I/O registers + offset
        {
            u8 offset = fetch_byte();
            regs_.a = memory_.read(IO_REGISTERS_BASE + offset);
            return CYCLES_IMMEDIATE_BYTE + CYCLES_REGISTER_OP;
        }
        
        case 0xE2:  // LD (C),A - Store A to I/O registers + C
            memory_.write(IO_REGISTERS_BASE + regs_.c, regs_.a);
            return CYCLES_MEMORY_WRITE;
            
        case 0xF2:  // LD A,(C) - Load A from I/O registers + C
            regs_.a = memory_.read(IO_REGISTERS_BASE + regs_.c);
            return CYCLES_MEMORY_READ;
            
        case 0xEA:  // LD (nn),A
        {
            u16 addr = fetch_word();
            memory_.write(addr, regs_.a);
            return 4;
        }
        
        case 0xFA:  // LD A,(nn)
        {
            u16 addr = fetch_word();
            regs_.a = memory_.read(addr);
            return 4;
        }
        
        case 0x08:  // LD (nn),SP
        {
            u16 addr = fetch_word();
            memory_.write16(addr, regs_.sp);
            return CYCLES_LD_NN_SP;
        }
        
        case 0xF9:  // LD SP,HL
            regs_.sp = regs_.hl();
            return 2;
            
        case 0xF8:  // LD HL,SP+n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            int result = regs_.sp + offset;
            
            // Half carry and carry calculated from lower byte
            regs_.set_flag(Registers::FLAG_Z, false);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, ((regs_.sp & NIBBLE_MASK) + (offset & NIBBLE_MASK)) > NIBBLE_MASK);
            regs_.set_flag(Registers::FLAG_C, ((regs_.sp & BYTE_MASK) + (offset & BYTE_MASK)) > BYTE_MASK);
            
            regs_.set_hl(result & ADDR_MASK_16BIT);
            return CYCLES_IMMEDIATE_WORD;
        }
        
        // ====================================================================
        // 16-bit ADD
        // ====================================================================
        
        case 0x09:  // ADD HL,BC
        {
            u32 result = regs_.hl() + regs_.bc();
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, ((regs_.hl() & ADDR_MASK_12BIT) + (regs_.bc() & ADDR_MASK_12BIT)) > ADDR_MASK_12BIT);
            regs_.set_flag(Registers::FLAG_C, result > ADDR_MASK_16BIT);
            regs_.set_hl(result & ADDR_MASK_16BIT);
            return CYCLES_MEMORY_READ;
        }
        
        case 0x19:  // ADD HL,DE
        {
            u32 result = regs_.hl() + regs_.de();
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, ((regs_.hl() & ADDR_MASK_12BIT) + (regs_.de() & ADDR_MASK_12BIT)) > ADDR_MASK_12BIT);
            regs_.set_flag(Registers::FLAG_C, result > ADDR_MASK_16BIT);
            regs_.set_hl(result & ADDR_MASK_16BIT);
            return CYCLES_MEMORY_READ;
        }
        
        case 0x29:  // ADD HL,HL
        {
            u32 result = regs_.hl() + regs_.hl();
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, ((regs_.hl() & ADDR_MASK_12BIT) + (regs_.hl() & ADDR_MASK_12BIT)) > ADDR_MASK_12BIT);
            regs_.set_flag(Registers::FLAG_C, result > ADDR_MASK_16BIT);
            regs_.set_hl(result & ADDR_MASK_16BIT);
            return CYCLES_MEMORY_READ;
        }
        
        case 0x39:  // ADD HL,SP
        {
            u32 result = regs_.hl() + regs_.sp;
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, ((regs_.hl() & ADDR_MASK_12BIT) + (regs_.sp & ADDR_MASK_12BIT)) > ADDR_MASK_12BIT);
            regs_.set_flag(Registers::FLAG_C, result > ADDR_MASK_16BIT);
            regs_.set_hl(result & ADDR_MASK_16BIT);
            return CYCLES_MEMORY_READ;
        }
        
        case 0xE8:  // ADD SP,n
        {
            i8 offset = static_cast<i8>(fetch_byte());
            int result = regs_.sp + offset;
            
            regs_.set_flag(Registers::FLAG_Z, false);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, ((regs_.sp & NIBBLE_MASK) + (offset & NIBBLE_MASK)) > NIBBLE_MASK);
            regs_.set_flag(Registers::FLAG_C, ((regs_.sp & BYTE_MASK) + (offset & BYTE_MASK)) > BYTE_MASK);
            
            regs_.sp = result & ADDR_MASK_16BIT;
            return CYCLES_ADD_SP_N;
        }
        
        // ====================================================================
        // Rotates and Shifts on A
        // ====================================================================
        
        case 0x07:  // RLCA - Rotate A left
        {
            bool carry = (regs_.a & BIT_7) != 0;
            regs_.a = (regs_.a << BIT_SHIFT) | (carry ? BIT_SHIFT : 0);
            regs_.set_flag(Registers::FLAG_Z, false);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_REGISTER_OP;
        }
        
        case 0x17:  // RLA - Rotate A left through carry
        {
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (regs_.a & BIT_7) != 0;
            regs_.a = (regs_.a << BIT_SHIFT) | (old_carry ? BIT_SHIFT : 0);
            regs_.set_flag(Registers::FLAG_Z, false);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return CYCLES_REGISTER_OP;
        }
        
        case 0x0F:  // RRCA - Rotate A right
        {
            bool carry = (regs_.a & BIT_0) != 0;
            regs_.a = (regs_.a >> BIT_SHIFT) | (carry ? BIT_7 : 0);
            regs_.set_flag(Registers::FLAG_Z, false);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, carry);
            return CYCLES_REGISTER_OP;
        }
        
        case 0x1F:  // RRA - Rotate A right through carry
        {
            bool old_carry = regs_.get_flag(Registers::FLAG_C);
            bool new_carry = (regs_.a & BIT_0) != 0;
            regs_.a = (regs_.a >> BIT_SHIFT) | (old_carry ? BIT_7 : 0);
            regs_.set_flag(Registers::FLAG_Z, false);
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, new_carry);
            return CYCLES_REGISTER_OP;
        }
        
        // ====================================================================
        // Misc Operations
        // ====================================================================
        
        case 0x27:  // DAA - Decimal Adjust Accumulator
        {
            // Adjust A to BCD after ADD/SUB
            u8 correction = 0;
            bool set_carry = false;
            
            if (regs_.get_flag(Registers::FLAG_H) || (!regs_.get_flag(Registers::FLAG_N) && (regs_.a & NIBBLE_MASK) > BCD_DIGIT_MAX))
            {
                correction |= BCD_CORRECTION_LOWER;
            }
            
            if (regs_.get_flag(Registers::FLAG_C) || (!regs_.get_flag(Registers::FLAG_N) && regs_.a > BCD_BYTE_MAX))
            {
                correction |= BCD_CORRECTION_UPPER;
                set_carry = true;
            }
            
            if (regs_.get_flag(Registers::FLAG_N))
            {
                regs_.a -= correction;
            }
            else
            {
                regs_.a += correction;
            }
            
            regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, set_carry);
            return 1;
        }
        
        case 0x2F:  // CPL - Complement A
            regs_.a = ~regs_.a;
            regs_.set_flag(Registers::FLAG_N, true);
            regs_.set_flag(Registers::FLAG_H, true);
            return 1;
            
        case 0x3F:  // CCF - Complement Carry Flag
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, !regs_.get_flag(Registers::FLAG_C));
            return 1;
            
        case 0x37:  // SCF - Set Carry Flag
            regs_.set_flag(Registers::FLAG_N, false);
            regs_.set_flag(Registers::FLAG_H, false);
            regs_.set_flag(Registers::FLAG_C, true);
            return 1;
        
        default:
            // Unknown opcode - this shouldn't happen with valid ROMs
            return 1;
    }
}

} // namespace gbglow
