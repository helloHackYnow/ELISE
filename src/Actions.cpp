//
// Created by victor on 19/06/25.
//

#include "Actions.h"

#include <sys/stat.h>

bool EActions::SpawnKeyframe::Execute(AppState &state) {
    if (uuid < 0) uuid = state.create_keyframe(keyframe, commands);
    else state.create_keyframe(keyframe, commands, uuid);

    return true;
}

bool EActions::SpawnKeyframe::Undo(AppState &state) {
    state.delete_keyframes({uuid});

    state.selected_keyframes.clear();
    state.selected_command = -1;

    return true;
}

bool EActions::CreateKeyframe::Execute(AppState &state) {
    if (uuid < 0) uuid = state.create_keyframe(trigger_sample);
    else state.create_keyframe(trigger_sample, uuid);

    return true;
}

bool EActions::CreateKeyframe::Undo(AppState &state) {
    state.delete_keyframes({uuid});

    state.selected_keyframes.clear();
    state.selected_command = -1;

    return true;
}

bool EActions::CreateKeyframe::IsMergeable(const Action *other) {
    return false;
}

std::string EActions::CreateKeyframe::GetDescription() const {
    return "Create a keyframe";
}

bool EActions::DeleteKeyframes::Execute(AppState &state) {
    keyframes.clear();

    for (int64_t uuid: uuids) {
        auto& keyframe = state.keyframes.at(state.keyframe_uuid_to_index.at(uuid));
        keyframes.emplace_back(keyframe, state.keyframe_uuid_to_commands.at(uuid));
    }

    state.delete_keyframes(uuids);
    state.build_keyframe_uuid_to_index_map();
    state.selected_keyframes.clear();

    return true;
}

bool EActions::DeleteKeyframes::Undo(AppState &state) {

    for (auto & pair: keyframes) {
        state.create_keyframe(pair.first, pair.second, pair.first.uuid);
    }

    state.build_keyframe_uuid_to_index_map();
    return true;
}

std::string EActions::DeleteKeyframes::GetDescription() const {
    return "Delete a keyframe";
}

EActions::MoveKeyframes::MoveKeyframes(int sample_delta, const std::set<int64_t>& keyframes_uuid):
    keyframes_uuid(keyframes_uuid),
    sample_delta(sample_delta) {

    timestamp = std::chrono::system_clock::now();
    kind = move_keyframes;
}

bool EActions::MoveKeyframes::Execute(AppState &state) {
    for (int uuid: keyframes_uuid) {
        assert(state.keyframe_uuid_to_index.contains(uuid));
        auto& keyframe = state.keyframes.at(state.keyframe_uuid_to_index.at(uuid));
        keyframe.trigger_sample += sample_delta;

        for (auto & command: state.keyframe_uuid_to_commands.at(uuid)) {
            retimeCommand(command, keyframe.trigger_sample);
        }
    }

    state.order_keyframes();

    return true;
}

bool EActions::MoveKeyframes::Undo(AppState &state) {
    for (int uuid: keyframes_uuid) {
        assert(state.keyframe_uuid_to_index.contains(uuid));
        auto& keyframe = state.keyframes.at(state.keyframe_uuid_to_index.at(uuid));
        keyframe.trigger_sample -= sample_delta;

        for (auto & command: state.keyframe_uuid_to_commands.at(uuid)) {
            retimeCommand(command, keyframe.trigger_sample);
        }
    }

    state.order_keyframes();

    return true;
}

bool EActions::MoveKeyframes::IsMergeable(const Action *other) {
    if (other->kind != move_keyframes) {
        return false;
    }

    auto other_ = (const MoveKeyframes*)(other);

    return other_->keyframes_uuid == keyframes_uuid;
}

bool EActions::MoveKeyframes::Merge(std::unique_ptr<Action> action) {
    auto other = (const MoveKeyframes*)(action.get());

    sample_delta += other->sample_delta;
    return true;
}

std::string EActions::MoveKeyframes::GetDescription() const {
    return "Move keyframes";
}

const std::set<int64_t> & EActions::MoveKeyframes::GetKeyframes() const {
    return keyframes_uuid;
}

int64_t EActions::MoveKeyframes::GetSampleDelta() const {
    return sample_delta;
}
