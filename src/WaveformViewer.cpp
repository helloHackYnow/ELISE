//
// Created by victor on 26/05/25.
//

#include "WaveformViewer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <thread>


void WaveformViewer::drawMenuBar() {
    ImGui::BeginMenuBar();

    ImGui::Spacing();
    ImGui::Checkbox(" Waveform", &show_waveform);
    ImGui::Spacing();
    ImGui::BeginDisabled(computing_envelope);
    ImGui::Checkbox(" Envelope", &show_envelope);
    ImGui::EndDisabled();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Checkbox("Auto scroll", &is_auto_scroll_enabled);
    ImGui::EndMenuBar();
}

void WaveformViewer::drawGrid(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {

    // Vertical grid lines (time)
    //---------------------------
    float samples_per_pixel = getSamplesPerPixel();
    float seconds_per_pixel = samples_per_pixel / sample_rate;

    // Determine grid spacing
    float grid_spacing_seconds = 0.1f;
    if (seconds_per_pixel > 0.05f) grid_spacing_seconds = 1.0f;
    else if (seconds_per_pixel > 0.005f) grid_spacing_seconds = 0.1f;
    else if (seconds_per_pixel > 0.0005f) grid_spacing_seconds = 0.01f;
    else grid_spacing_seconds = 0.001f;

    float grid_spacing_samples = grid_spacing_seconds * sample_rate;
    float start_sample = horizontal_offset;
    float start_grid = floor(start_sample / grid_spacing_samples) * grid_spacing_samples;

    for (float sample = start_grid; sample < start_sample + canvas_size.x * samples_per_pixel;
         sample += grid_spacing_samples) {
        float x = sampleToPixel(sample, canvas_size.x);
        if (x >= 0 && x <= canvas_size.x) {
            draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y),
                             ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y),
                             IM_COL32(60, 60, 60, 100));
        }
         }

    // Horizontal grid lines (amplitude)
    // ---------------------------------
    for (int i = -10; i <= 10; ++i) {
        float amplitude = i * 0.1f;
        float y = amplitudeToPixel(amplitude, canvas_size.y);
        if (y >= 0 && y <= canvas_size.y) {
            ImU32 color = (i == 0) ? IM_COL32(100, 100, 100, 150) : IM_COL32(60, 60, 60, 100);
            draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + y),
                             ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y),
                             color);
        }
    }

}

void WaveformViewer::drawWaveform(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {
    if (waveform_data.empty()) return;

    float samples_per_pixel = getSamplesPerPixel();
    std::vector<ImVec2> points;

    for (int x = 0; x < canvas_size.x; ++x) {
        float sample_pos = horizontal_offset + x * samples_per_pixel;

        if (sample_pos >= waveform_data.size()) break;

        float amplitude = 0.0f;
        if (samples_per_pixel < 1.0f) {
            // Interpolate when zoomed in
            int idx = (int)sample_pos;
            float frac = sample_pos - idx;
            if (idx + 1 < waveform_data.size()) {
                amplitude = waveform_data[idx] * (1.0f - frac) + waveform_data[idx + 1] * frac;
            } else {
                amplitude = waveform_data[idx];
            }
        } else {
            // Average when zoomed out
            int start_idx = (int)sample_pos;
            int end_idx = std::min((int)(sample_pos + samples_per_pixel), (int)waveform_data.size());
            float sum = 0.0f;
            for (int i = start_idx; i < end_idx; ++i) {
                sum += waveform_data[i];
            }
            amplitude = sum / (end_idx - start_idx);
        }

        float y = amplitudeToPixel(amplitude, canvas_size.y);
        points.push_back(ImVec2(canvas_pos.x + x, canvas_pos.y + y));
    }

    // Draw waveform
    for (int i = 1; i < points.size(); ++i) {
        draw_list->AddLine(points[i-1], points[i], IM_COL32(100, 200, 255, 255), 1.0f);
    }
}

void WaveformViewer::drawEnvelope(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {
    if (!show_envelope || envelope_data.empty()) return;

    float samples_per_pixel = getSamplesPerPixel();
    std::vector<ImVec2> envelope_points_pos, envelope_points_neg;

    for (int x = 0; x < canvas_size.x; ++x) {
        float sample_pos = horizontal_offset + x * samples_per_pixel;

        if (sample_pos >= envelope_data.size()) break;

        float amplitude = 0.0f;
        if (samples_per_pixel < 1.0f) {
            // Interpolate when zoomed in
            int idx = (int)sample_pos;
            float frac = sample_pos - idx;
            if (idx + 1 < envelope_data.size()) {
                amplitude = envelope_data[idx] * (1.0f - frac) + envelope_data[idx + 1] * frac;
            } else {
                amplitude = envelope_data[idx];
            }
        } else {
            // Average when zoomed out
            int start_idx = (int)sample_pos;
            int end_idx = std::min((int)(sample_pos + samples_per_pixel), (int)envelope_data.size());
            float sum = 0.0f;
            for (int i = start_idx; i < end_idx; ++i) {
                sum += envelope_data[i];
            }
            amplitude = sum / (end_idx - start_idx);
        }

        float y_pos = amplitudeToPixel(amplitude, canvas_size.y);
        float y_neg = amplitudeToPixel(-amplitude, canvas_size.y);

        envelope_points_pos.push_back(ImVec2(canvas_pos.x + x, canvas_pos.y + y_pos));
        envelope_points_neg.push_back(ImVec2(canvas_pos.x + x, canvas_pos.y + y_neg));
    }

    // Draw positive envelope
    for (int i = 1; i < envelope_points_pos.size(); ++i) {
        draw_list->AddLine(envelope_points_pos[i-1], envelope_points_pos[i],
                         IM_COL32(178, 251, 165, 210), 1.5f);
    }

    // Draw negative envelope
    for (int i = 1; i < envelope_points_neg.size(); ++i) {
        draw_list->AddLine(envelope_points_neg[i-1], envelope_points_neg[i],
                         IM_COL32(178, 251, 165, 210), 1.5f);
    }

    // Optional: Fill between envelopes for a more solid look
    if (envelope_points_pos.size() > 1) {
        for (int i = 0; i < envelope_points_pos.size(); ++i) {
            draw_list->AddLine(envelope_points_pos[i], envelope_points_neg[i],
                             IM_COL32(178, 251, 165, 70), 1.0f);
        }
    }
}

void WaveformViewer::drawNotes(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {
    if (notes.empty()) return;

    auto min_visible_sample = horizontal_offset;
    auto max_visible_sample = horizontal_offset + canvas_size.x * getSamplesPerPixel();

    int note_idx = get_first_note_at_sample(min_visible_sample);

    while (note_idx < (int)notes.size() && notes[note_idx].start_sample < max_visible_sample) {
        auto &note = notes[note_idx];

        // Draw note
        float x_start = sampleToPixel(note.start_sample, canvas_size.x);
        float x_end = sampleToPixel(note.start_sample + note.duration, canvas_size.x);
        draw_list->AddRectFilled(ImVec2(canvas_pos.x + x_start, canvas_pos.y + canvas_size.y / 2 - (note.frequency - 20000)/100),
                               ImVec2(canvas_pos.x + x_end, canvas_pos.y + canvas_size.y / 2),
                               IM_COL32(255, 255, 255, 180), 2.0f);
        note_idx++;
    }
}

void WaveformViewer::drawCursor(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {
    float cursor_x = sampleToPixel(cursor_position, canvas_size.x);
    static float cursor_tip_width = 9.0f;
    static float cursor_padding = 5.0f;
    static auto cursor_color = IM_COL32(255, 0, 0, 180);
    if (cursor_x >= 0 && cursor_x <= canvas_size.x) {
        draw_list->AddLine(ImVec2(canvas_pos.x + cursor_x, canvas_pos.y + cursor_padding),
                         ImVec2(canvas_pos.x + cursor_x, canvas_pos.y + canvas_size.y - cursor_padding),
                         cursor_color, 2.0f);

        // Small horizontal lines
        draw_list->AddLine(ImVec2(canvas_pos.x + cursor_x - cursor_tip_width/2, canvas_pos.y + cursor_padding),
                            ImVec2(canvas_pos.x + cursor_x + cursor_tip_width/2, canvas_pos.y + cursor_padding),
                            cursor_color, 2.0f);

        draw_list->AddLine(ImVec2(canvas_pos.x + cursor_x - cursor_tip_width/2, canvas_pos.y + canvas_size.y - cursor_padding),
                            ImVec2(canvas_pos.x + cursor_x + cursor_tip_width/2, canvas_pos.y + canvas_size.y - cursor_padding),
                            cursor_color, 2.0f);
    }
}

void WaveformViewer::drawKeyframes(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {

    selected_keyframe_index = -1;
    for (int i = 0; i < keyframes.size(); ++i) {
        float keyframe_x = sampleToPixel(keyframes[i].trigger_sample, canvas_size.x);

        // Draw keyframe handle at top (larger, easier to click)
        static ImVec2 rect_size{5, 10};
        static float rec_rounding = 5.f;
        ImVec2 handle_center = ImVec2(canvas_pos.x + keyframe_x, canvas_pos.y + rect_size.y + 2);

        static ImU32 disabled_color = IM_COL32(120, 120, 120, 255);
        static ImU32 locked_color   = IM_COL32(255, 100, 120, 255);
        static ImU32 default_color   = IM_COL32(255, 200, 100, 255);

        if (keyframe_x >= -10 && keyframe_x <= canvas_size.x + 10) {

            ImU32 color = default_color;
            if (!keyframes[i].is_enabled) color = disabled_color;
            else if (keyframes[i].is_locked) color = locked_color;

            if (selected_keyframes.contains(keyframes[i].uuid)) {
                selected_keyframe_index = i;

                static const ImU32 halo_colors[] = {
                    IM_COL32(100, 150, 255, 60),  // Outer (most transparent)
                    IM_COL32(100, 150, 255, 100), // Middle
                    IM_COL32(100, 150, 255, 140)  // Inner (most opaque)
                };
                static const float halo_sizes[] = {4.0f, 2.5f, 1.5f};

                // Draw multiple layers for smoother glow
                for (int layer = 0; layer < 3; ++layer) {
                    draw_list->AddRectFilled(
                        ImVec2(handle_center.x - rect_size.x - halo_sizes[layer],
                               handle_center.y - rect_size.y - halo_sizes[layer]),
                        ImVec2(handle_center.x + rect_size.x + halo_sizes[layer],
                               handle_center.y + rect_size.y + halo_sizes[layer]),
                        halo_colors[layer], rec_rounding + halo_sizes[layer]);

                    draw_list->AddRectFilled(
                        ImVec2(canvas_pos.x + keyframe_x - halo_sizes[layer],
                               canvas_pos.y + 15 - halo_sizes[layer]),
                        ImVec2(canvas_pos.x + keyframe_x + halo_sizes[layer],
                               canvas_pos.y + canvas_size.y + halo_sizes[layer]),
                        halo_colors[layer], rec_rounding + halo_sizes[layer]);
                }
            }

            // Draw keyframe line
            draw_list->AddLine(ImVec2(canvas_pos.x + keyframe_x, canvas_pos.y + 15),
                             ImVec2(canvas_pos.x + keyframe_x, canvas_pos.y + canvas_size.y),
                             color, 1.5f);

            draw_list->AddRectFilled(ImVec2(handle_center.x - rect_size.x, handle_center.y - rect_size.y),
                                   ImVec2(handle_center.x + rect_size.x, handle_center.y + rect_size.y),
                                   color, rec_rounding);

            draw_list->AddRect(ImVec2(handle_center.x - rect_size.x, handle_center.y - rect_size.y),
                             ImVec2(handle_center.x + rect_size.x, handle_center.y + rect_size.y),
                             IM_COL32(0, 0, 0, 150), rec_rounding, 0, 1.0f);
        }
    }
}

void WaveformViewer::drawGradientPreview(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size)
{
    if (draw_gradient_preview)
    {
        float gradient_preview_height = 30.f;

        ImVec2 gradient_pos_1{
            sampleToPixel(static_cast<float>(gradient_start), canvas_size.x) + canvas_pos.x,
            canvas_pos.y + canvas_size.y - gradient_preview_height };

        ImVec2 gradient_pos_2{
            sampleToPixel(std::max(static_cast<float>(gradient_start + gradient_duration), 0.0f), canvas_size.x) + canvas_pos.x,
            canvas_pos.y + canvas_size.y };

        draw_list->AddRectFilled(
            gradient_pos_1,
            gradient_pos_2,
            ImColor(100, 230, 255, 150),
            std::min(10.f, std::max(gradient_pos_2.x - gradient_pos_1.x, 0.f)),

            ImDrawFlags_RoundCornersTopRight
        );

        draw_list->AddRect(
            gradient_pos_1,
            gradient_pos_2,
            ImColor(100*0.8, 230*0.8, 255*0.8, 255),
            std::min(10.f, std::max(gradient_pos_2.x - gradient_pos_1.x, 0.f)),
            ImDrawFlags_RoundCornersTopRight,
            2.0f
        );
    }
}

void WaveformViewer::drawSelectedKeyFrameTimestamp(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size)
{
    if (selected_keyframes.size() == 1 && selected_keyframe_index >= 0) {
        // Get the on-screen position
        float keyframe_x = sampleToPixel(keyframes[selected_keyframe_index].trigger_sample, canvas_size.x);

        float timestamp_x = keyframe_x + 10;

        int timestamp_ms = keyframes[selected_keyframe_index].trigger_sample * 1000 / sample_rate;

        char buff[128];
        sprintf(buff, "%i ms", timestamp_ms);

        ImVec2 text_size = ImGui::CalcTextSize(buff);
        ImVec2 text_pos{ timestamp_x + 10, canvas_pos.y + 2 };

        draw_list->AddRectFilled(
            ImVec2(text_pos.x - 3, text_pos.y),
            ImVec2(text_pos.x + text_size.x + 3, text_pos.y + text_size.y + 3), 
            ImColor(25, 25, 25, 220), 3.f);

        draw_list->AddText(text_pos, ImColor(250, 250, 250, 255), buff);
    }
}

void WaveformViewer::drawTimeScale(ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, float scale_height) {
    ImVec2 scale_pos = ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y);

        // Clear scale background
        draw_list->AddRectFilled(scale_pos,
                               ImVec2(scale_pos.x + canvas_size.x, scale_pos.y + scale_height),
                               IM_COL32(25, 25, 25, 255));

        // Draw scale border
        draw_list->AddLine(ImVec2(scale_pos.x, scale_pos.y),
                         ImVec2(scale_pos.x + canvas_size.x, scale_pos.y),
                         IM_COL32(100, 100, 100, 255));

        float samples_per_pixel = getSamplesPerPixel();
        float seconds_per_pixel = samples_per_pixel / 44100.0f;

        // Determine appropriate scale spacing based on zoom level
        // TODO : Wtf is this AI garbage
        float scale_spacing_seconds;

        if (seconds_per_pixel > 2.0f) scale_spacing_seconds = 10.0f;
        else if (seconds_per_pixel > 1.0f) scale_spacing_seconds = 5.0f;
        else if (seconds_per_pixel > 0.5f) scale_spacing_seconds = 2.0f;
        else if (seconds_per_pixel > 0.1f) scale_spacing_seconds = 1.0f;
        else if (seconds_per_pixel > 0.05f) scale_spacing_seconds = 0.5f;
        else if (seconds_per_pixel > 0.02f) scale_spacing_seconds = 0.2f;
        else if (seconds_per_pixel > 0.01f) scale_spacing_seconds = 0.1f;
        else if (seconds_per_pixel > 0.005f) scale_spacing_seconds = 0.05f;
        else if (seconds_per_pixel > 0.002f) scale_spacing_seconds = 0.02f;
        else if (seconds_per_pixel > 0.001f) scale_spacing_seconds = 0.01f;
        else if (seconds_per_pixel > 0.0005f) scale_spacing_seconds = 0.005f;
        else scale_spacing_seconds = 0.001f;

        float scale_spacing_samples = scale_spacing_seconds * 44100.0f;
        float start_sample = horizontal_offset;
        float start_scale = floor(start_sample / scale_spacing_samples) * scale_spacing_samples;

        // Calculate minimum pixel spacing to avoid overcrowding
        float min_pixel_spacing = 50.0f;
        float current_pixel_spacing = scale_spacing_samples / samples_per_pixel;

        // Skip drawing if scales would be too close together
        if (current_pixel_spacing < min_pixel_spacing) {
            // Try to find a suitable multiplier
            int multiplier = (int)ceil(min_pixel_spacing / current_pixel_spacing);
            scale_spacing_samples *= multiplier;
            scale_spacing_seconds *= multiplier;
            start_scale = floor(start_sample / scale_spacing_samples) * scale_spacing_samples;
        }

        for (float sample = start_scale; sample < start_sample + canvas_size.x * samples_per_pixel;
             sample += scale_spacing_samples) {
            float x = sampleToPixel(sample, canvas_size.x);
            if (x >= -20 && x <= canvas_size.x + 20) {
                // Draw tick mark
                draw_list->AddLine(ImVec2(scale_pos.x + x, scale_pos.y),
                                 ImVec2(scale_pos.x + x, scale_pos.y + 8),
                                 IM_COL32(150, 150, 150, 255), 1.0f);

                // Draw time label
                float time_seconds = sample / 44100.0f;
                char time_str[32];

                if (scale_spacing_seconds >= 1.0f) {
                    snprintf(time_str, sizeof(time_str), "%.1fs", time_seconds);
                } else if (scale_spacing_seconds >= 0.1f) {
                    snprintf(time_str, sizeof(time_str), "%.2fs", time_seconds);
                } else {
                    snprintf(time_str, sizeof(time_str), "%.3fs", time_seconds);
                }

                ImVec2 text_size = ImGui::CalcTextSize(time_str);
                draw_list->AddText(ImVec2(scale_pos.x + x - text_size.x * 0.5f, scale_pos.y + 10),
                                 IM_COL32(180, 180, 180, 255), time_str);
            }
        }
    }

void WaveformViewer::drawDebugWindow() {
    if (ImGui::Begin("Debug Info", &should_draw_debug)) {

        ImGui::Text("Zoom: H=%.2f, V=%.2f", horizontal_zoom, vertical_zoom);
        ImGui::Text("Offset: %.2f samples", horizontal_offset);
        ImGui::Text("Cursor: %.4f samples (%.4fs)", cursor_position, cursor_position / 44100.0f);
        ImGui::Text("Keyframes: %d", (int)keyframes.size());
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("Ctrl+Scroll: Vertical Zoom");
        ImGui::BulletText("Shift+Scroll: Horizontal Zoom");
        ImGui::BulletText("Scroll: Pan Horizontally");
        ImGui::BulletText("Click: Move Cursor");
        ImGui::BulletText("Enter: Add Keyframe");
        ImGui::BulletText("Delete: Remove Selected Keyframe");
        ImGui::BulletText("Drag Keyframe Handles: Move Keyframes");

        ImGui::Separator();
        ImGui::Text("Cursor");
        ImGui::Checkbox("Follow Cursor", &is_auto_scroll_enabled);

        ImGui::Separator();
        ImGui::Text("Audio Processing:");

        ImGui::Separator();
        ImGui::Text("Envelope Visualization:");
        ImGui::Checkbox("Show Waveform", &show_waveform);

        ImGui::BeginDisabled(computing_notes.load());
        ImGui::Checkbox("Show Notes", &show_notes);
        ImGui::SameLine();
        if (ImGui::Button("Compute notes")) detect_notes();
        ImGui::EndDisabled();

        ImGui::BeginDisabled(computing_envelope.load());
        ImGui::Checkbox("Show Envelope", &show_envelope);
        ImGui::EndDisabled();

        ImGui::SliderFloat("Envelope Window (ms)", &envelope_window_ms, 1.0f, 100.0f, "%.1f ms");


        if (ImGui::Button("Recalculate Envelope")) {
            computeEnvelope();
        }

        ImGui::End();
    }
}

void WaveformViewer::handleInput(ImVec2 canvas_pos, ImVec2 canvas_size) {

    ImGuiIO& io = ImGui::GetIO();

        // Handle mouse wheel
        if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
            if (io.KeyCtrl) {
                // Vertical zoom
                float zoom_factor = 1.0f + io.MouseWheel * 0.1f;
                vertical_zoom = std::clamp(vertical_zoom * zoom_factor,
                                         min_vertical_zoom, max_vertical_zoom);
            } else if (io.KeyShift) {

                float old_center_sample = pixelToSample(canvas_size.x / 2, canvas_size.x);

                // Horizontal zoom
                float zoom_factor = 1.0f + io.MouseWheel * 0.1f;
                float old_zoom = horizontal_zoom;
                horizontal_zoom = std::clamp(horizontal_zoom * zoom_factor,
                                           min_horizontal_zoom, max_horizontal_zoom);

                float new_center_sample = pixelToSample(canvas_size.x / 2, canvas_size.x);

                float sample_shift = old_center_sample - new_center_sample;
                horizontal_offset += sample_shift;

            } else {
                // Horizontal scroll
                float samples_per_pixel = getSamplesPerPixel();
                horizontal_offset -= io.MouseWheel * 50.f * samples_per_pixel;
            }

            // Clamp horizontal offset
            horizontal_offset = std::max(0.0f, std::min(horizontal_offset,
                                        (float)waveform_data.size() - canvas_size.x * getSamplesPerPixel()));
        }

        // Handle mouse clicks and dragging
        if (ImGui::IsWindowHovered()) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float mouse_x = mouse_pos.x - canvas_pos.x;

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

                // Check if clicking on keyframe handle
                float mouse_y = mouse_pos.y - canvas_pos.y;
                bool clicked = false;
                for (int i = 0; i < keyframes.size(); ++i) {
                    float keyframe_x = sampleToPixel(keyframes[i].trigger_sample, canvas_size.x);
                    // Check if clicking on the handle (top 20 pixels)
                    if (abs(mouse_x - keyframe_x) < 8.0f && mouse_y < 20.0f) {

                        if (ImGui::IsKeyDown(ImGuiMod_Ctrl)) {
                            if (!selected_keyframes.contains(keyframes[i].uuid)) {
                                keyframe_selection_callback(keyframes[i].uuid);
                                selected_keyframes.insert(keyframes[i].uuid);
                            } else {
                                keyframe_unselection_callback(keyframes[i].uuid);
                                selected_keyframes.erase(keyframes[i].uuid);
                            }
                        } else if (ImGui::IsKeyDown(ImGuiMod_Shift)) {

                            if (selected_keyframes.size() != 1) {
                                reset_selection_callback();
                                selected_keyframes.clear();

                                keyframe_selection_callback(keyframes[i].uuid);
                                selected_keyframes.insert(keyframes[i].uuid);
                            } else {
                                keyframe_selection_callback(keyframes[i].uuid);
                                selected_keyframes.insert(keyframes[i].uuid);

                                auto a = keyframes[selected_keyframe_index].trigger_sample;
                                auto b = keyframes[i].trigger_sample;

                                auto start = std::min(a, b);
                                auto end = std::max(a, b);

                                for (auto & keyframe: keyframes) {
                                    if (start < keyframe.trigger_sample && keyframe.trigger_sample < end) {
                                        keyframe_selection_callback(keyframe.uuid);
                                        selected_keyframes.insert(keyframe.uuid);
                                    }
                                }
                            }

                        } else {
                            keyframe_selection_callback(keyframes[i].uuid);
                            selected_keyframes.insert(keyframes[i].uuid);
                        }

                        dragging_keyframe = true;
                        clicked = true;
                        break;
                    }
                }

                if (!clicked) {
                    // Move cursor
                    reset_selection_callback();
                    cursor_position = pixelToSample(mouse_x, canvas_size.x);
                    cursor_position = std::clamp(cursor_position, 0.0f, (float)waveform_data.size());
                    dragging_cursor = true;
                }
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                if (dragging_keyframe && !selected_keyframes.empty()) {
                    auto delta = io.MouseDelta.x;
                    keyframe_drag_callback(pixelToSample(delta, canvas_size.x) - horizontal_offset);

                } else if (dragging_cursor) {
                    cursor_position = pixelToSample(mouse_x, canvas_size.x);
                    cursor_position = std::clamp(cursor_position, 0.0f, (float)waveform_data.size());
                }
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                dragging_cursor = false;
                dragging_keyframe = false;
            }
        } else {
            dragging_cursor = false;
            dragging_keyframe = false;
        }

        // Handle Enter key for adding keyframes
        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            // Check if keyframe already exists at cursor position
            bool exists = false;
            for (auto& kf : keyframes) {
                if (abs(kf.trigger_sample - cursor_position) < getSamplesPerPixel()) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                keyframe_creation_callback(cursor_position);
            }
        }

        // Handle Delete key for removing selected keyframe
        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && selected_keyframes.size() > 0) {
            keyframe_deletion_callback();
        }
}

float WaveformViewer::getSamplesPerPixel() const {
    return 1.0f / horizontal_zoom;
}

float WaveformViewer::sampleToPixel(float sample, float canvas_width) const {
    return (sample - horizontal_offset) * horizontal_zoom;
}

float WaveformViewer::pixelToSample(float pixel, float canvas_width) const {
    return horizontal_offset + pixel / horizontal_zoom;
}

float WaveformViewer::amplitudeToPixel(float amplitude, float canvas_height) const {
    return canvas_height * 0.5f - amplitude * vertical_zoom * canvas_height * 0.4f;
}

void WaveformViewer::update_offset(float canvas_width) {
    if (is_auto_scroll_enabled) {
        if (cursor_position > horizontal_offset + canvas_width / 4 / horizontal_zoom) {
            horizontal_offset = cursor_position - canvas_width / 4 / horizontal_zoom;
        }
    }
}

void WaveformViewer::detect_notes() {
    if (!computing_notes.load()) {
        computing_notes.store(true);
        std::thread detection_thread([this]() {
            notes = detectNotes(waveform_data, sample_rate);
            computing_notes.store(false);
        });
        detection_thread.detach();
    }
}

void WaveformViewer::computeEnvelope() {
    if (!computing_envelope.load()) {
        computing_envelope.store(true);
        std::thread envelope_thread([this]() {
            envelope_data = compute_envelope(waveform_data, sample_rate, envelope_window_ms);
            computing_envelope.store(false);
        });
        envelope_thread.detach();
    }
}

int WaveformViewer::get_first_note_at_sample(int sample) const {

    int a = 0;
    int b = notes.size() - 1;

    while (a <= b) {
        int m = (a + b) / 2;
        int note_end = notes[m].start_sample + notes[m].duration;

        if (note_end <= sample) {
            a = m + 1;
        } else {
            if (m == 0 || notes[m - 1].start_sample + notes[m - 1].duration <= sample) {
                return m;
            }
            b = m - 1;
        }
    }

    return -1;
}

WaveformViewer::WaveformViewer() {

}

void WaveformViewer::draw() {
    // Reserve space for time scale at bottom
    const float scale_height = 25.0f;


    ImGui::Begin("Waveform Viewer", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar);

    drawMenuBar();

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 available_size = ImGui::GetContentRegionAvail();
    ImVec2 canvas_size = ImVec2(available_size.x, available_size.y - scale_height);

    if (canvas_size.x < 50.0f || canvas_size.y < 50.0f) {
        ImGui::End();
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Handle input
    handleInput(canvas_pos, canvas_size);
    update_offset(canvas_size.x);

    // Clear background
    draw_list->AddRectFilled(canvas_pos,
                           ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                           IM_COL32(30, 30, 30, 255));

    // Draw grid
    drawGrid(draw_list, canvas_pos, canvas_size);

    // Draw waveform
    if (show_waveform) drawWaveform(draw_list, canvas_pos, canvas_size);

    // Draw envelope
    drawEnvelope(draw_list, canvas_pos, canvas_size);

    // Draw notes
    if (show_notes) drawNotes(draw_list, canvas_pos, canvas_size);

    // Draw cursor
    drawCursor(draw_list, canvas_pos, canvas_size);

    // Draw keyframes
    drawKeyframes(draw_list, canvas_pos, canvas_size);

    drawSelectedKeyFrameTimestamp(draw_list, canvas_pos, canvas_size);
    drawGradientPreview(draw_list, canvas_pos, canvas_size);

    // Draw time scale
    drawTimeScale(draw_list, canvas_pos, canvas_size, scale_height);

    // Invisible button for input handling
    ImGui::SetCursorScreenPos(canvas_pos);
    // ImGui::InvisibleButton("canvas", ImVec2(canvas_size.x, canvas_size.y + scale_height),
    //                       ImGuiButtonFlags_MouseButtonLeft |
    //                       ImGuiButtonFlags_MouseButtonRight);

    if (should_draw_debug) drawDebugWindow();

    draw_gradient_preview = false;

    ImGui::End();
}

void WaveformViewer::set_cursor_position(float cursor_position) {
    this->cursor_position = cursor_position;
}

float WaveformViewer::get_cursor_position() {
    return cursor_position;
}

void WaveformViewer::set_sample_rate(float sample_rate) {
    this->sample_rate = sample_rate;
}

void WaveformViewer::set_waveform_data(const std::vector<float>& waveform_data) {
    this->waveform_data = waveform_data;
    computeEnvelope();
}

void WaveformViewer::set_keyframes(const std::vector<Keyframe>& keyframes) {
    this->keyframes = keyframes;
}

void WaveformViewer::set_selected_keyframe(const std::set<int64_t> &selected_keyframes_uuid) {
    selected_keyframes = selected_keyframes_uuid;
}

void WaveformViewer::set_gradient_preview(int64_t start, int64_t duration)
{
    gradient_start = start;
    gradient_duration = duration;

    draw_gradient_preview = true;
}

