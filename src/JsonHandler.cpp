//
// Created by victor on 28/05/25.
//

#include "JsonHandler.h"

#include <fstream>
#include <iostream>


void to_json(json &j, const Group &group) {
    j = json{
        {"name", group.name},
        {"lights", group.lights}
    };
}

void from_json(const json &j, Group &group) {
    j.at("name").get_to(group.name);
    j.at("lights").get_to(group.lights);
}

void to_json(json &j, const GradientKind &kind) {
    switch (kind) {
        case GradientKind::linear:
            j = "linear";
            break;
        case GradientKind::ease_in:
            j = "ease_in";
            break;
        case GradientKind::ease_out:
            j = "ease_out";
            break;
        case GradientKind::ease_in_out:
            j = "ease_in_out";
            break;
        default: j = "linear";
    }
}

void from_json(const json &j, GradientKind &kind) {
    auto value = j.get<std::string>();
    if (value == "linear") {
        kind = GradientKind::linear;
    } else if (value == "ease_in") {
        kind = GradientKind::ease_in;
    } else if (value == "ease_out") {
        kind = GradientKind::ease_out;
    } else if (value == "ease_in_out") {
        kind = GradientKind::ease_in_out;
    } else {
        throw std::runtime_error("Invalid gradient kind: " + value);
    }
}

void to_json(json &j, const ToggleInfo &info) {
    j = json{
        {"is_on", info.is_on},
        {"color", info.color}
    };
}

void from_json(const json &j, ToggleInfo &info) {
    j.at("is_on").get_to(info.is_on);
    j.at("color").get_to(info.color);
}

void to_json(json &j, const GradientInfo &info) {
    j = json{
        {"start_color", info.start_color},
        {"end_color", info.end_color},
        {"kind", info.kind},
        // Don't serialize start sample, because the info is given by the parent keyframe
        {"duration", info.duration}
    };
}

void from_json(const json &j, GradientInfo &info) {
    j.at("start_color").get_to(info.start_color);
    j.at("end_color").get_to(info.end_color);
    j.at("kind").get_to(info.kind);
    j.at("duration").get_to(info.duration);
}

void to_json(json &j, const AnimationKind &kind) {
    switch (kind) {
        case AnimationKind::gradient:
            j = "gradient";
            break;
        case AnimationKind::toggle:
            j = "toggle";
            break;
        default: j = "gradient";
    }
}

void from_json(const json &j, AnimationKind &kind) {
    auto value = j.get<std::string>();
    if (value == "gradient") {
        kind = AnimationKind::gradient;
    } else if (value == "toggle") {
        kind = AnimationKind::toggle;
    } else {
        throw std::runtime_error("Invalid animation kind: " + value);
    }
}

void to_json(json &j, const AnimationDesc &desc) {
    switch (desc.kind) {
        case AnimationKind::gradient:
            j = json{
                {"kind", "gradient"},
                {"gradient", desc.gradient}
            };
            break;
        case AnimationKind::toggle:
            j = json{
                {"kind", "toggle"},
                {"toggle", desc.toggle}
            };
            break;
    }
}

void from_json(const json &j, AnimationDesc &desc) {
    j.at("kind").get_to(desc.kind);
    switch (desc.kind) {
        case AnimationKind::gradient:
            j.at("gradient").get_to(desc.gradient);
            desc.toggle = ToggleInfo{false, Color{0, 0, 0, 255}};
            break;
        case AnimationKind::toggle:
            j.at("toggle").get_to(desc.toggle);
            desc.gradient = GradientInfo{Color{0, 0, 0, 255}, Color{0, 0, 0, 255}, GradientKind::linear, 0};
            break;
    }
}

void to_json(json &j, const Command &command) {
    j = json{
        {"animation", command.animation},
        {"trigger_sample", command.trigger_sample},
        {"group_id", command.group_id}
    };
}

void from_json(const json &j, Command &command) {
    j.at("animation").get_to(command.animation);
    j.at("trigger_sample").get_to(command.trigger_sample);
    j.at("group_id").get_to(command.group_id);
}

void to_json(json &j, const Color &c) {
    j = json{
        {"r", c.r},
        {"g", c.g},
        {"b", c.b},
        {"a", c.a}
    };
}

void from_json(const json &j, Color &c) {
    j.at("r").get_to(c.r);
    j.at("g").get_to(c.g);
    j.at("b").get_to(c.b);
    j.at("a").get_to(c.a);
}

void to_json(json &j, const JsonKeyframes &k) {
    j = json{
        {"trigger_sample", k.trigger_sample},
        {"commands", k.commands}
    };
}

void from_json(const json &j, JsonKeyframes &k) {
    j.at("trigger_sample").get_to(k.trigger_sample);
    j.at("commands").get_to(k.commands);
}


void to_json(json &j, const ProjectData &p) {
    std::vector<JsonKeyframes> json_keyframes;

    for (auto & keyframe: p.keyframes) {
        JsonKeyframes json_keyframe;
        json_keyframe.trigger_sample = keyframe.trigger_sample;
        json_keyframe.commands = p.keyframe_uuid_to_commands.at(keyframe.uuid);
        json_keyframes.push_back(json_keyframe);
    }

    j = json{
        {"light_count", p.light_count},
        {"groups", p.groups},
        {"keyframes", json_keyframes},
        {"max_uuid", p.max_uuid}
    };
}

void from_json(const json &j, ProjectData &p) {
    j.at("light_count").get_to(p.light_count);
    j.at("groups").get_to(p.groups);
    j.at("max_uuid").get_to(p.max_uuid);

    std::vector<JsonKeyframes> json_keyframes;
    j.at("keyframes").get_to(json_keyframes);

    p.keyframes.clear();
    p.keyframe_uuid_to_commands.clear();

    int64_t uuid = 0;
    for (auto & json_keyframe: json_keyframes) {
        Keyframe keyframe;
        keyframe.is_locked = true;
        keyframe.trigger_sample = json_keyframe.trigger_sample;
        keyframe.uuid = uuid++;
        p.keyframes.push_back(keyframe);

        p.keyframe_uuid_to_commands[keyframe.uuid] = json_keyframe.commands;
    }
}

ProjectData load(const std::string &path) {
    std::ifstream file(path);

    ProjectData p;
    json j;

    if (!file.is_open()) {
        std::cout << "Failed to open file " << path << std::endl;
        p = json::object();
    }

    try {
        file >> j;
        p = j;
    } catch (const json::parse_error& e) {
        std::cout << "Failed to parse json file " << path << std::endl;
        p = json::object();
    }

    file.close();
    return p;
}

void save(const std::string &path, const ProjectData &data) {
    std::ofstream file(path);

    if (!file.is_open()) {
        std::cout << "Failed to open file " << path << std::endl;
        return;
    }

    json j = data;
    file << j.dump(4);
    file.close();
}
