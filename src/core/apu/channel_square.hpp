#pragma once

#include "core/types.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

class ChannelSquare {
public:
    explicit ChannelSquare(bool has_sweep = false) : is_ch1(has_sweep) {}

    void reset() {
        nrx0 = 0; nrx1 = 0; nrx2 = 0; nrx3 = 0; nrx4 = 0;
        enabled = false;
        timer = 0;
        duty_pos = 0;
        length_counter = 0;
        volume = 0;
        envelope_timer = 0;
        sweep_timer = 0;
        shadow_freq = 0;
        sweep_enabled = false;
    }

    void tick() {
        if (timer > 0) {
            timer--;
        } else {
            reload_timer();
            duty_pos = (duty_pos + 1) & 0x07;
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
        if (envelope_timer > 0) {
            envelope_timer--;
        }
        if (envelope_timer == 0) {
            envelope_timer = envelope_period;
            if (envelope_add && volume < 15) {
                volume++;
            } else if (!envelope_add && volume > 0) {
                volume--;
            }
        }
    }

    void tick_sweep() {
        if (!is_ch1) return;
        if (sweep_timer > 0) sweep_timer--;
        if (sweep_timer == 0) {
            sweep_timer = sweep_period == 0 ? 8 : sweep_period;
            if (sweep_enabled && sweep_period > 0) {
                u16 new_freq = calculate_sweep_freq();
                if (new_freq <= 2047 && sweep_shift > 0) {
                    shadow_freq = new_freq;
                    nrx3 = static_cast<u8>(new_freq & 0xFF);
                    nrx4 = (nrx4 & 0xF8) | static_cast<u8>((new_freq >> 8) & 0x07);
                    reload_timer();
                    calculate_sweep_freq(); // Check again for overflow
                }
            }
        }
    }

    [[nodiscard]] u8 sample() const {
        if (!enabled || volume == 0) return 0;
        static constexpr u8 DUTY_CYCLES[4][8] = {
            {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
            {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
            {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
            {0, 1, 1, 1, 1, 1, 1, 0}  // 75%
        };
        u8 wave = DUTY_CYCLES[duty_pattern][duty_pos];
        return wave ? volume : 0;
    }

    void trigger() {
        enabled = true;
        if (length_counter == 0) length_counter = 64;
        reload_timer();
        envelope_timer = envelope_period;
        volume = initial_volume;

        if (is_ch1) {
            shadow_freq = frequency();
            sweep_timer = sweep_period == 0 ? 8 : sweep_period;
            sweep_enabled = (sweep_period > 0 || sweep_shift > 0);
            if (sweep_shift > 0) {
                calculate_sweep_freq();
            }
        }
    }

    // Register accessors
    void write_nrx0(u8 val) {
        if (!is_ch1) return;
        nrx0 = val;
        sweep_period = (val >> 4) & 0x07;
        sweep_negate = bit::test(val, 3);
        sweep_shift = val & 0x07;
    }
    void write_nrx1(u8 val) {
        nrx1 = val;
        duty_pattern = (val >> 6) & 0x03;
        length_counter = 64 - (val & 0x3F);
    }
    void write_nrx2(u8 val) {
        nrx2 = val;
        initial_volume = (val >> 4) & 0x0F;
        envelope_add = bit::test(val, 3);
        envelope_period = val & 0x07;
        if ((val & 0xF8) == 0) enabled = false; // DAC power off
    }
    void write_nrx3(u8 val) { nrx3 = val; }
    void write_nrx4(u8 val) {
        nrx4 = val;
        length_enabled = bit::test(val, 6);
        if (bit::test(val, 7)) trigger();
    }

    [[nodiscard]] u8 read_nrx0() const { return nrx0 | 0x80; }
    [[nodiscard]] u8 read_nrx1() const { return nrx1 | 0x3F; }
    [[nodiscard]] u8 read_nrx2() const { return nrx2; }
    [[nodiscard]] u8 read_nrx3() const { return 0xFF; }
    [[nodiscard]] u8 read_nrx4() const { return nrx4 | 0xBF; }

    [[nodiscard]] bool is_enabled() const { return enabled; }

private:
    [[nodiscard]] u16 frequency() const {
        return ((static_cast<u16>(nrx4 & 0x07)) << 8) | nrx3;
    }

    void reload_timer() {
        timer = (2048 - frequency()) * 4;
    }

    u16 calculate_sweep_freq() {
        u16 new_freq = shadow_freq >> sweep_shift;
        if (sweep_negate) {
            new_freq = shadow_freq - new_freq;
        } else {
            new_freq = shadow_freq + new_freq;
        }
        if (new_freq > 2047) {
            enabled = false;
        }
        return new_freq;
    }

    bool is_ch1{false};
    u8 nrx0{0}, nrx1{0}, nrx2{0}, nrx3{0}, nrx4{0};

    bool enabled{false};
    u16 timer{0};
    u8 duty_pattern{0};
    u8 duty_pos{0};
    u16 length_counter{0};
    bool length_enabled{false};

    u8 initial_volume{0};
    u8 volume{0};
    bool envelope_add{false};
    u8 envelope_period{0};
    u8 envelope_timer{0};

    // Sweep CH1
    u8 sweep_period{0};
    bool sweep_negate{false};
    u8 sweep_shift{0};
    u8 sweep_timer{0};
    u16 shadow_freq{0};
    bool sweep_enabled{false};
};

} // namespace gb
