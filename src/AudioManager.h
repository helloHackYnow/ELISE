//
// Created by victor on 26/05/25.
//

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H


#include "../libs/kiss_fft.hh"
#include "../libs/miniaudio.h"
#include <thread>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool loadMP3(const std::string& path);

    const std::vector<float>& getOriginalSamples() const;
    int getSampleRate() const;

    void play(int start_ms = 0);
    void pause();
    void stop();

private:
    static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    void initPlaybackDevice();


private:
    std::vector<float> original_samples;
    int sample_rate = 44100;

    ma_device device{};
    bool isDeviceInitialized = false;

    std::atomic<bool> isPlaying{false};
    std::atomic<uint64_t> playheadPosition{0};



};



#endif //AUDIOMANAGER_H
