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


struct KeyframeContent {
    bool is_locked;
    bool is_enabled;
    std::vector<Command> commands;

    bool operator==(const KeyframeContent &other) const {
        if (other.is_locked != is_locked) return false;

        if (other.is_enabled != is_enabled) return false;

        // Check the command vector
        if (commands.size() != other.commands.size()) return false;

        for (size_t i = 0; i < commands.size(); i++) {
            if (commands[i] != other.commands[i]) return false;
        }

        return true;
    }
};

class AppState {
public:
    std::vector<Keyframe> keyframes = {};
    std::map<int64_t, int> keyframe_uuid_to_index = {};
    std::set<int64_t> selected_keyframes = {};
    int64_t max_keyframe_uuid = 0;

    std::unordered_map<int64_t, std::vector<Command>> keyframe_uuid_to_commands = {};
    int selected_command = -1;

    KeyframeContent last_selected_content = {};
    int64_t last_selected_uuid = -1;

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
    void update_last_selected();
    bool is_selected_keyframe_edited();

    Keyframe& get_keyframe(int64_t uuid);
    KeyframeContent get_keyframe_content(int64_t uuid);
};



#endif //APPSTATE_H
