#include <thread>
#include <chrono>
#include "apu.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

void APU::reset() {
    ch1.reset();
    ch2.reset();
    ch3.reset();
    ch4.reset();

    nr50 = 0x77;
    nr51 = 0xF3;
    nr52 = 0xF1;
    master_enable = true;

    frame_sequencer_counter = 0;
    frame_sequencer_step = 0;
    sample_counter = 0.0;

    write_pos = 0;
    read_pos = 0;
}

void APU::tick(u16 t_cycles) {
    if (!master_enable) return;

    // Tick channels
    for (u16 i = 0; i < t_cycles; ++i) {
        ch1.tick();
        ch2.tick();
        ch3.tick();
        ch4.tick();

        // 512Hz Frame Sequencer clock (2048 M-cycles = 8192 T-cycles)
        frame_sequencer_counter++;
        if (frame_sequencer_counter >= 8192) {
            frame_sequencer_counter -= 8192;
            tick_frame_sequencer();
        }

        // Downsampling engine to 44.1kHz
        sample_counter += 1.0;
        if (sample_counter >= CYCLES_PER_SAMPLE) {
            sample_counter -= CYCLES_PER_SAMPLE;
            generate_sample();
        }
    }
}

void APU::tick_frame_sequencer() {
    switch (frame_sequencer_step) {
        case 0:
            ch1.tick_length(); ch2.tick_length(); ch3.tick_length(); ch4.tick_length();
            break;
        case 1:
            break;
        case 2:
            ch1.tick_length(); ch2.tick_length(); ch3.tick_length(); ch4.tick_length();
            ch1.tick_sweep();
            break;
        case 3:
            break;
        case 4:
            ch1.tick_length(); ch2.tick_length(); ch3.tick_length(); ch4.tick_length();
            break;
        case 5:
            break;
        case 6:
            ch1.tick_length(); ch2.tick_length(); ch3.tick_length(); ch4.tick_length();
            ch1.tick_sweep();
            break;
        case 7:
            ch1.tick_envelope(); ch2.tick_envelope(); ch4.tick_envelope();
            break;
    }
    frame_sequencer_step = (frame_sequencer_step + 1) & 0x07;
}

void APU::generate_sample() {
    u8 s1 = mute_ch1 ? 0 : ch1.sample();
    u8 s2 = mute_ch2 ? 0 : ch2.sample();
    u8 s3 = mute_ch3 ? 0 : ch3.sample();
    u8 s4 = mute_ch4 ? 0 : ch4.sample();

    float left_mix = 0.0f;
    float right_mix = 0.0f;

    // Terminal mapping (NR51)
    if (bit::test(nr51, 0)) right_mix += s1;
    if (bit::test(nr51, 1)) right_mix += s2;
    if (bit::test(nr51, 2)) right_mix += s3;
    if (bit::test(nr51, 3)) right_mix += s4;

    if (bit::test(nr51, 4)) left_mix += s1;
    if (bit::test(nr51, 5)) left_mix += s2;
    if (bit::test(nr51, 6)) left_mix += s3;
    if (bit::test(nr51, 7)) left_mix += s4;

    // Master volume scaling (NR50)
    u8 vol_right = (nr50 & 0x07) + 1;
    u8 vol_left = ((nr50 >> 4) & 0x07) + 1;

    // Normalize 4 channels * max vol 15 * max volume 8 to [-1.0, 1.0]
    constexpr float MAX_SUM = 4.0f * 15.0f * 8.0f;

    AudioSample sample;
    sample.left = (left_mix * vol_left) / MAX_SUM;
    sample.right = (right_mix * vol_right) / MAX_SUM;

    while (true) {
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            size_t next_write = (write_pos + 1) % BUFFER_SIZE;
            if (next_write != read_pos) {
                ring_buffer[write_pos] = sample;
                write_pos = next_write;
                break;
            } else if (!sync_to_audio) {
                // If not syncing, overwrite oldest sample and drop
                ring_buffer[write_pos] = sample;
                write_pos = next_write;
                read_pos = (read_pos + 1) % BUFFER_SIZE;
                break;
            }
        }
        // Buffer is full and syncing is enabled. Sleep briefly to wait for audio thread.
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

u8 APU::read_register(Address addr) const {
    switch (addr) {
        case 0xFF10: return ch1.read_nrx0();
        case 0xFF11: return ch1.read_nrx1();
        case 0xFF12: return ch1.read_nrx2();
        case 0xFF13: return ch1.read_nrx3();
        case 0xFF14: return ch1.read_nrx4();
        case 0xFF16: return ch2.read_nrx1();
        case 0xFF17: return ch2.read_nrx2();
        case 0xFF18: return ch2.read_nrx3();
        case 0xFF19: return ch2.read_nrx4();
        case 0xFF24: return nr50;
        case 0xFF25: return nr51;
        case 0xFF26: {
            u8 val = (master_enable ? 0x80 : 0x00) | 0x70;
            if (ch1.is_enabled()) val |= 0x01;
            if (ch2.is_enabled()) val |= 0x02;
            if (ch3.is_enabled()) val |= 0x04;
            if (ch4.is_enabled()) val |= 0x08;
            return val;
        }
        default: return 0xFF;
    }
}

void APU::write_register(Address addr, u8 val) {
    if (!master_enable && addr != 0xFF26) return;

    switch (addr) {
        case 0xFF10: ch1.write_nrx0(val); break;
        case 0xFF11: ch1.write_nrx1(val); break;
        case 0xFF12: ch1.write_nrx2(val); break;
        case 0xFF13: ch1.write_nrx3(val); break;
        case 0xFF14: ch1.write_nrx4(val); break;
        case 0xFF16: ch2.write_nrx1(val); break;
        case 0xFF17: ch2.write_nrx2(val); break;
        case 0xFF18: ch2.write_nrx3(val); break;
        case 0xFF19: ch2.write_nrx4(val); break;
        case 0xFF1A: ch3.write_nr30(val); break;
        case 0xFF1B: ch3.write_nr31(val); break;
        case 0xFF1C: ch3.write_nr32(val); break;
        case 0xFF1D: ch3.write_nr33(val); break;
        case 0xFF1E: ch3.write_nr34(val); break;
        case 0xFF20: ch4.write_nr41(val); break;
        case 0xFF21: ch4.write_nr42(val); break;
        case 0xFF22: ch4.write_nr43(val); break;
        case 0xFF23: ch4.write_nr44(val); break;
        case 0xFF24: nr50 = val; break;
        case 0xFF25: nr51 = val; break;
        case 0xFF26:
            master_enable = bit::test(val, 7);
            if (!master_enable) {
                ch1.reset(); ch2.reset(); ch3.reset(); ch4.reset();
                nr50 = 0; nr51 = 0;
            }
            break;
        default: break;
    }
}

void APU::get_samples(float* buffer, size_t num_samples) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    for (size_t i = 0; i < num_samples; ++i) {
        if (read_pos != write_pos) {
            const auto& s = ring_buffer[read_pos];
            buffer[i * 2] = s.left;
            buffer[i * 2 + 1] = s.right;
            read_pos = (read_pos + 1) % BUFFER_SIZE;
        } else {
            buffer[i * 2] = 0.0f;
            buffer[i * 2 + 1] = 0.0f;
        }
    }
}

size_t APU::available_samples() const {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (write_pos >= read_pos) {
        return write_pos - read_pos;
    }
    return BUFFER_SIZE - (read_pos - write_pos);
}

} // namespace gb
