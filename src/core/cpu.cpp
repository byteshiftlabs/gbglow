#include "cpu.h"
#include <stdexcept>

namespace emugbc {

CPU::CPU(Memory& memory) 
    : memory_(memory)
    , ime_(false)
    , halted_(false)
    , stopped_(false) {
}

void CPU::reset() {
    regs_.reset();
    ime_ = false;
    halted_ = false;
    stopped_ = false;
}

Cycles CPU::step() {
    if (halted_) {
        // When halted, CPU does nothing but waits for interrupt
        return 1;  // 1 M-cycle
    }
    
    // Fetch instruction
    u8 opcode = fetch_byte();
    
    // Execute instruction
    return execute_instruction(opcode);
}

void CPU::handle_interrupts() {
    if (!ime_) return;  // Interrupts disabled
    
    u8 ie = memory_.read(0xFFFF);  // Interrupt Enable
    u8 if_reg = memory_.read(0xFF0F);  // Interrupt Flags
    
    u8 pending = ie & if_reg;
    if (pending == 0) return;  // No pending interrupts
    
    // Priority order: VBlank > LCD STAT > Timer > Serial > Joypad
    u8 interrupt_bit = 0;
    u16 interrupt_vector = 0;
    
    if (pending & 0x01) {
        // VBlank interrupt
        interrupt_bit = 0x01;
        interrupt_vector = 0x40;
    } else if (pending & 0x02) {
        // LCD STAT interrupt
        interrupt_bit = 0x02;
        interrupt_vector = 0x48;
    } else if (pending & 0x04) {
        // Timer interrupt
        interrupt_bit = 0x04;
        interrupt_vector = 0x50;
    } else if (pending & 0x08) {
        // Serial interrupt
        interrupt_bit = 0x08;
        interrupt_vector = 0x58;
    } else if (pending & 0x10) {
        // Joypad interrupt
        interrupt_bit = 0x10;
        interrupt_vector = 0x60;
    }
    
    if (interrupt_vector != 0) {
        // Service the interrupt
        halted_ = false;  // Wake from halt
        ime_ = false;     // Disable interrupts
        
        // Clear the interrupt flag
        memory_.write(0xFF0F, if_reg & ~interrupt_bit);
        
        // Push PC and jump to interrupt vector
        push_stack(regs_.pc);
        regs_.pc = interrupt_vector;
    }
}

u8 CPU::fetch_byte() {
    u8 byte = memory_.read(regs_.pc);
    regs_.pc++;
    return byte;
}

u16 CPU::fetch_word() {
    u16 word = memory_.read16(regs_.pc);
    regs_.pc += 2;
    return word;
}

void CPU::push_stack(u16 value) {
    regs_.sp -= 2;
    memory_.write16(regs_.sp, value);
}

u16 CPU::pop_stack() {
    u16 value = memory_.read16(regs_.sp);
    regs_.sp += 2;
    return value;
}

Cycles CPU::execute_instruction(u8 opcode) {
    switch (opcode) {
        // === NOP and HALT ===
        case 0x00: return 1;  // NOP
        case 0x76: halted_ = true; return 1;  // HALT
        
        // === 8-bit Loads: LD r,r ===
        case 0x7F: return 1;  // LD A,A
        case 0x78: regs_.a = regs_.b; return 1;  // LD A,B
        case 0x79: regs_.a = regs_.c; return 1;  // LD A,C
        case 0x7A: regs_.a = regs_.d; return 1;  // LD A,D
        case 0x7B: regs_.a = regs_.e; return 1;  // LD A,E
        case 0x7C: regs_.a = regs_.h; return 1;  // LD A,H
        case 0x7D: regs_.a = regs_.l; return 1;  // LD A,L
        
        case 0x47: regs_.b = regs_.a; return 1;  // LD B,A
        case 0x40: return 1;  // LD B,B
        case 0x41: regs_.b = regs_.c; return 1;  // LD B,C
        case 0x42: regs_.b = regs_.d; return 1;  // LD B,D
        case 0x43: regs_.b = regs_.e; return 1;  // LD B,E
        case 0x44: regs_.b = regs_.h; return 1;  // LD B,H
        case 0x45: regs_.b = regs_.l; return 1;  // LD B,L
        
        case 0x4F: regs_.c = regs_.a; return 1;  // LD C,A
        case 0x48: regs_.c = regs_.b; return 1;  // LD C,B
        case 0x49: return 1;  // LD C,C
        case 0x4A: regs_.c = regs_.d; return 1;  // LD C,D
        case 0x4B: regs_.c = regs_.e; return 1;  // LD C,E
        case 0x4C: regs_.c = regs_.h; return 1;  // LD C,H
        case 0x4D: regs_.c = regs_.l; return 1;  // LD C,L
        
        case 0x57: regs_.d = regs_.a; return 1;  // LD D,A
        case 0x50: regs_.d = regs_.b; return 1;  // LD D,B
        case 0x51: regs_.d = regs_.c; return 1;  // LD D,C
        case 0x52: return 1;  // LD D,D
        case 0x53: regs_.d = regs_.e; return 1;  // LD D,E
        case 0x54: regs_.d = regs_.h; return 1;  // LD D,H
        case 0x55: regs_.d = regs_.l; return 1;  // LD D,L
        
        case 0x5F: regs_.e = regs_.a; return 1;  // LD E,A
        case 0x58: regs_.e = regs_.b; return 1;  // LD E,B
        case 0x59: regs_.e = regs_.c; return 1;  // LD E,C
        case 0x5A: regs_.e = regs_.d; return 1;  // LD E,D
        case 0x5B: return 1;  // LD E,E
        case 0x5C: regs_.e = regs_.h; return 1;  // LD E,H
        case 0x5D: regs_.e = regs_.l; return 1;  // LD E,L
        
        case 0x67: regs_.h = regs_.a; return 1;  // LD H,A
        case 0x60: regs_.h = regs_.b; return 1;  // LD H,B
        case 0x61: regs_.h = regs_.c; return 1;  // LD H,C
        case 0x62: regs_.h = regs_.d; return 1;  // LD H,D
        case 0x63: regs_.h = regs_.e; return 1;  // LD H,E
        case 0x64: return 1;  // LD H,H
        case 0x65: regs_.h = regs_.l; return 1;  // LD H,L
        
        case 0x6F: regs_.l = regs_.a; return 1;  // LD L,A
        case 0x68: regs_.l = regs_.b; return 1;  // LD L,B
        case 0x69: regs_.l = regs_.c; return 1;  // LD L,C
        case 0x6A: regs_.l = regs_.d; return 1;  // LD L,D
        case 0x6B: regs_.l = regs_.e; return 1;  // LD L,E
        case 0x6C: regs_.l = regs_.h; return 1;  // LD L,H
        case 0x6D: return 1;  // LD L,L
        
        // === 8-bit Loads: LD r,n ===
        case 0x3E: regs_.a = fetch_byte(); return 2;  // LD A,n
        case 0x06: regs_.b = fetch_byte(); return 2;  // LD B,n
        case 0x0E: regs_.c = fetch_byte(); return 2;  // LD C,n
        case 0x16: regs_.d = fetch_byte(); return 2;  // LD D,n
        case 0x1E: regs_.e = fetch_byte(); return 2;  // LD E,n
        case 0x26: regs_.h = fetch_byte(); return 2;  // LD H,n
        case 0x2E: regs_.l = fetch_byte(); return 2;  // LD L,n
        
        // === 8-bit Loads: LD r,(HL) ===
        case 0x7E: regs_.a = memory_.read(regs_.hl()); return 2;  // LD A,(HL)
        case 0x46: regs_.b = memory_.read(regs_.hl()); return 2;  // LD B,(HL)
        case 0x4E: regs_.c = memory_.read(regs_.hl()); return 2;  // LD C,(HL)
        case 0x56: regs_.d = memory_.read(regs_.hl()); return 2;  // LD D,(HL)
        case 0x5E: regs_.e = memory_.read(regs_.hl()); return 2;  // LD E,(HL)
        case 0x66: regs_.h = memory_.read(regs_.hl()); return 2;  // LD H,(HL)
        case 0x6E: regs_.l = memory_.read(regs_.hl()); return 2;  // LD L,(HL)
        
        // === 8-bit Loads: LD (HL),r ===
        case 0x77: memory_.write(regs_.hl(), regs_.a); return 2;  // LD (HL),A
        case 0x70: memory_.write(regs_.hl(), regs_.b); return 2;  // LD (HL),B
        case 0x71: memory_.write(regs_.hl(), regs_.c); return 2;  // LD (HL),C
        case 0x72: memory_.write(regs_.hl(), regs_.d); return 2;  // LD (HL),D
        case 0x73: memory_.write(regs_.hl(), regs_.e); return 2;  // LD (HL),E
        case 0x74: memory_.write(regs_.hl(), regs_.h); return 2;  // LD (HL),H
        case 0x75: memory_.write(regs_.hl(), regs_.l); return 2;  // LD (HL),L
        case 0x36: memory_.write(regs_.hl(), fetch_byte()); return 3;  // LD (HL),n
        
        // === 8-bit Loads: Special ===
        case 0x0A: regs_.a = memory_.read(regs_.bc()); return 2;  // LD A,(BC)
        case 0x1A: regs_.a = memory_.read(regs_.de()); return 2;  // LD A,(DE)
        case 0xFA: regs_.a = memory_.read(fetch_word()); return 4;  // LD A,(nn)
        case 0x02: memory_.write(regs_.bc(), regs_.a); return 2;  // LD (BC),A
        case 0x12: memory_.write(regs_.de(), regs_.a); return 2;  // LD (DE),A
        case 0xEA: memory_.write(fetch_word(), regs_.a); return 4;  // LD (nn),A
        
        // === 8-bit Loads: LD A,(C) / LD (C),A (I/O) ===
        case 0xF2: regs_.a = memory_.read(0xFF00 + regs_.c); return 2;  // LD A,(C)
        case 0xE2: memory_.write(0xFF00 + regs_.c, regs_.a); return 2;  // LD (C),A
        
        // === 8-bit Loads: LDH ===
        case 0xE0: memory_.write(0xFF00 + fetch_byte(), regs_.a); return 3;  // LDH (n),A
        case 0xF0: regs_.a = memory_.read(0xFF00 + fetch_byte()); return 3;  // LDH A,(n)
        
        // === 8-bit Loads: LD (HL+/-),A and LD A,(HL+/-) ===
        case 0x22: memory_.write(regs_.hl(), regs_.a); regs_.set_hl(regs_.hl() + 1); return 2;  // LD (HL+),A
        case 0x32: memory_.write(regs_.hl(), regs_.a); regs_.set_hl(regs_.hl() - 1); return 2;  // LD (HL-),A
        case 0x2A: regs_.a = memory_.read(regs_.hl()); regs_.set_hl(regs_.hl() + 1); return 2;  // LD A,(HL+)
        case 0x3A: regs_.a = memory_.read(regs_.hl()); regs_.set_hl(regs_.hl() - 1); return 2;  // LD A,(HL-)
        
        // === 16-bit Loads ===
        case 0x01: regs_.set_bc(fetch_word()); return 3;  // LD BC,nn
        case 0x11: regs_.set_de(fetch_word()); return 3;  // LD DE,nn
        case 0x21: regs_.set_hl(fetch_word()); return 3;  // LD HL,nn
        case 0x31: regs_.sp = fetch_word(); return 3;  // LD SP,nn
        case 0xF9: regs_.sp = regs_.hl(); return 2;  // LD SP,HL
        case 0x08: memory_.write16(fetch_word(), regs_.sp); return 5;  // LD (nn),SP
        
        // === PUSH/POP ===
        case 0xF5: push_stack(regs_.af()); return 4;  // PUSH AF
        case 0xC5: push_stack(regs_.bc()); return 4;  // PUSH BC
        case 0xD5: push_stack(regs_.de()); return 4;  // PUSH DE
        case 0xE5: push_stack(regs_.hl()); return 4;  // PUSH HL
        case 0xF1: regs_.set_af(pop_stack()); return 3;  // POP AF
        case 0xC1: regs_.set_bc(pop_stack()); return 3;  // POP BC
        case 0xD1: regs_.set_de(pop_stack()); return 3;  // POP DE
        case 0xE1: regs_.set_hl(pop_stack()); return 3;  // POP HL
        
        // === ALU: ADD ===
        case 0x87: alu_add(regs_.a); return 1;  // ADD A,A
        case 0x80: alu_add(regs_.b); return 1;  // ADD A,B
        case 0x81: alu_add(regs_.c); return 1;  // ADD A,C
        case 0x82: alu_add(regs_.d); return 1;  // ADD A,D
        case 0x83: alu_add(regs_.e); return 1;  // ADD A,E
        case 0x84: alu_add(regs_.h); return 1;  // ADD A,H
        case 0x85: alu_add(regs_.l); return 1;  // ADD A,L
        case 0x86: alu_add(memory_.read(regs_.hl())); return 2;  // ADD A,(HL)
        case 0xC6: alu_add(fetch_byte()); return 2;  // ADD A,n
        
        // === ALU: ADC ===
        case 0x8F: alu_add(regs_.a, true); return 1;  // ADC A,A
        case 0x88: alu_add(regs_.b, true); return 1;  // ADC A,B
        case 0x89: alu_add(regs_.c, true); return 1;  // ADC A,C
        case 0x8A: alu_add(regs_.d, true); return 1;  // ADC A,D
        case 0x8B: alu_add(regs_.e, true); return 1;  // ADC A,E
        case 0x8C: alu_add(regs_.h, true); return 1;  // ADC A,H
        case 0x8D: alu_add(regs_.l, true); return 1;  // ADC A,L
        case 0x8E: alu_add(memory_.read(regs_.hl()), true); return 2;  // ADC A,(HL)
        case 0xCE: alu_add(fetch_byte(), true); return 2;  // ADC A,n
        
        // === ALU: SUB ===
        case 0x97: alu_sub(regs_.a); return 1;  // SUB A
        case 0x90: alu_sub(regs_.b); return 1;  // SUB B
        case 0x91: alu_sub(regs_.c); return 1;  // SUB C
        case 0x92: alu_sub(regs_.d); return 1;  // SUB D
        case 0x93: alu_sub(regs_.e); return 1;  // SUB E
        case 0x94: alu_sub(regs_.h); return 1;  // SUB H
        case 0x95: alu_sub(regs_.l); return 1;  // SUB L
        case 0x96: alu_sub(memory_.read(regs_.hl())); return 2;  // SUB (HL)
        case 0xD6: alu_sub(fetch_byte()); return 2;  // SUB n
        
        // === ALU: SBC ===
        case 0x9F: alu_sub(regs_.a, true); return 1;  // SBC A,A
        case 0x98: alu_sub(regs_.b, true); return 1;  // SBC A,B
        case 0x99: alu_sub(regs_.c, true); return 1;  // SBC A,C
        case 0x9A: alu_sub(regs_.d, true); return 1;  // SBC A,D
        case 0x9B: alu_sub(regs_.e, true); return 1;  // SBC A,E
        case 0x9C: alu_sub(regs_.h, true); return 1;  // SBC A,H
        case 0x9D: alu_sub(regs_.l, true); return 1;  // SBC A,L
        case 0x9E: alu_sub(memory_.read(regs_.hl()), true); return 2;  // SBC A,(HL)
        case 0xDE: alu_sub(fetch_byte(), true); return 2;  // SBC A,n
        
        // === ALU: AND ===
        case 0xA7: alu_and(regs_.a); return 1;  // AND A
        case 0xA0: alu_and(regs_.b); return 1;  // AND B
        case 0xA1: alu_and(regs_.c); return 1;  // AND C
        case 0xA2: alu_and(regs_.d); return 1;  // AND D
        case 0xA3: alu_and(regs_.e); return 1;  // AND E
        case 0xA4: alu_and(regs_.h); return 1;  // AND H
        case 0xA5: alu_and(regs_.l); return 1;  // AND L
        case 0xA6: alu_and(memory_.read(regs_.hl())); return 2;  // AND (HL)
        case 0xE6: alu_and(fetch_byte()); return 2;  // AND n
        
        // === ALU: OR ===
        case 0xB7: alu_or(regs_.a); return 1;  // OR A
        case 0xB0: alu_or(regs_.b); return 1;  // OR B
        case 0xB1: alu_or(regs_.c); return 1;  // OR C
        case 0xB2: alu_or(regs_.d); return 1;  // OR D
        case 0xB3: alu_or(regs_.e); return 1;  // OR E
        case 0xB4: alu_or(regs_.h); return 1;  // OR H
        case 0xB5: alu_or(regs_.l); return 1;  // OR L
        case 0xB6: alu_or(memory_.read(regs_.hl())); return 2;  // OR (HL)
        case 0xF6: alu_or(fetch_byte()); return 2;  // OR n
        
        // === ALU: XOR ===
        case 0xAF: alu_xor(regs_.a); return 1;  // XOR A
        case 0xA8: alu_xor(regs_.b); return 1;  // XOR B
        case 0xA9: alu_xor(regs_.c); return 1;  // XOR C
        case 0xAA: alu_xor(regs_.d); return 1;  // XOR D
        case 0xAB: alu_xor(regs_.e); return 1;  // XOR E
        case 0xAC: alu_xor(regs_.h); return 1;  // XOR H
        case 0xAD: alu_xor(regs_.l); return 1;  // XOR L
        case 0xAE: alu_xor(memory_.read(regs_.hl())); return 2;  // XOR (HL)
        case 0xEE: alu_xor(fetch_byte()); return 2;  // XOR n
        
        // === ALU: CP ===
        case 0xBF: alu_cp(regs_.a); return 1;  // CP A
        case 0xB8: alu_cp(regs_.b); return 1;  // CP B
        case 0xB9: alu_cp(regs_.c); return 1;  // CP C
        case 0xBA: alu_cp(regs_.d); return 1;  // CP D
        case 0xBB: alu_cp(regs_.e); return 1;  // CP E
        case 0xBC: alu_cp(regs_.h); return 1;  // CP H
        case 0xBD: alu_cp(regs_.l); return 1;  // CP L
        case 0xBE: alu_cp(memory_.read(regs_.hl())); return 2;  // CP (HL)
        case 0xFE: alu_cp(fetch_byte()); return 2;  // CP n
        
        // === ALU: INC (8-bit) ===
        case 0x3C: alu_inc(regs_.a); return 1;  // INC A
        case 0x04: alu_inc(regs_.b); return 1;  // INC B
        case 0x0C: alu_inc(regs_.c); return 1;  // INC C
        case 0x14: alu_inc(regs_.d); return 1;  // INC D
        case 0x1C: alu_inc(regs_.e); return 1;  // INC E
        case 0x24: alu_inc(regs_.h); return 1;  // INC H
        case 0x2C: alu_inc(regs_.l); return 1;  // INC L
        case 0x34: { u8 val = memory_.read(regs_.hl()); alu_inc(val); memory_.write(regs_.hl(), val); return 3; }  // INC (HL)
        
        // === ALU: DEC (8-bit) ===
        case 0x3D: alu_dec(regs_.a); return 1;  // DEC A
        case 0x05: alu_dec(regs_.b); return 1;  // DEC B
        case 0x0D: alu_dec(regs_.c); return 1;  // DEC C
        case 0x15: alu_dec(regs_.d); return 1;  // DEC D
        case 0x1D: alu_dec(regs_.e); return 1;  // DEC E
        case 0x25: alu_dec(regs_.h); return 1;  // DEC H
        case 0x2D: alu_dec(regs_.l); return 1;  // DEC L
        case 0x35: { u8 val = memory_.read(regs_.hl()); alu_dec(val); memory_.write(regs_.hl(), val); return 3; }  // DEC (HL)
        
        // === ALU: INC (16-bit) ===
        case 0x03: regs_.set_bc(regs_.bc() + 1); return 2;  // INC BC
        case 0x13: regs_.set_de(regs_.de() + 1); return 2;  // INC DE
        case 0x23: regs_.set_hl(regs_.hl() + 1); return 2;  // INC HL
        case 0x33: regs_.sp++; return 2;  // INC SP
        
        // === ALU: DEC (16-bit) ===
        case 0x0B: regs_.set_bc(regs_.bc() - 1); return 2;  // DEC BC
        case 0x1B: regs_.set_de(regs_.de() - 1); return 2;  // DEC DE
        case 0x2B: regs_.set_hl(regs_.hl() - 1); return 2;  // DEC HL
        case 0x3B: regs_.sp--; return 2;  // DEC SP
        
        // === Jumps ===
        case 0xC3: regs_.pc = fetch_word(); return 4;  // JP nn
        case 0xC2: if (!regs_.get_flag(Registers::FLAG_Z)) { regs_.pc = fetch_word(); return 4; } fetch_word(); return 3;  // JP NZ,nn
        case 0xCA: if (regs_.get_flag(Registers::FLAG_Z)) { regs_.pc = fetch_word(); return 4; } fetch_word(); return 3;  // JP Z,nn
        case 0xD2: if (!regs_.get_flag(Registers::FLAG_C)) { regs_.pc = fetch_word(); return 4; } fetch_word(); return 3;  // JP NC,nn
        case 0xDA: if (regs_.get_flag(Registers::FLAG_C)) { regs_.pc = fetch_word(); return 4; } fetch_word(); return 3;  // JP C,nn
        case 0xE9: regs_.pc = regs_.hl(); return 1;  // JP (HL)
        
        // === Relative Jumps ===
        case 0x18: { i8 offset = static_cast<i8>(fetch_byte()); regs_.pc += offset; return 3; }  // JR e
        case 0x20: { i8 offset = static_cast<i8>(fetch_byte()); if (!regs_.get_flag(Registers::FLAG_Z)) { regs_.pc += offset; return 3; } return 2; }  // JR NZ,e
        case 0x28: { i8 offset = static_cast<i8>(fetch_byte()); if (regs_.get_flag(Registers::FLAG_Z)) { regs_.pc += offset; return 3; } return 2; }  // JR Z,e
        case 0x30: { i8 offset = static_cast<i8>(fetch_byte()); if (!regs_.get_flag(Registers::FLAG_C)) { regs_.pc += offset; return 3; } return 2; }  // JR NC,e
        case 0x38: { i8 offset = static_cast<i8>(fetch_byte()); if (regs_.get_flag(Registers::FLAG_C)) { regs_.pc += offset; return 3; } return 2; }  // JR C,e
        
        // === Calls ===
        case 0xCD: { u16 addr = fetch_word(); push_stack(regs_.pc); regs_.pc = addr; return 6; }  // CALL nn
        case 0xC4: { u16 addr = fetch_word(); if (!regs_.get_flag(Registers::FLAG_Z)) { push_stack(regs_.pc); regs_.pc = addr; return 6; } return 3; }  // CALL NZ,nn
        case 0xCC: { u16 addr = fetch_word(); if (regs_.get_flag(Registers::FLAG_Z)) { push_stack(regs_.pc); regs_.pc = addr; return 6; } return 3; }  // CALL Z,nn
        case 0xD4: { u16 addr = fetch_word(); if (!regs_.get_flag(Registers::FLAG_C)) { push_stack(regs_.pc); regs_.pc = addr; return 6; } return 3; }  // CALL NC,nn
        case 0xDC: { u16 addr = fetch_word(); if (regs_.get_flag(Registers::FLAG_C)) { push_stack(regs_.pc); regs_.pc = addr; return 6; } return 3; }  // CALL C,nn
        
        // === Returns ===
        case 0xC9: regs_.pc = pop_stack(); return 4;  // RET
        case 0xC0: if (!regs_.get_flag(Registers::FLAG_Z)) { regs_.pc = pop_stack(); return 5; } return 2;  // RET NZ
        case 0xC8: if (regs_.get_flag(Registers::FLAG_Z)) { regs_.pc = pop_stack(); return 5; } return 2;  // RET Z
        case 0xD0: if (!regs_.get_flag(Registers::FLAG_C)) { regs_.pc = pop_stack(); return 5; } return 2;  // RET NC
        case 0xD8: if (regs_.get_flag(Registers::FLAG_C)) { regs_.pc = pop_stack(); return 5; } return 2;  // RET C
        case 0xD9: regs_.pc = pop_stack(); ime_ = true; return 4;  // RETI
        
        // === RST (Restart) ===
        case 0xC7: push_stack(regs_.pc); regs_.pc = 0x00; return 4;  // RST 00H
        case 0xCF: push_stack(regs_.pc); regs_.pc = 0x08; return 4;  // RST 08H
        case 0xD7: push_stack(regs_.pc); regs_.pc = 0x10; return 4;  // RST 10H
        case 0xDF: push_stack(regs_.pc); regs_.pc = 0x18; return 4;  // RST 18H
        case 0xE7: push_stack(regs_.pc); regs_.pc = 0x20; return 4;  // RST 20H
        case 0xEF: push_stack(regs_.pc); regs_.pc = 0x28; return 4;  // RST 28H
        case 0xF7: push_stack(regs_.pc); regs_.pc = 0x30; return 4;  // RST 30H
        case 0xFF: push_stack(regs_.pc); regs_.pc = 0x38; return 4;  // RST 38H
        
        // === Miscellaneous ===
        case 0x27: /* DAA */ return 1;  // TODO: Implement DAA
        case 0x2F: regs_.a = ~regs_.a; regs_.set_flag(Registers::FLAG_N, true); regs_.set_flag(Registers::FLAG_H, true); return 1;  // CPL
        case 0x3F: regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, false); regs_.set_flag(Registers::FLAG_C, !regs_.get_flag(Registers::FLAG_C)); return 1;  // CCF
        case 0x37: regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, false); regs_.set_flag(Registers::FLAG_C, true); return 1;  // SCF
        case 0xF3: ime_ = false; return 1;  // DI
        case 0xFB: ime_ = true; return 1;  // EI
        
        // === Rotates and Shifts (non-CB) ===
        case 0x07: { bool bit7 = (regs_.a & 0x80) != 0; regs_.a = (regs_.a << 1) | (bit7 ? 1 : 0); regs_.f = 0; regs_.set_flag(Registers::FLAG_C, bit7); return 1; }  // RLCA
        case 0x17: { bool carry = regs_.get_flag(Registers::FLAG_C); bool bit7 = (regs_.a & 0x80) != 0; regs_.a = (regs_.a << 1) | (carry ? 1 : 0); regs_.f = 0; regs_.set_flag(Registers::FLAG_C, bit7); return 1; }  // RLA
        case 0x0F: { bool bit0 = (regs_.a & 0x01) != 0; regs_.a = (regs_.a >> 1) | (bit0 ? 0x80 : 0); regs_.f = 0; regs_.set_flag(Registers::FLAG_C, bit0); return 1; }  // RRCA
        case 0x1F: { bool carry = regs_.get_flag(Registers::FLAG_C); bool bit0 = (regs_.a & 0x01) != 0; regs_.a = (regs_.a >> 1) | (carry ? 0x80 : 0); regs_.f = 0; regs_.set_flag(Registers::FLAG_C, bit0); return 1; }  // RRA
        
        // === CB Prefix ===
        case 0xCB: { u8 cb_opcode = fetch_byte(); return execute_cb_instruction(cb_opcode); }
        
        // === 16-bit ALU ===
        case 0x09: { u16 hl = regs_.hl(); u16 bc = regs_.bc(); u32 result = hl + bc; regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, (hl & 0x0FFF) + (bc & 0x0FFF) > 0x0FFF); regs_.set_flag(Registers::FLAG_C, result > 0xFFFF); regs_.set_hl(result & 0xFFFF); return 2; }  // ADD HL,BC
        case 0x19: { u16 hl = regs_.hl(); u16 de = regs_.de(); u32 result = hl + de; regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, (hl & 0x0FFF) + (de & 0x0FFF) > 0x0FFF); regs_.set_flag(Registers::FLAG_C, result > 0xFFFF); regs_.set_hl(result & 0xFFFF); return 2; }  // ADD HL,DE
        case 0x29: { u16 hl = regs_.hl(); u32 result = hl + hl; regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, (hl & 0x0FFF) + (hl & 0x0FFF) > 0x0FFF); regs_.set_flag(Registers::FLAG_C, result > 0xFFFF); regs_.set_hl(result & 0xFFFF); return 2; }  // ADD HL,HL
        case 0x39: { u16 hl = regs_.hl(); u16 sp = regs_.sp; u32 result = hl + sp; regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, (hl & 0x0FFF) + (sp & 0x0FFF) > 0x0FFF); regs_.set_flag(Registers::FLAG_C, result > 0xFFFF); regs_.set_hl(result & 0xFFFF); return 2; }  // ADD HL,SP
        case 0xE8: { i8 offset = static_cast<i8>(fetch_byte()); u16 sp = regs_.sp; regs_.set_flag(Registers::FLAG_Z, false); regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, ((sp & 0x0F) + (offset & 0x0F)) > 0x0F); regs_.set_flag(Registers::FLAG_C, ((sp & 0xFF) + (offset & 0xFF)) > 0xFF); regs_.sp = sp + offset; return 4; }  // ADD SP,e
        case 0xF8: { i8 offset = static_cast<i8>(fetch_byte()); u16 sp = regs_.sp; regs_.set_flag(Registers::FLAG_Z, false); regs_.set_flag(Registers::FLAG_N, false); regs_.set_flag(Registers::FLAG_H, ((sp & 0x0F) + (offset & 0x0F)) > 0x0F); regs_.set_flag(Registers::FLAG_C, ((sp & 0xFF) + (offset & 0xFF)) > 0xFF); regs_.set_hl(sp + offset); return 3; }  // LD HL,SP+e
        
        default:
            // Unimplemented opcode
            return 1;
    }
}

Cycles CPU::execute_cb_instruction(u8 opcode) {
    // CB instructions operate on 8 registers + (HL)
    // Pattern: bits 0-2 = register, bits 3-7 = operation
    
    u8 reg_idx = opcode & 0x07;
    u8* reg = nullptr;
    u8 temp_val = 0;
    bool is_hl = (reg_idx == 6);
    
    // Get register pointer or (HL) value
    if (is_hl) {
        temp_val = memory_.read(regs_.hl());
        reg = &temp_val;
    } else {
        u8* regs[] = {&regs_.b, &regs_.c, &regs_.d, &regs_.e, &regs_.h, &regs_.l, nullptr, &regs_.a};
        reg = regs[reg_idx];
    }
    
    u8 bit_idx = (opcode >> 3) & 0x07;
    
    // Determine operation
    if (opcode < 0x40) {
        // 0x00-0x3F: Rotates and shifts
        u8 op = opcode >> 3;
        switch (op) {
            case 0x00: { // RLC
                bool bit7 = (*reg & 0x80) != 0;
                *reg = (*reg << 1) | (bit7 ? 1 : 0);
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit7);
                break;
            }
            case 0x01: { // RRC
                bool bit0 = (*reg & 0x01) != 0;
                *reg = (*reg >> 1) | (bit0 ? 0x80 : 0);
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit0);
                break;
            }
            case 0x02: { // RL
                bool carry = regs_.get_flag(Registers::FLAG_C);
                bool bit7 = (*reg & 0x80) != 0;
                *reg = (*reg << 1) | (carry ? 1 : 0);
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit7);
                break;
            }
            case 0x03: { // RR
                bool carry = regs_.get_flag(Registers::FLAG_C);
                bool bit0 = (*reg & 0x01) != 0;
                *reg = (*reg >> 1) | (carry ? 0x80 : 0);
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit0);
                break;
            }
            case 0x04: { // SLA
                bool bit7 = (*reg & 0x80) != 0;
                *reg <<= 1;
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit7);
                break;
            }
            case 0x05: { // SRA
                bool bit0 = (*reg & 0x01) != 0;
                bool bit7 = (*reg & 0x80) != 0;
                *reg = (*reg >> 1) | (bit7 ? 0x80 : 0);
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit0);
                break;
            }
            case 0x06: { // SWAP
                *reg = (*reg << 4) | (*reg >> 4);
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                break;
            }
            case 0x07: { // SRL
                bool bit0 = (*reg & 0x01) != 0;
                *reg >>= 1;
                regs_.f = 0;
                regs_.set_flag(Registers::FLAG_Z, *reg == 0);
                regs_.set_flag(Registers::FLAG_C, bit0);
                break;
            }
        }
    } else if (opcode < 0x80) {
        // 0x40-0x7F: BIT
        bool bit_set = (*reg & (1 << bit_idx)) != 0;
        regs_.set_flag(Registers::FLAG_Z, !bit_set);
        regs_.set_flag(Registers::FLAG_N, false);
        regs_.set_flag(Registers::FLAG_H, true);
    } else if (opcode < 0xC0) {
        // 0x80-0xBF: RES
        *reg &= ~(1 << bit_idx);
    } else {
        // 0xC0-0xFF: SET
        *reg |= (1 << bit_idx);
    }
    
    // Write back if (HL)
    if (is_hl) {
        memory_.write(regs_.hl(), temp_val);
        return 4;  // (HL) operations take 4 M-cycles
    }
    
    return 2;  // Register operations take 2 M-cycles
}

// ALU Operations
void CPU::alu_add(u8 value, bool use_carry) {
    u16 result = regs_.a + value;
    if (use_carry && regs_.get_flag(Registers::FLAG_C)) {
        result++;
    }
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, ((regs_.a & 0x0F) + (value & 0x0F)) > 0x0F);
    regs_.set_flag(Registers::FLAG_C, result > 0xFF);
    
    regs_.a = result & 0xFF;
}

void CPU::alu_sub(u8 value, bool use_carry) {
    u16 result = regs_.a - value;
    if (use_carry && regs_.get_flag(Registers::FLAG_C)) {
        result--;
    }
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, (regs_.a & 0x0F) < (value & 0x0F));
    regs_.set_flag(Registers::FLAG_C, result > 0xFF);
    
    regs_.a = result & 0xFF;
}

void CPU::alu_and(u8 value) {
    regs_.a &= value;
    
    regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, true);
    regs_.set_flag(Registers::FLAG_C, false);
}

void CPU::alu_or(u8 value) {
    regs_.a |= value;
    
    regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, false);
    regs_.set_flag(Registers::FLAG_C, false);
}

void CPU::alu_xor(u8 value) {
    regs_.a ^= value;
    
    regs_.set_flag(Registers::FLAG_Z, regs_.a == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, false);
    regs_.set_flag(Registers::FLAG_C, false);
}

void CPU::alu_cp(u8 value) {
    u16 result = regs_.a - value;
    
    regs_.set_flag(Registers::FLAG_Z, (result & 0xFF) == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, (regs_.a & 0x0F) < (value & 0x0F));
    regs_.set_flag(Registers::FLAG_C, result > 0xFF);
}

void CPU::alu_inc(u8& reg) {
    reg++;
    
    regs_.set_flag(Registers::FLAG_Z, reg == 0);
    regs_.set_flag(Registers::FLAG_N, false);
    regs_.set_flag(Registers::FLAG_H, (reg & 0x0F) == 0);
}

void CPU::alu_dec(u8& reg) {
    reg--;
    
    regs_.set_flag(Registers::FLAG_Z, reg == 0);
    regs_.set_flag(Registers::FLAG_N, true);
    regs_.set_flag(Registers::FLAG_H, (reg & 0x0F) == 0x0F);
}

} // namespace emugbc
