// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#pragma once

#include "types.h"

namespace gbglow {

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
    u8 a = 0;  // Accumulator
    u8 f = 0;  // Flags register
    u8 b = 0;
    u8 c = 0;
    u8 d = 0;
    u8 e = 0;
    u8 h = 0;
    u8 l = 0;
    
    // 16-bit registers
    u16 sp = 0;  // Stack pointer
    u16 pc = 0;  // Program counter
    
    // Flag register bit positions
    enum Flags : u8 {
        FLAG_Z = 0x80,  // Zero flag (bit 7)
        FLAG_N = 0x40,  // Subtract flag (bit 6)
        FLAG_H = 0x20,  // Half-carry flag (bit 5)
        FLAG_C = 0x10   // Carry flag (bit 4)
        // Bits 0-3 are always 0
    };
    
    // 16-bit register pair accessors
    u16 af() const
    {
        return (static_cast<u16>(a) << 8) | f;
    }
    
    u16 bc() const
    {
        return (static_cast<u16>(b) << 8) | c;
    }
    
    u16 de() const
    {
        return (static_cast<u16>(d) << 8) | e;
    }
    
    u16 hl() const
    {
        return (static_cast<u16>(h) << 8) | l;
    }
    
    void set_af(u16 value)
    {
        a = value >> 8;
        f = value & 0xF0;  // Lower 4 bits always 0
    }
    
    void set_bc(u16 value)
    {
        b = value >> 8;
        c = value & 0xFF;
    }
    
    void set_de(u16 value)
    {
        d = value >> 8;
        e = value & 0xFF;
    }
    
    void set_hl(u16 value)
    {
        h = value >> 8;
        l = value & 0xFF;
    }
    
    // Flag operations
    bool get_flag(Flags flag) const
    {
        return (f & flag) != 0;
    }
    
    void set_flag(Flags flag, bool value);
    
    // Initialize registers to post-boot values
    void reset();
};

} // namespace gbglow
