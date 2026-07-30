#pragma once

#include "core/types.hpp"

namespace gb {

enum class InterruptType : u8 {
    VBlank = 0,
    LCDStat = 1,
    Timer = 2,
    Serial = 3,
    Joypad = 4
};

class InterruptController {
public:
    void reset();

    [[nodiscard]] u8 read_if() const { return io_if | 0xE0; }
    [[nodiscard]] u8 read_ie() const { return io_ie; }

    void write_if(u8 val) { io_if = val & 0x1F; }
    void write_ie(u8 val) { io_ie = val & 0x1F; }

    void request_interrupt(InterruptType type);
    void clear_interrupt(InterruptType type);

    [[nodiscard]] bool is_pending() const { return (io_if & io_ie & 0x1F) != 0; }
    [[nodiscard]] u8 pending_mask() const { return (io_if & io_ie & 0x1F); }

    [[nodiscard]] bool ime() const { return ime_flag; }
    void set_ime(bool enable) { ime_flag = enable; if (!enable) ei_delay = 0; }

    void schedule_ei() { ei_delay = 2; }
    void update_ei();

private:
    u8 io_if{0xE1};
    u8 io_ie{0x00};
    bool ime_flag{false};
    u8 ei_delay{0};
};

} // namespace gb
