//
// Created by victor on 28/05/25.
//

#ifndef JSONHANDLER_H
#define JSONHANDLER_H

#include "../libs/nlohmann/json.hpp"
#include "LightManager.h"

struct ProjectData {
    size_t light_count = 12;
    int sample_rate = 44100;
    std::vector<Group> groups;
    std::vector<Keyframe> keyframes;
    int64_t max_uuid;
    std::unordered_map<int64_t, std::vector<Command>> keyframe_uuid_to_commands;
};

struct JsonKeyframes {
    int64_t trigger_sample = 0;
    std::vector<Command> commands;
};

using json = nlohmann::json;

void to_json(json& j, const Group& group);
void from_json(const json& j, Group& group);

void to_json(json& j, const GradientKind& kind);
void from_json(const json& j, GradientKind& kind);

void to_json(json& j, const ToggleInfo& info);
void from_json(const json& j, ToggleInfo& info);

void to_json(json& j, const GradientInfo& info);
void from_json(const json& j, GradientInfo& info);

void to_json(json &j, const BlinkInfo &info);
void from_json(const json &j, BlinkInfo &info);


void to_json(json& j, const AnimationKind& kind);
void from_json(const json& j, AnimationKind& kind);

void to_json(json& j, const AnimationDesc& desc);
void from_json(const json& j, AnimationDesc& desc);

void to_json(json& j, const Command& command);
void from_json(const json& j, Command& command);

void to_json(json& j, const Color& c);
void from_json(const json& j, Color& c);

void to_json(json& j, const JsonKeyframes& k);
void from_json(const json& j, JsonKeyframes& k);

void to_json(json& j, const ProjectData& p);
void from_json(const json& j, ProjectData& p);

ProjectData load(const std::string& path);
void save(const std::string& path, const ProjectData& data);


#endif //JSONHANDLER_H
