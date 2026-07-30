#include "serial.hpp"
#include "utils/bit_utils.hpp"
#include <iostream>

namespace gb {

void Serial::reset() {
    sb = 0x00;
    sc = 0x7E;
    serial_output.clear();
    transfer_in_progress = false;
    transfer_cycles = 0;
}

void Serial::write_sc(u8 val) {
    sc = val;
    if (bit::test(sc, 7)) { // Start Transfer
        bool internal_clock = bit::test(sc, 0);
        if (internal_clock) {
            transfer_in_progress = true;
            transfer_cycles = 512; // 8 bits @ 8192Hz clock speed = 512 T-cycles
            
            char c = static_cast<char>(sb);
            if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t') {
                serial_output += c;
                if (callback) callback(c);
            }
        } else {
            // External clock: No link cable attached, transfer does not run automatically
            transfer_in_progress = false;
        }
    }
}

void Serial::tick(u16 t_cycles) {
    if (!transfer_in_progress) return;

    if (transfer_cycles <= t_cycles) {
        transfer_in_progress = false;
        transfer_cycles = 0;
        sb = 0xFF; // Disconnected cable reads 0xFF
        bit::clear(sc, 7); // Transfer complete
        if (callback) {
            callback(static_cast<char>(sb));
        }
        interrupt_controller.request_interrupt(InterruptType::Serial);
    } else {
        transfer_cycles -= t_cycles;
    }
}
} // namespace gb
