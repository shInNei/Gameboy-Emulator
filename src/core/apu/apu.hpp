#pragma once

#include "core/types.hpp"
#include "core/quirks.hpp"
#include "channel_square.hpp"
#include "channel_wave.hpp"
#include "channel_noise.hpp"
#include <vector>
#include <mutex>

namespace gb {

struct AudioSample {
    float left{0.0f};
    float right{0.0f};
};

class APU {
public:
    explicit APU(const HardwareQuirks& quirks) : quirks(quirks), ch1(true), ch2(false) {}

    void reset();
    void tick(u16 t_cycles);

    [[nodiscard]] u8 read_register(Address addr) const;
    void write_register(Address addr, u8 val);

    u8 read_wave_ram(Address addr) const { return ch3.read_wave_ram(addr); }
    void write_wave_ram(Address addr, u8 val) { ch3.write_wave_ram(addr, val); }

    // Audio Output Buffer Interface
    void get_samples(float* buffer, size_t num_samples);
    [[nodiscard]] size_t available_samples() const;

    // Channel Mute controls for Debugger
    bool mute_ch1{false};
    bool mute_ch2{false};
    bool mute_ch3{false};
    bool mute_ch4{false};
    bool sync_to_audio{true};

    [[nodiscard]] const ChannelSquare& get_ch1() const { return ch1; }
    [[nodiscard]] const ChannelSquare& get_ch2() const { return ch2; }
    [[nodiscard]] const ChannelWave& get_ch3() const { return ch3; }
    [[nodiscard]] const ChannelNoise& get_ch4() const { return ch4; }

private:
    void tick_frame_sequencer();
    void generate_sample();

    const HardwareQuirks& quirks;

    ChannelSquare ch1;
    ChannelSquare ch2;
    ChannelWave ch3;
    ChannelNoise ch4;

    u8 nr50{0x77};
    u8 nr51{0xF3};
    u8 nr52{0xF1};
    bool master_enable{true};

    u16 frame_sequencer_counter{0};
    u8 frame_sequencer_step{0};

    // Sampling clock accumulator
    double sample_counter{0.0};
    static constexpr double TARGET_SAMPLE_RATE = 44100.0;
    static constexpr double CYCLES_PER_SAMPLE = CPU_CLOCK_HZ / TARGET_SAMPLE_RATE;

    // Ring Buffer for Audio Output
    static constexpr size_t BUFFER_SIZE = 8192;
    std::vector<AudioSample> ring_buffer{BUFFER_SIZE};
    size_t write_pos{0};
    size_t read_pos{0};
    mutable std::mutex buffer_mutex;
};

} // namespace gb
