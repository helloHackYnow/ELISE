//
// Created by victor on 19/06/25.
//

#ifndef ACTIONS_H
#define ACTIONS_H

#include "ActionManager.h"

namespace EActions {

    enum Kind {
        spawn_keyframe,
        create_keyframe,
        delete_keyframe,
        move_keyframes,
    };

    class SpawnKeyframe : public Action {
        int uuid = -1;
        Keyframe keyframe;
        std::vector<Command> commands;
    public:
        SpawnKeyframe(Keyframe keyframe, const std::vector<Command>& commands) : keyframe(keyframe), commands(commands) {kind=spawn_keyframe;}
        bool Execute(AppState &state) override;
        bool Undo(AppState &state) override;
        bool IsMergeable(const Action *other) override {return false;}
        bool Merge(std::unique_ptr<Action> action) override {return false;}
        std::string GetDescription() const override {return "SpawnKeyframe";}
    };

    class CreateKeyframe : public Action {
        int64_t trigger_sample;
        int64_t uuid = -1;
    public:
        explicit CreateKeyframe(int64_t trigger_sample) : trigger_sample(trigger_sample) {kind = create_keyframe;}
        bool Execute(AppState &state) override;
        bool Undo(AppState &state) override;
        bool IsMergeable(const Action *other) override;
        bool Merge(std::unique_ptr<Action> action) override {return false;};
        std::string GetDescription() const override;
    };

    class DeleteKeyframes : public Action {
        std::set<int64_t> uuids;
        std::vector<std::pair<Keyframe, std::vector<Command>>> keyframes;
        std::vector<Command> commands = {};
    public:
        DeleteKeyframes(const std::set<int64_t>& uuids): uuids(uuids) {kind = delete_keyframe;};
        bool Execute(AppState &state) override;
        bool Undo(AppState &state) override;
        bool IsMergeable(const Action *other) override {return false;};
        bool Merge(std::unique_ptr<Action> action) override {return false;};
        std::string GetDescription() const override;
    };

    class MoveKeyframes : public Action {
        int sample_delta;
        std::set<int64_t> keyframes_uuid;
    public:
        MoveKeyframes(int sample_delta, const std::set<int64_t>& keyframes_uuid);
        bool Execute(AppState &state) override;
        bool Undo(AppState &state) override;
        bool IsMergeable(const Action *other) override;
        bool Merge(std::unique_ptr<Action> action) override;
        std::string GetDescription() const override;
        const std::set<int64_t>& GetKeyframes() const;
        int64_t GetSampleDelta() const;
    };
}



#endif //ACTIONS_H
