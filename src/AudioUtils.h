//
// Created by victor on 26/05/25.
//

#ifndef AUDIOUTILS_H
#define AUDIOUTILS_H
#include <cstdint>
#include <vector>


struct DetectedNote {
    uint64_t start_sample;
    uint64_t duration; // In sample count
    float frequency;
};

std::vector<float> compute_envelope(const std::vector<float>& samples, int sample_rate, float window_ms);
std::vector<DetectedNote> detectNotes(const std::vector<float>& samples, int sample_rate);
float hann(std::size_t n, std::size_t N);



#endif //AUDIOUTILS_H
