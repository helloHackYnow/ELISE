//
// Created by victor on 19/06/25.
//

#include "AppState.h"

#include <algorithm>

int64_t AppState::create_keyframe(int64_t trigger_sample) {
    max_keyframe_uuid++;
    auto uuid = max_keyframe_uuid;
    keyframes.push_back(Keyframe{trigger_sample, uuid});

    // Create empty command
    keyframe_uuid_to_commands[uuid].push_back(Command{});

    order_keyframes();

    return uuid;
}

int64_t AppState::create_keyframe(int64_t trigger_sample, int64_t uuid) {
    max_keyframe_uuid++;
    keyframes.push_back(Keyframe{trigger_sample, uuid});

    // Create empty command
    keyframe_uuid_to_commands.erase(uuid);
    keyframe_uuid_to_commands[uuid].push_back(Command{});

    order_keyframes();

    return uuid;
}

int64_t AppState::create_keyframe(const Keyframe &keyframe) {
    auto uuid = create_keyframe(keyframe.trigger_sample);
    auto& keyframe_ = keyframes.at(keyframe_uuid_to_index.at(uuid));

    keyframe_.is_enabled = keyframe.is_enabled;
    keyframe_.is_locked = keyframe.is_locked;

    return uuid;
}

int64_t AppState::create_keyframe(const Keyframe &keyframe, int64_t uuid) {
    create_keyframe(keyframe.trigger_sample, uuid);
    auto& keyframe_ = keyframes.at(keyframe_uuid_to_index.at(uuid));

    keyframe_ = keyframe;
    keyframe_.uuid = uuid;
    return uuid;
}

int64_t AppState::create_keyframe(const Keyframe &keyframe, const std::vector<Command> &commands) {
    auto uuid = create_keyframe(keyframe);

    keyframe_uuid_to_commands[uuid] = commands;

    for (auto& command : keyframe_uuid_to_commands[uuid]) { retimeCommand(command, keyframe.trigger_sample); }

    return uuid;
}

int64_t AppState::create_keyframe(const Keyframe &keyframe, const std::vector<Command> &commands, int64_t uuid) {
    create_keyframe(keyframe, uuid);

    keyframe_uuid_to_commands[uuid] = commands;

    for (auto& command : keyframe_uuid_to_commands[uuid]) { retimeCommand(command, keyframe.trigger_sample); }

    return uuid;
}

void AppState::delete_keyframes(const std::set<int64_t> &uuids) {
    std::vector<int> to_delete;
    to_delete.reserve(uuids.size());

    for (int64_t selected_keyframe: uuids) {
        to_delete.push_back(keyframe_uuid_to_index[selected_keyframe]);
        keyframe_uuid_to_index.erase(selected_keyframe);
    }

    std::sort(to_delete.begin(), to_delete.end());
    std::reverse(to_delete.begin(), to_delete.end());

    for (int keyframe_index: to_delete) {
        keyframes.erase(keyframes.begin() + keyframe_index);
    }

    build_keyframe_uuid_to_index_map();
}


void AppState::order_keyframes() {
    std::sort(keyframes.begin(), keyframes.end(), compare);
    build_keyframe_uuid_to_index_map();
}

void AppState::build_keyframe_uuid_to_index_map() {
    keyframe_uuid_to_index.clear();

    for (size_t i = 0; i < keyframes.size(); ++i) {
        keyframe_uuid_to_index[keyframes[i].uuid] = i;
    }
}

void AppState::update_last_selected() {
    if (selected_keyframes.size() == 1) {
        last_selected_uuid = *selected_keyframes.begin();
        last_selected_content = get_keyframe_content(last_selected_uuid);
    } else {
        last_selected_uuid = -1;
    }
}

bool AppState::is_selected_keyframe_edited() {
    if (selected_keyframes.size() != 1) {
        return false;
    }

    auto uuid = *selected_keyframes.begin();
    if (last_selected_uuid != uuid) {
        return false;
    }

    auto new_content = get_keyframe_content(uuid);
    return new_content != last_selected_content;
}

Keyframe & AppState::get_keyframe(int64_t uuid) {
    return keyframes.at(keyframe_uuid_to_index.at(uuid));
}

KeyframeContent AppState::get_keyframe_content(int64_t uuid) {
    auto& keyframe = get_keyframe(uuid);
    return KeyframeContent{keyframe.is_locked, keyframe.is_enabled, keyframe_uuid_to_commands.at(uuid)};
}
