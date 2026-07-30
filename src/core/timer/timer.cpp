#include "timer.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

void Timer::reset() {
    internal_counter = 0x0000;
    tima = 0x00;
    tma = 0x00;
    tac = 0xF8;
    prev_and_result = false;
    tima_reload_pending = false;
    tima_reload_delay_cycles = 0;
}

static constexpr int BIT_POSITIONS[] = {9, 3, 5, 7}; // 4096Hz, 262144Hz, 65536Hz, 16384Hz

void Timer::check_multiplexer() {
    bool timer_enabled = bit::test(tac, 2);
    int bit_pos = BIT_POSITIONS[tac & 0x03];
    bool bit_val = bit::test(internal_counter, bit_pos);
    bool current_and_result = timer_enabled && bit_val;

    // Falling edge detector triggers TIMA increment
    if (prev_and_result && !current_and_result) {
        if (tima == 0xFF) {
            tima = 0x00;
            tima_reload_pending = true;
            tima_reload_delay_cycles = 4; // 1 M-cycle delay
        } else {
            tima++;
        }
    }
    prev_and_result = current_and_result;
}

void Timer::tick(u8 t_cycles) {
    for (u8 i = 0; i < t_cycles; ++i) {
        internal_counter++;
        check_multiplexer();

        if (tima_reload_pending) {
            if (tima_reload_delay_cycles > 0) {
                tima_reload_delay_cycles--;
            }
            if (tima_reload_delay_cycles == 0) {
                tima = tma;
                interrupt_controller.request_interrupt(InterruptType::Timer);
                tima_reload_pending = false;
            }
        }
    }
}

void Timer::write_div(u8) {
    internal_counter = 0;
    check_multiplexer();
}

void Timer::write_tima(u8 val) {
    if (tima_reload_pending && quirks.tima_write_during_reload) {
        tima = val;
        tima_reload_pending = false;
    } else {
        tima = val;
    }
}

void Timer::write_tma(u8 val) {
    tma = val;
    if (tima_reload_pending && tima_reload_delay_cycles == 0) {
        tima = val;
    }
}

void Timer::write_tac(u8 val) {
    tac = val & 0x07;
    check_multiplexer();
}

} // namespace gb
