//
// Created by victor on 27/05/25.
//

#include "LightManager.h"

#include <algorithm>
#include <chrono>

ImVec4 get_vec(const Color &col) {
    return ImVec4(col.r / 255.f, col.g / 255.f, col.b / 255.f, col.a / 255.f);
}

Color interpolate_linear(Color a, Color b, float t) {
    return {
        int(m_lerp(a.r, b.r, t)),
        int(m_lerp(a.g, b.g, t)),
        int(m_lerp(a.b, b.b, t)),
        int(m_lerp(a.a, b.a, t))
    };
}

Color computeGradientColor(const GradientInfo &gradient, int64_t sample) {
    double t = std::clamp((sample - gradient.start_sample) / (double) gradient.duration, 0.0, 1.0);

    switch (gradient.kind) {
        case GradientKind::linear:
            return interpolate_linear(gradient.start_color, gradient.end_color, t);
        case GradientKind::ease_in:
            return interpolate_linear(gradient.start_color, gradient.end_color, ease_in(t));
        case GradientKind::ease_out:
            return interpolate_linear(gradient.start_color, gradient.end_color, ease_out(t));
        case GradientKind::ease_in_out:
            return interpolate_linear(gradient.start_color, gradient.end_color, ease_in_out(t));
        default:
            return interpolate_linear(gradient.start_color, gradient.end_color, t);
    }
}

Color computeToggleColor(const ToggleInfo &toggle, int64_t sample) {
    return toggle.is_on ? toggle.color : Color{0, 0, 0, 255};
}

Color computeBlinkColor(const BlinkInfo &blink, int64_t sample) {
    if (blink.period == 0) return blink.off_color;
    auto elapsed = std::max(int64_t(0), sample - blink.start_sample) % blink.period;
    return elapsed < blink.period / 2 ? blink.on_color : blink.off_color;

}

void retimeCommand(Command &command, int64_t new_sample) {
    command.trigger_sample = new_sample;

    switch (command.animation.kind) {
        case AnimationKind::gradient :
            command.animation.gradient.start_sample = new_sample;
            break;

        case AnimationKind::blink :
            command.animation.blink.start_sample = new_sample;
            break;

        default :
            break;
    }
}

Color computeAnimationColor(const AnimationDesc &animation, int64_t sample) {
    switch (animation.kind) {
        case AnimationKind::toggle : {
            return computeToggleColor(animation.toggle, sample);
        }

        case AnimationKind::gradient : {
            return computeGradientColor(animation.gradient, sample);
        }

        case AnimationKind::blink : {
            return computeBlinkColor(animation.blink, sample);
        }
        default:
            return Color{0, 0, 0, 0};
    }
}

LightManager::LightManager() {

}

int LightManager::addLight() {
    auto light_id = lights.size();
    lights.push_back(AnimationDesc{});
    light_states.push_back(Color{0, 0, 0, 255});
    return light_id;
}

int LightManager::new_group(std::vector<size_t> light_ids) {
    auto group_id = group_ids.size();
    group_ids.push_back(light_ids);
    return group_id;
}

void LightManager::update(int64_t current_sample) {
    updateAnimations(current_sample);
    updateLightStates(current_sample);
}

void LightManager::updateAnimations(int64_t current_sample) {

    while (!commands.empty() && commands.back().trigger_sample <= current_sample) {
        auto command = commands.back();
        const auto& animation = command.animation;

        for (int light_id : group_ids.at(command.group_id)) {
            lights.at(light_id) = command.animation;
        }

        commands.pop_back();
    }
}

void LightManager::updateLightStates(int64_t current_sample) {
    for (int i = 0; i < light_states.size(); i++) {
        light_states[i] = computeAnimationColor(lights.at(i), current_sample);
    }
}

void LightManager::setCommandStack(const std::vector<Command> &commands) {
    this->commands = commands;
}

void LightManager::reset() {
    for (auto& light : lights) {
        light = AnimationDesc{};
    }
    for (auto& light_state : light_states) {
        light_state = Color{0, 0, 0, 255};
    }

    commands.clear();
}


const std::vector<Color> & LightManager::getLightStates() {
    return light_states;
}
