//
// Created by victor on 26/05/25.
//

#include "AudioManager.h"

#include <cstring>

AudioManager::AudioManager() {

}

AudioManager::~AudioManager() {
    stop();
}

bool AudioManager::loadMP3(const std::string& path) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, sample_rate);
    ma_decoder decoder;

    if (ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS)
        return false;

    ma_uint64 frameCount;
    float* audioData = nullptr;

    if (ma_decode_file(path.c_str(), &config, &frameCount, (void**)&audioData) != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        return false;
    }

    original_samples.assign(audioData, audioData + frameCount);
    sample_rate = decoder.outputSampleRate;

    ma_free(audioData, nullptr);
    ma_decoder_uninit(&decoder);

    return true;
}

const std::vector<float> & AudioManager::getOriginalSamples() const {
    return original_samples;
}

int AudioManager::getSampleRate() const {
    return sample_rate;
}

void AudioManager::play(const int start_ms) {
    if (is_playing) return;

    if (!isDeviceInitialized) {
        initPlaybackDevice();
    }

    // Clamp starting position
    size_t startFrame = size_t(start_ms * sample_rate / 1000.0f);
    playheadPosition = std::min(startFrame, original_samples.size());

    is_playing = true;
    ma_device_start(&device);
}

void AudioManager::pause() {
    is_playing = false;
}

void AudioManager::stop() {
    if (isDeviceInitialized) {
        ma_device_uninit(&device);
        isDeviceInitialized = false;
    }
    is_playing = false;
    playheadPosition = 0;
}

bool AudioManager::isPlaying() const {
    return is_playing;
}

size_t AudioManager::getPlayheadPosition() const {
    return playheadPosition;
}

void AudioManager::dataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    auto* self = static_cast<AudioManager*>(pDevice->pUserData);
    float* out = (float*)pOutput;
    memset(out, 0, frameCount * sizeof(float));

    if (!self->is_playing) return;

    const auto& samples = self->original_samples;
    size_t remaining = samples.size() - self->playheadPosition;

    ma_uint32 framesToCopy = (ma_uint32)std::min<size_t>(frameCount, remaining);

    memcpy(out, samples.data() + self->playheadPosition, framesToCopy * sizeof(float));
    self->playheadPosition += framesToCopy;

    if (framesToCopy < frameCount) {
        // Reached end
        self->is_playing = false;
    }
}

void AudioManager::initPlaybackDevice() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.sampleRate = sample_rate;
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.dataCallback = dataCallback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize playback device.");
    }
}
