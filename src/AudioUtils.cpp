//
// Created by victor on 26/05/25.
//

#include "AudioUtils.h"

#include <algorithm>
#include <complex>
#include <math.h>
#include <numeric>
#include "../libs/kiss_fft.hh"

std::vector<float> compute_envelope(const std::vector<float> &samples, int sample_rate, float window_ms) {
    if (samples.empty()) return {};

    std::vector<float> envelope_data;

    envelope_data.clear();
    envelope_data.reserve(samples.size());

    // Calculate window size in samples
    int window_samples = int(window_ms * 0.001f * sample_rate);
    window_samples = std::max(1, window_samples);

    // Calculate RMS envelope
    for (int i = 0; i < samples.size(); ++i) {
        float sum_squares = 0.0f;
        int count = 0;

        // Calculate RMS over window centered at current sample
        int start = std::max(0, i - window_samples / 2);
        int end = std::min((int)samples.size(), i + window_samples / 2);

        for (int j = start; j < end; ++j) {
            sum_squares += samples[j] * samples[j];
            count++;
        }

        float rms = std::sqrt(sum_squares / count);
        envelope_data.push_back(rms);
    }

    return envelope_data;
}

std::vector<DetectedNote> detectNotes(const std::vector<float> &samples, int sample_rate) {
    const std::size_t frameSize = 2048;
    const std::size_t hopSize   = 512;
    std::size_t numFrames = 1 + (samples.size() - frameSize) / hopSize;

    // 1) Pre-allocate
    std::vector<std::vector<std::complex<float>>> spectra(numFrames);
    std::vector<float> flux(numFrames, 0.0f);
    std::vector<float> pitches(numFrames, 0.0f);

    // 2) FFT & Pitch/Flux per frame
    kissfft<float> fft(frameSize, false);
    for (std::size_t n = 0; n < numFrames; ++n) {
        std::size_t offset = n * hopSize;
        // 2a) copy + window
        std::vector<std::complex<float>> X(frameSize);
        for (std::size_t i = 0; i < frameSize; ++i)
            X[i].real(samples[offset + i] * hann(i, frameSize));
        // 2b) FFT
        fft.transform(X.data(), X.data());
        spectra[n] = X;
        // 2c) spectral flux
        if (n > 0) {
            float sum = 0;
            for (std::size_t k = 0; k < frameSize/2; ++k) {
                float d = std::abs(X[k]) - std::abs(spectra[n-1][k]);
                sum += (d > 0 ? d : 0);
            }
            flux[n] = sum;
        }
        // 2d) peak-bin pitch
        std::size_t peak = std::max_element(
            spectra[n].begin(),
            spectra[n].begin() + frameSize/2,
            [](auto &a, auto &b){ return std::abs(a) < std::abs(b); }
        ) - spectra[n].begin();
        pitches[n] = (peak * sample_rate) / float(frameSize);
    }

    // 3) Find onsets
    float meanFlux = std::accumulate(flux.begin(), flux.end(), 0.0f) / numFrames;
    float varFlux = 0;
    for (auto f : flux) varFlux += (f-meanFlux)*(f-meanFlux);
    float thresh = meanFlux + 2 * std::sqrt(varFlux / numFrames);

    std::vector<std::size_t> onsets;
    for (std::size_t n = 1; n < numFrames; ++n)
        if (flux[n] > thresh)
            onsets.push_back(n);

    // 4) Build notes
    std::vector<DetectedNote> notes;
    for (std::size_t idx = 0; idx + 1 < onsets.size(); ++idx) {
        std::size_t i = onsets[idx], j = onsets[idx+1];
        uint64_t start = i * hopSize;
        uint64_t dur   = (j - i) * hopSize;

        // e.g. median pitch in [i..j)
        std::vector<float> blockPitches(
            pitches.begin() + i,
            pitches.begin() + j
        );
        std::nth_element(
            blockPitches.begin(),
            blockPitches.begin() + blockPitches.size()/2,
            blockPitches.end()
        );
        float freq = blockPitches[blockPitches.size()/2];

        notes.push_back({start, dur, freq});
    }

    return notes;
}

float hann(std::size_t n, std::size_t N) {
    return 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846 * n / (N - 1)));
}