// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 gbglow Contributors
// This file is part of gbglow. See LICENSE for details.

#include "registers.h"

namespace gbglow {

// DMG post-boot register values
// These match what the Game Boy hardware sets after executing the boot ROM
namespace {
    constexpr u8  POST_BOOT_A  = 0x01;
    constexpr u8  POST_BOOT_F  = 0xB0;
    constexpr u8  POST_BOOT_B  = 0x00;
    constexpr u8  POST_BOOT_C  = 0x13;
    constexpr u8  POST_BOOT_D  = 0x00;
    constexpr u8  POST_BOOT_E  = 0xD8;
    constexpr u8  POST_BOOT_H  = 0x01;
    constexpr u8  POST_BOOT_L  = 0x4D;
    constexpr u16 POST_BOOT_SP = 0xFFFE;
    constexpr u16 POST_BOOT_PC = 0x0100;  // Cartridge entry point
} // anonymous namespace

void Registers::set_flag(Flags flag, bool value)
{
    if (value)
    {
        f |= flag;
    }
    else
    {
        f &= ~flag;
    }
}

void Registers::reset()
{
    // Initialize to post-boot ROM state
    // These values match what the Game Boy hardware sets after boot sequence
    a = POST_BOOT_A;
    f = POST_BOOT_F;
    b = POST_BOOT_B;
    c = POST_BOOT_C;
    d = POST_BOOT_D;
    e = POST_BOOT_E;
    h = POST_BOOT_H;
    l = POST_BOOT_L;
    sp = POST_BOOT_SP;
    pc = POST_BOOT_PC;
}

} // namespace gbglow
