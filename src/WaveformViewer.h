//
// Created by victor on 26/05/25.
//

#ifndef WAVEFORMVIEWER_H
#define WAVEFORMVIEWER_H

#include <vector>

#include "imgui.h"

class WaveformViewer {
private:
    std::vector<float> waveform_data;
    float sample_rate = 44100.0f;
    float horizontal_zoom = 1.0f;
    float vertical_zoom = 1.0f;
    float horizontal_offset = 0.0f;
    float cursor_position = 0.0f;
    std::vector<float> keyframes;
    int selected_keyframe = -1;
    bool dragging_cursor = false;
    bool dragging_keyframe = false;

    // Debug window
    bool should_draw_debug = true;

    // View parameters
    float min_horizontal_zoom = 0.0001f;
    float max_horizontal_zoom = 100.0f;
    float min_vertical_zoom = 0.001f;
    float max_vertical_zoom = 10.0f;

    // Audio
    std::vector<float> envelope_data;
    bool show_envelope = true;
    float envelope_window_ms = 10.0f; // 10ms window for envelope calculation


private:
    void computeEnvelope();

    void drawMenuBar();
    void drawGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawWaveform(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawEnvelope(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawCursor(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawKeyframes(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawTimeScale(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, float scale_height);
    void drawDebugWindow();

    void handleInput(ImVec2 canvas_pos, ImVec2 canvas_size);

    float getSamplesPerPixel() const;
    float sampleToPixel(float sample, float canvas_width) const;
    float pixelToSample(float pixel, float canvas_width) const;
    float amplitudeToPixel(float amplitude, float canvas_height) const;

public:
    WaveformViewer();
    void draw();
    void set_cursor_position(float cursor_position);

};



#endif //WAVEFORMVIEWER_H
