#include "joypad.h"
#include "../core/memory.h"
#include <iostream>

namespace gbglow {

Joypad::Joypad(Memory& memory)
    : memory_(memory)
    , button_a_(false)
    , button_b_(false)
    , button_start_(false)
    , button_select_(false)
    , dpad_up_(false)
    , dpad_down_(false)
    , dpad_left_(false)
    , dpad_right_(false)
    , select_buttons_(true)      // Not selected initially
    , select_directions_(true)    // Not selected initially
{
}

void Joypad::press_a() {
    if (!button_a_) {
        button_a_ = true;
        request_interrupt();
    }
}

void Joypad::release_a() {
    button_a_ = false;
}

void Joypad::press_b() {
    if (!button_b_) {
        button_b_ = true;
        request_interrupt();
    }
}

void Joypad::release_b() {
    button_b_ = false;
}

void Joypad::press_start() {
    if (!button_start_) {
        button_start_ = true;
        request_interrupt();
    }
}

void Joypad::release_start() {
    button_start_ = false;
}

void Joypad::press_select() {
    if (!button_select_) {
        button_select_ = true;
        request_interrupt();
    }
}

void Joypad::release_select() {
    button_select_ = false;
}

void Joypad::press_up() {
    if (!dpad_up_) {
        dpad_up_ = true;
        request_interrupt();
    }
}

void Joypad::release_up() {
    dpad_up_ = false;
}

void Joypad::press_down() {
    if (!dpad_down_) {
        dpad_down_ = true;
        request_interrupt();
    }
}

void Joypad::release_down() {
    dpad_down_ = false;
}

void Joypad::press_left() {
    if (!dpad_left_) {
        dpad_left_ = true;
        request_interrupt();
    }
}

void Joypad::release_left() {
    dpad_left_ = false;
}

void Joypad::press_right() {
    if (!dpad_right_) {
        dpad_right_ = true;
        request_interrupt();
    }
}

void Joypad::release_right() {
    dpad_right_ = false;
}

u8 Joypad::read_register() const {
    // Start with all inputs not pressed (bits set to 1)
    u8 result = INPUT_MASK;
    
    // If button keys are selected (bit 5 = 0, so select_buttons_ = true)
    if (select_buttons_) {
        if (button_a_) {
            result &= ~(1 << BIT_RIGHT_A);  // Clear bit 0 when A pressed
        }
        if (button_b_) {
            result &= ~(1 << BIT_LEFT_B);   // Clear bit 1 when B pressed
        }
        if (button_select_) {
            result &= ~(1 << BIT_UP_SELECT); // Clear bit 2 when Select pressed
        }
        if (button_start_) {
            result &= ~(1 << BIT_DOWN_START); // Clear bit 3 when Start pressed
        }
    }
    
    // If direction keys are selected (bit 4 = 0, so select_directions_ = true)
    if (select_directions_) {
        if (dpad_right_) {
            result &= ~(1 << BIT_RIGHT_A);  // Clear bit 0 when Right pressed
        }
        if (dpad_left_) {
            result &= ~(1 << BIT_LEFT_B);   // Clear bit 1 when Left pressed
        }
        if (dpad_up_) {
            result &= ~(1 << BIT_UP_SELECT); // Clear bit 2 when Up pressed
        }
        if (dpad_down_) {
            result &= ~(1 << BIT_DOWN_START); // Clear bit 3 when Down pressed
        }
    }
    
    // Add selection bits back - if NOT selected, set bit to 1
    if (!select_buttons_) {
        result |= (1 << BIT_SELECT_BUTTONS);
    }
    if (!select_directions_) {
        result |= (1 << BIT_SELECT_DIRECTIONS);
    }
    
    // Unused bits are always set to 1
    result |= UNUSED_MASK;
    
    return result;
}

void Joypad::write_register(u8 value) {
    // Only bits 4 and 5 are writable (selection bits)
    // Bit 5: Select button keys (0=select)
    // Bit 4: Select direction keys (0=select)
    select_buttons_ = (value & (BIT_NOT_PRESSED << BIT_SELECT_BUTTONS)) == 0;
    select_directions_ = (value & (BIT_NOT_PRESSED << BIT_SELECT_DIRECTIONS)) == 0;
}

void Joypad::request_interrupt() {
    // Set joypad interrupt flag (bit 4 of IF register)
    u8 if_reg = memory_.read(REG_IF);
    memory_.write(REG_IF, if_reg | JOYPAD_INT_BIT);
}

} // namespace gbglow
