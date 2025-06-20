//
// Created by victor on 19/06/25.
//

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>
#include "LightManager.h"
#include "AppState.h"

class Action {
public:
    int kind = 0;
    bool mergeable = false;
    std::chrono::system_clock::time_point timestamp;

    virtual ~Action() = default;
    virtual bool Execute(AppState& state) = 0;
    virtual bool Undo(AppState& state) = 0;

    virtual bool IsMergeable(const Action* other) = 0;
    virtual bool Merge(std::unique_ptr<Action> action) = 0;

    virtual std::string GetDescription() const = 0;

    // Optional: for debugging/logging
    virtual bool IsReversible() const { return true; }
};

class Composite : public Action {
    std::vector<std::unique_ptr<Action>> actions = {};
public:
    Composite() = default;

    bool AddAction(std::unique_ptr<Action> action);

    bool Execute(AppState& state) override;
    bool Undo(AppState& state) override;
    bool IsMergeable(const Action* other) override {return false;}
    bool Merge(std::unique_ptr<Action> action) override {return false;};
    std::string GetDescription() const override { return "Composite"; }
};

class ActionManager {
public:
    ActionManager() = default;
    ~ActionManager() = default;

    bool execute(AppState& state, std::unique_ptr<Action> action);
    bool undo_last(AppState& state);
    bool redo_last(AppState& state);

private:
    std::chrono::duration<double> max_delta = std::chrono::milliseconds(250);
    std::vector<std::unique_ptr<Action>> undo_stack;
    std::vector<std::unique_ptr<Action>> redo_stack;
};



#endif //ACTIONMANAGER_H
