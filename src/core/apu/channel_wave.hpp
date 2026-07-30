#pragma once

#include "core/types.hpp"
#include "utils/bit_utils.hpp"
#include <array>

namespace gb {

class ChannelWave {
public:
    void reset() {
        nr30 = 0; nr31 = 0; nr32 = 0; nr33 = 0; nr34 = 0;
        enabled = false;
        timer = 0;
        sample_pos = 0;
        length_counter = 0;
        wave_ram.fill(0);
    }

    void tick() {
        if (timer > 0) {
            timer--;
        } else {
            reload_timer();
            sample_pos = (sample_pos + 1) & 31;
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

    [[nodiscard]] u8 sample() const {
        if (!enabled || !dac_enabled) return 0;
        u8 byte_val = wave_ram[sample_pos / 2];
        u8 nibble = (sample_pos % 2 == 0) ? (byte_val >> 4) : (byte_val & 0x0F);

        switch (volume_shift) {
            case 0: return 0;       // Mute
            case 1: return nibble;  // 100%
            case 2: return nibble >> 1; // 50%
            case 3: return nibble >> 2; // 25%
            default: return 0;
        }
    }

    void trigger() {
        enabled = dac_enabled;
        if (length_counter == 0) length_counter = 256;
        reload_timer();
        sample_pos = 0;
    }

    void write_nr30(u8 val) {
        nr30 = val;
        dac_enabled = bit::test(val, 7);
        if (!dac_enabled) enabled = false;
    }
    void write_nr31(u8 val) {
        nr31 = val;
        length_counter = 256 - val;
    }
    void write_nr32(u8 val) {
        nr32 = val;
        volume_shift = (val >> 5) & 0x03;
    }
    void write_nr33(u8 val) { nr33 = val; }
    void write_nr34(u8 val) {
        nr34 = val;
        length_enabled = bit::test(val, 6);
        if (bit::test(val, 7)) trigger();
    }

    u8 read_wave_ram(Address addr) const {
        size_t offset = addr - 0xFF30;
        if (offset < 16) return wave_ram[offset];
        return 0xFF;
    }

    void write_wave_ram(Address addr, u8 val) {
        size_t offset = addr - 0xFF30;
        if (offset < 16) wave_ram[offset] = val;
    }

    [[nodiscard]] bool is_enabled() const { return enabled; }

private:
    [[nodiscard]] u16 frequency() const {
        return ((static_cast<u16>(nr34 & 0x07)) << 8) | nr33;
    }

    void reload_timer() {
        timer = (2048 - frequency()) * 2;
    }

    u8 nr30{0}, nr31{0}, nr32{0}, nr33{0}, nr34{0};
    bool enabled{false};
    bool dac_enabled{false};
    u16 timer{0};
    u8 sample_pos{0};
    u16 length_counter{0};
    bool length_enabled{false};
    u8 volume_shift{0};

    std::array<u8, 16> wave_ram{};
};

} // namespace gb
