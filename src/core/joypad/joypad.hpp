#pragma once

#include "core/types.hpp"
#include "core/bus/interrupt_controller.hpp"

namespace gb {

enum class Key : u8 {
    Right  = 0,
    Left   = 1,
    Up     = 2,
    Down   = 3,
    A      = 4,
    B      = 5,
    Select = 6,
    Start  = 7
};

class Joypad {
public:
    explicit Joypad(InterruptController& interrupts) : interrupt_controller(interrupts) {}

    void reset();

    void key_down(Key key);
    void key_up(Key key);

    [[nodiscard]] u8 read() const;
    void write(u8 val);

private:
    InterruptController& interrupt_controller;

    // Bit 0: Right/A, Bit 1: Left/B, Bit 2: Up/Select, Bit 3: Down/Start
    u8 dpad{0x0F};
    u8 buttons{0x0F};

    bool select_dpad{true};
    bool select_buttons{true};
};

} // namespace gb
