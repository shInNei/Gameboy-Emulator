#pragma once

#include "core/types.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

class ChannelNoise {
public:
    void reset() {
        nr41 = 0; nr42 = 0; nr43 = 0; nr44 = 0;
        enabled = false;
        timer = 0;
        lfsr = 0x7FFF;
        length_counter = 0;
        volume = 0;
        envelope_timer = 0;
    }

    void tick() {
        if (timer > 0) {
            timer--;
        } else {
            reload_timer();
            u8 xor_result = (lfsr & 0x01) ^ ((lfsr >> 1) & 0x01);
            lfsr >>= 1;
            lfsr |= (xor_result << 14);
            if (width_mode) {
                lfsr &= ~(1 << 6);
                lfsr |= (xor_result << 6);
            }
        }
    }

    void tick_length() {
        if (length_enabled && length_counter > 0) {
            length_counter--;
            if (length_counter == 0) {
                enabled = false;
            }
        }
    }

    void tick_envelope() {
        if (envelope_period == 0) return;
        if (envelope_timer > 0) envelope_timer--;
        if (envelope_timer == 0) {
            envelope_timer = envelope_period;
            if (envelope_add && volume < 15) {
                volume++;
            } else if (!envelope_add && volume > 0) {
                volume--;
            }
        }
    }

    [[nodiscard]] u8 sample() const {
        if (!enabled || volume == 0) return 0;
        return (lfsr & 0x01) ? 0 : volume;
    }

    void trigger() {
        enabled = true;
        if (length_counter == 0) length_counter = 64;
        reload_timer();
        lfsr = 0x7FFF;
        envelope_timer = envelope_period;
        volume = initial_volume;
    }

    void write_nr41(u8 val) {
        nr41 = val;
        length_counter = 64 - (val & 0x3F);
    }
    void write_nr42(u8 val) {
        nr42 = val;
        initial_volume = (val >> 4) & 0x0F;
        envelope_add = bit::test(val, 3);
        envelope_period = val & 0x07;
        if ((val & 0xF8) == 0) enabled = false;
    }
    void write_nr43(u8 val) {
        nr43 = val;
        clock_shift = (val >> 4) & 0x0F;
        width_mode = bit::test(val, 3);
        divisor_code = val & 0x07;
    }
    void write_nr44(u8 val) {
        nr44 = val;
        length_enabled = bit::test(val, 6);
        if (bit::test(val, 7)) trigger();
    }

    [[nodiscard]] bool is_enabled() const { return enabled; }

private:
    void reload_timer() {
        static constexpr u16 DIVISORS[8] = {8, 16, 32, 48, 64, 80, 96, 112};
        timer = DIVISORS[divisor_code] << clock_shift;
    }

    u8 nr41{0}, nr42{0}, nr43{0}, nr44{0};
    bool enabled{false};
    u16 timer{0};
    u16 lfsr{0x7FFF};

    u16 length_counter{0};
    bool length_enabled{false};

    u8 initial_volume{0};
    u8 volume{0};
    bool envelope_add{false};
    u8 envelope_period{0};
    u8 envelope_timer{0};

    u8 clock_shift{0};
    bool width_mode{false};
    u8 divisor_code{0};
};

} // namespace gb
