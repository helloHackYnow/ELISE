//
// Created by victor on 19/06/25.
//

#include "ActionManager.h"

bool ActionManager::execute(AppState &state, std::unique_ptr<Action> action) {
    if (!action->Execute(state)) {
        return false;
    }

    redo_stack.clear();

    if (!undo_stack.empty() && action->IsMergeable(undo_stack.back().get())) {
        if (action->timestamp - undo_stack.back()->timestamp < max_delta) {
            auto back = std::move(undo_stack.back());
            undo_stack.pop_back();

            action->Merge(std::move(back));
        }
    }

    undo_stack.push_back(std::move(action));

    return true;
}

bool ActionManager::undo_last(AppState &state) {
    if (undo_stack.empty()) {
        return false;
    }

    auto action = std::move(undo_stack.back());
    undo_stack.pop_back();

    if (!action->Undo(state)) {
        undo_stack.push_back(std::move(action));
        return false;
    }

    redo_stack.push_back(std::move(action));
    return true;
}

bool ActionManager::redo_last(AppState &state) {
    if (redo_stack.empty()) {
        return false;
    }

    auto action = std::move(redo_stack.back());
    redo_stack.pop_back();

    if (!action->Execute(state)) {
        redo_stack.push_back(std::move(action));
        return false;
    }

    undo_stack.push_back(std::move(action));
    return true;
}

