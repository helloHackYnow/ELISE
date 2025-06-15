//
// Created by victor on 26/05/25.
//

#ifndef WAVEFORMVIEWER_H
#define WAVEFORMVIEWER_H

#include <atomic>
#include <functional>
#include <set>
#include <vector>

#include "imgui.h"
#include "AudioUtils.h"
#include "LightManager.h"



class WaveformViewer {
public:
    std::function<void(int64_t)> keyframe_creation_callback = nullptr;
    std::function<void()> keyframe_deletion_callback = nullptr;
    std::function<void(int64_t)> keyframe_drag_callback = nullptr;
    std::function<void(int64_t)> keyframe_selection_callback = nullptr;

    std::function<void(int64_t)> keyframe_unselection_callback = nullptr;
    std::function<void()> reset_selection_callback = nullptr;

private:
    std::vector<float> waveform_data;
    float sample_rate = 44100.0f;
    float horizontal_zoom = 1.0f;
    float vertical_zoom = 1.0f;
    float horizontal_offset = 0.0f; // Sample
    float cursor_position = 0.0f; // Sample

    std::vector<Keyframe> keyframes;
    std::set<int64_t> selected_keyframes;
    int selected_keyframe_index = -1;
    bool dragging_cursor = false;
    bool dragging_keyframe = false;

    bool is_auto_scroll_enabled = false;

    // Debug window
    bool should_draw_debug = false;

    // View parameters
    float min_horizontal_zoom = 0.0001f;
    float max_horizontal_zoom = 100.0f;
    float min_vertical_zoom = 0.001f;
    float max_vertical_zoom = 10.0f;

    // Audio
    bool show_waveform = true;

    // Notes
    std::vector<DetectedNote> notes;
    bool show_notes = false;
    std::atomic_bool computing_notes = false;
    float note_window_ms = 10.0f;

    // Envelope
    std::vector<float> envelope_data;
    bool show_envelope = false;
    std::atomic_bool computing_envelope = false;
    float envelope_window_ms = 10.0f;

    // Gradient preview
    int64_t gradient_start; // In sample
    int64_t gradient_duration; // In sample
    bool draw_gradient_preview;




private:


    void drawMenuBar();
    void drawGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawWaveform(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawEnvelope(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawNotes(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawCursor(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawKeyframes(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawGradientPreview(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawSelectedKeyFrameTimestamp(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void drawTimeScale(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, float scale_height);
    void drawDebugWindow();

    void handleInput(ImVec2 canvas_pos, ImVec2 canvas_size);

    float getSamplesPerPixel() const;
    float sampleToPixel(float sample, float canvas_width) const;
    float pixelToSample(float pixel, float canvas_width) const;
    float amplitudeToPixel(float amplitude, float canvas_height) const;

    void update_offset(float canvas_width);

    void detect_notes();
    void computeEnvelope();

    int get_first_note_at_sample(int sample) const;

public:
    WaveformViewer();
    void draw();

    void set_cursor_position(float cursor_position);
    float get_cursor_position();

    void set_sample_rate(float sample_rate);
    void set_waveform_data(const std::vector<float>& waveform_data);

    void set_keyframes(const std::vector<Keyframe>& keyframes);
    void set_selected_keyframe(const std::set<int64_t>& selected_keyframes_uuid);

    void set_gradient_preview(int64_t start, int64_t duration);
};



#endif //WAVEFORMVIEWER_H
