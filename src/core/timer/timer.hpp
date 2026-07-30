#pragma once

#include "core/types.hpp"
#include "core/quirks.hpp"
#include "core/bus/interrupt_controller.hpp"

namespace gb {

class Timer {
public:
    Timer(InterruptController& interrupts, const HardwareQuirks& quirks)
        : interrupt_controller(interrupts), quirks(quirks) {}

    void reset();
    void tick(u8 t_cycles);

    [[nodiscard]] u8 read_div() const { return static_cast<u8>(internal_counter >> 8); }
    [[nodiscard]] u8 read_tima() const { return tima; }
    [[nodiscard]] u8 read_tma() const { return tma; }
    [[nodiscard]] u8 read_tac() const { return tac | 0xF8; }

    void write_div(u8 val);
    void write_tima(u8 val);
    void write_tma(u8 val);
    void write_tac(u8 val);

private:
    void check_multiplexer();

    InterruptController& interrupt_controller;
    const HardwareQuirks& quirks;

    u16 internal_counter{0xABCC}; // DMG boot state
    u8 tima{0x00};
    u8 tma{0x00};
    u8 tac{0xF8};

    bool prev_and_result{false};
    bool tima_reload_pending{false};
    u8 tima_reload_delay_cycles{0};
};

} // namespace gb
