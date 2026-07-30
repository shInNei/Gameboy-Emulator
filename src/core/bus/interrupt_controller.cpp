#include "interrupt_controller.hpp"

namespace gb {

void InterruptController::reset() {
    io_if = 0xE1;
    io_ie = 0x00;
    ime_flag = false;
    ei_delay = 0;
}

void InterruptController::request_interrupt(InterruptType type) {
    io_if |= (1 << static_cast<u8>(type));
}

void InterruptController::clear_interrupt(InterruptType type) {
    io_if &= ~(1 << static_cast<u8>(type));
}

void InterruptController::update_ei() {
    if (ei_delay > 0) {
        ei_delay--;
        if (ei_delay == 0) {
            ime_flag = true;
        }
    }
}

} // namespace gb
