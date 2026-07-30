#pragma once

#include "core/types.hpp"
#include "core/bus/interrupt_controller.hpp"
#include <string>
#include <functional>

namespace gb {

class Serial {
public:
    explicit Serial(InterruptController& interrupts) : interrupt_controller(interrupts) {}

    void reset();
    void tick(u16 t_cycles);

    [[nodiscard]] u8 read_sb() const { return sb; }
    [[nodiscard]] u8 read_sc() const { return sc | 0x7E; }

    void write_sb(u8 val) { sb = val; }
    void write_sc(u8 val);

    [[nodiscard]] const std::string& get_output() const { return serial_output; }
    void clear_output() { serial_output.clear(); }

    void set_output_callback(std::function<void(char)> cb) { callback = cb; }

private:
    InterruptController& interrupt_controller;

    u8 sb{0x00};
    u8 sc{0x7E};
    std::string serial_output;
    std::function<void(char)> callback;

    bool transfer_in_progress{false};
    u16 transfer_cycles{0};
};

} // namespace gb
