#include "timer.h"
#include "memory.h"

namespace emugbc {

Timer::Timer(Memory& memory)
    : memory_(memory)
    , div_counter_(0)
    , tima_counter_(0)
    , div_(0)
    , tima_(0)
    , tma_(0)
    , tac_(0)
{
}

void Timer::step(Cycles cycles) {
    // Update DIV register - always increments regardless of timer enable
    step_div_only(cycles);
    
    // Update TIMA register only if timer is enabled
    if (is_timer_enabled()) {
        tima_counter_ += cycles;
        u16 frequency = get_clock_frequency();
        
        while (tima_counter_ >= frequency) {
            tima_counter_ -= frequency;
            
            // Increment TIMA
            if (tima_ == 0xFF) {
                // Overflow: reload from TMA and trigger interrupt
                tima_ = tma_;
                request_timer_interrupt();
            } else {
                tima_++;
            }
        }
    }
}

void Timer::step_div_only(Cycles cycles) {
    div_counter_ += cycles;
    // Optimize: most CPU instructions are 4-24 cycles, DIV updates every 256
    // Only check/update when threshold is actually reached
    if (div_counter_ >= DIV_FREQUENCY) {
        // Calculate increments (rarely more than 1)
        u8 increments = div_counter_ / DIV_FREQUENCY;
        div_ += increments;
        div_counter_ %= DIV_FREQUENCY;
    }
}

u8 Timer::read_div() const {
    return div_;
}

u8 Timer::read_tima() const {
    return tima_;
}

u8 Timer::read_tma() const {
    return tma_;
}

u8 Timer::read_tac() const {
    return tac_;
}

void Timer::write_div(u8 value) {
    // Any write to DIV resets it to 0
    (void)value;  // Suppress unused parameter warning
    div_ = 0;
    div_counter_ = 0;
}

void Timer::write_tima(u8 value) {
    tima_ = value;
}

void Timer::write_tma(u8 value) {
    tma_ = value;
}

void Timer::write_tac(u8 value) {
    // Only lower 3 bits are used
    tac_ = value & 0x07;
    
    // Reset TIMA counter when changing configuration
    tima_counter_ = 0;
}

bool Timer::is_timer_enabled() const {
    return (tac_ & TAC_ENABLE_BIT) != 0;
}

u16 Timer::get_clock_frequency() const {
    u8 clock_select = tac_ & TAC_CLOCK_SELECT_MASK;
    
    switch (clock_select) {
        case 0x00:
            return CLOCK_FREQ_00;  // 4096 Hz
        case 0x01:
            return CLOCK_FREQ_01;  // 262144 Hz
        case 0x02:
            return CLOCK_FREQ_10;  // 65536 Hz
        case 0x03:
            return CLOCK_FREQ_11;  // 16384 Hz
        default:
            return CLOCK_FREQ_00;  // Unreachable
    }
}

void Timer::request_timer_interrupt() {
    // Set timer interrupt flag (bit 2 of IF register)
    u8 if_reg = memory_.read(REG_IF);
    memory_.write(REG_IF, if_reg | TIMER_INT_BIT);
}

} // namespace emugbc
