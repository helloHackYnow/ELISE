//
// Created by victor on 19/06/25.
//

#ifndef APPSTATE_H
#define APPSTATE_H

#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include "LightManager.h"


class AppState {
public:
    std::vector<Keyframe> keyframes = {};
    std::map<int64_t, int> keyframe_uuid_to_index = {};
    std::set<int64_t> selected_keyframes = {};
    int64_t max_keyframe_uuid = 0;

    std::unordered_map<int64_t, std::vector<Command>> keyframe_uuid_to_commands = {};
    int selected_command = -1;

    // Return the keyframe uuid
    int64_t create_keyframe(int64_t trigger_sample);
    int64_t create_keyframe(int64_t trigger_sample, int64_t uuid);
    int64_t create_keyframe(const Keyframe& keyframe);
    int64_t create_keyframe(const Keyframe& keyframe, int64_t uuid);
    int64_t create_keyframe(const Keyframe& keyframe, const std::vector<Command>& commands);
    int64_t create_keyframe(const Keyframe& keyframe, const std::vector<Command>& commands, int64_t uuid);

    void delete_keyframes(const std::set<int64_t>& uuid);

    void order_keyframes();
    void build_keyframe_uuid_to_index_map();
};



#endif //APPSTATE_H
