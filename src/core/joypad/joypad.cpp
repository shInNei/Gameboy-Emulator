#include "joypad.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

void Joypad::reset() {
    dpad = 0x0F;
    buttons = 0x0F;
    select_dpad = true;
    select_buttons = true;
}

void Joypad::key_down(Key key) {
    u8 key_idx = static_cast<u8>(key);
    bool was_pressed = false;

    if (key_idx < 4) { // DPad
        was_pressed = !bit::test(dpad, key_idx);
        bit::clear(dpad, key_idx);
        if (!was_pressed && !select_dpad) {
            interrupt_controller.request_interrupt(InterruptType::Joypad);
        }
    } else { // Action buttons
        u8 btn_idx = key_idx - 4;
        was_pressed = !bit::test(buttons, btn_idx);
        bit::clear(buttons, btn_idx);
        if (!was_pressed && !select_buttons) {
            interrupt_controller.request_interrupt(InterruptType::Joypad);
        }
    }
}

void Joypad::key_up(Key key) {
    u8 key_idx = static_cast<u8>(key);
    if (key_idx < 4) {
        bit::set(dpad, key_idx);
    } else {
        bit::set(buttons, key_idx - 4);
    }
}

u8 Joypad::read() const {
    u8 res = 0xC0 | (select_buttons ? 0x20 : 0x00) | (select_dpad ? 0x10 : 0x00) | 0x0F;
    if (!select_dpad) {
        res &= (dpad | 0xF0);
    }
    if (!select_buttons) {
        res &= (buttons | 0xF0);
    }
    return res;
}

void Joypad::write(u8 val) {
    select_dpad = bit::test(val, 4);
    select_buttons = bit::test(val, 5);
}

} // namespace gb
