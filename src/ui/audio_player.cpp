#define MINIAUDIO_IMPLEMENTATION
#include "vendor/miniaudio.h"
#include "audio_player.hpp"
#include <iostream>

namespace gb {

static void data_callback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount) {
    auto* player = static_cast<AudioPlayer*>(pDevice->pUserData);
    if (!player) return;

    auto* apu_ptr = reinterpret_cast<APU*>(pDevice->pUserData);
    if (apu_ptr) {
        apu_ptr->get_samples(static_cast<float*>(pOutput), frameCount);
    }
}

AudioPlayer::~AudioPlayer() {
    shutdown();
}

bool AudioPlayer::init(uint32_t sample_rate) {
    auto* device = new ma_device();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = sample_rate;
    config.dataCallback      = [](ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount) {
        auto* player = static_cast<AudioPlayer*>(pDevice->pUserData);
        if (player) {
            player->apu.get_samples(static_cast<float*>(pOutput), frameCount);
            
            float vol = player->get_volume();
            if (vol != 1.0f) {
                float* out = static_cast<float*>(pOutput);
                for (ma_uint32 i = 0; i < frameCount * 2; ++i) {
                    out[i] *= vol;
                }
            }
        }
    };
    config.pUserData         = this;

    if (ma_device_init(NULL, &config, device) != MA_SUCCESS) {
        delete device;
        return false;
    }

    if (ma_device_start(device) != MA_SUCCESS) {
        ma_device_uninit(device);
        delete device;
        return false;
    }

    device_ptr = device;
    initialized = true;
    return true;
}

void AudioPlayer::shutdown() {
    if (initialized && device_ptr) {
        auto* device = static_cast<ma_device*>(device_ptr);
        ma_device_uninit(device);
        delete device;
        device_ptr = nullptr;
        initialized = false;
    }
}

} // namespace gb
