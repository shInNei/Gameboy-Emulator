#pragma once

#include "core/apu/apu.hpp"
#include <memory>

namespace gb {

class AudioPlayer {
public:
    explicit AudioPlayer(APU& apu) : apu(apu) {}
    ~AudioPlayer();

    bool init(uint32_t sample_rate = 44100);
    void shutdown();

    void set_volume(float vol) { volume = vol; }
    [[nodiscard]] float get_volume() const { return volume; }

private:
    APU& apu;
    float volume{1.0f};
    bool initialized{false};
    void* device_ptr{nullptr}; // Pointer to miniaudio device struct
};

} // namespace gb
