#include "registers.h"

namespace gbcrush {

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
    a = 0x01;
    f = 0xB0;
    b = 0x00;
    c = 0x13;
    d = 0x00;
    e = 0xD8;
    h = 0x01;
    l = 0x4D;
    sp = 0xFFFE;
    pc = 0x0100;  // Cartridge entry point
}

} // namespace gbcrush
