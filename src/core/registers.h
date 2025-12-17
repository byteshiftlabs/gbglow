#pragma once

#include "types.h"

namespace emugbc {

/**
 * CPU Registers for the Sharp LR35902 (Game Boy CPU)
 * 
 * The Game Boy CPU has 8 8-bit registers that can be paired:
 * - A (Accumulator) + F (Flags)
 * - B + C
 * - D + E
 * - H + L
 * 
 * And 2 16-bit registers:
 * - SP (Stack Pointer)
 * - PC (Program Counter)
 */
class Registers {
public:
    // 8-bit registers
    u8 a;  // Accumulator
    u8 f;  // Flags register
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    
    // 16-bit registers
    u16 sp;  // Stack pointer
    u16 pc;  // Program counter
    
    // Flag register bit positions
    enum Flags : u8 {
        FLAG_Z = 0x80,  // Zero flag (bit 7)
        FLAG_N = 0x40,  // Subtract flag (bit 6)
        FLAG_H = 0x20,  // Half-carry flag (bit 5)
        FLAG_C = 0x10   // Carry flag (bit 4)
        // Bits 0-3 are always 0
    };
    
    // 16-bit register pair accessors
    u16 af() const { return (static_cast<u16>(a) << 8) | f; }
    u16 bc() const { return (static_cast<u16>(b) << 8) | c; }
    u16 de() const { return (static_cast<u16>(d) << 8) | e; }
    u16 hl() const { return (static_cast<u16>(h) << 8) | l; }
    
    void set_af(u16 value) { a = value >> 8; f = value & 0xF0; }  // Lower 4 bits always 0
    void set_bc(u16 value) { b = value >> 8; c = value & 0xFF; }
    void set_de(u16 value) { d = value >> 8; e = value & 0xFF; }
    void set_hl(u16 value) { h = value >> 8; l = value & 0xFF; }
    
    // Flag operations
    bool get_flag(Flags flag) const { return (f & flag) != 0; }
    void set_flag(Flags flag, bool value) {
        if (value) {
            f |= flag;
        } else {
            f &= ~flag;
        }
    }
    
    // Initialize registers to post-boot values
    void reset() {
        a = 0x01;  // After boot ROM
        f = 0xB0;
        b = 0x00;
        c = 0x13;
        d = 0x00;
        e = 0xD8;
        h = 0x01;
        l = 0x4D;
        sp = 0xFFFE;
        pc = 0x0100;  // Start of cartridge
    }
};

} // namespace emugbc
