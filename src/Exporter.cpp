//
// Created by victor on 29/05/25.
//

#include "Exporter.h"

#include <fstream>
#include <iostream>

std::string get_python_interpolation(const GradientKind &kind) {
    switch (kind) {
        case GradientKind::linear:
            return "Inter.LINEAR";
        case GradientKind::ease_in:
            return "Inter.EASE_IN";
        case GradientKind::ease_out:
            return "Inter.EASE_OUT";
        case GradientKind::ease_in_out:
            return "Inter.EASE_INOUT";
        default: return "Inter.LINEAR";
    }
}

std::string get_python_color(const Color &color) {
    return "(" + std::to_string(color.r) + ", " + std::to_string(color.g) + ", " +
        std::to_string(color.b) + ", " + std::to_string(color.a) + ")";
}

int sample_to_ms(int sample, int sample_rate) {
    return sample * 1000 / sample_rate;
}

std::string get_group_str(size_t group_id) {
    return "group_" + std::to_string(group_id);
}

std::string get_python_command(const Command &command, int sample_rate) {
    switch (command.animation.kind) {
        case AnimationKind::gradient: {
            std::string cmd = "gradient(";
            cmd += std::to_string(sample_to_ms(command.trigger_sample, sample_rate)) + ", ";
            cmd += get_group_str(command.group_id) + ", ";
            cmd += get_python_color(command.animation.gradient.start_color) + ", ";
            cmd += get_python_color(command.animation.gradient.end_color) + ", ";
            cmd += std::to_string(sample_to_ms(command.animation.gradient.duration, sample_rate)) + ", ";
            cmd += get_python_interpolation(command.animation.gradient.kind) + ")";
            return cmd;
        }

        case AnimationKind::toggle: {
            std::string cmd;
            if (command.animation.toggle.is_on) {
                cmd = "on(";
                cmd += std::to_string(sample_to_ms(command.trigger_sample, sample_rate)) + ", ";
                cmd += get_group_str(command.group_id) + ", ";
                cmd += get_python_color(command.animation.toggle.color) + ")";
            } else {
                cmd = "off(";
                cmd += std::to_string(sample_to_ms(command.trigger_sample, sample_rate)) + ", ";
                cmd += get_group_str(command.group_id) + ")";
            }

            return cmd;
        }
    }
}


std::string generate_python_script(const ProjectData &data) {

    std::string out = python_header;

    char buffer[1024];

    // Build light and groups definition
    for (int i = 0; i < data.light_count; ++i) {
        sprintf(buffer, "group_%d = add_light()", i);
        out += tab + std::string(buffer) + new_line;
    }

    out += new_line;

    for (int i = data.light_count; i < data.groups.size(); ++i) {
        std::string group_desc="(";
        for (auto light: data.groups.at(i).lights) {
            group_desc += "group_" + std::to_string(light) + ", ";
        }
        group_desc += ")";
        sprintf(buffer, "group_%d = new_group(%s)", i, group_desc.c_str());
        out += tab + std::string(buffer) + new_line;
    }

    out += new_line + new_line;

    // Generate commands
    for (auto & keyframe: data.keyframes) {
        for (auto &command: data.keyframe_uuid_to_commands.at(keyframe.uuid) ) {
            out += tab + get_python_command(command, data.sample_rate) + new_line;
        }
        out += new_line;
    }

    out += new_line;
    out += tab + "return main_return()" + new_line;

    return out;
}

void save_python_script(const std::string &path, const std::string &script) {
    std::ofstream file(path);

    if (!file.is_open()) {
        std::cout << "Failed to open file " << path << std::endl;
        return;
    }

    file << script;
    file.close();
}
