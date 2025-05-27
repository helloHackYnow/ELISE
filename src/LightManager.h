//
// Created by victor on 27/05/25.
//

#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H
#include <vector>
#include "InterpolationUtils.h"

struct Color {
    int r;
    int g;
    int b;
    int a;
};

Color interpolate_linear(Color a, Color b, float t);

class Animation {
public:
    virtual ~Animation() = 0;

private:
    virtual Color get_color_at_sample(int sample);
};

enum class GradientKind {
    linear,
    ease_in,
    ease_out,
    ease_in_out,
};

inline const char* GradientKind_str [] {
    "linear",
    "ease in",
    "ease out",
    "ease inout"
};

inline GradientKind GradientKind_from_int [] {
    GradientKind::linear,
    GradientKind::ease_in,
    GradientKind::ease_out,
    GradientKind::ease_in_out
};

inline const char* GradientKind_to_str(const GradientKind& kind) {
    switch (kind) {
        case GradientKind::linear:
            return "linear";
        case GradientKind::ease_in:
            return "ease in";
        case GradientKind::ease_out:
            return "ease out";
        case GradientKind::ease_in_out:
            return "ease inout";
    }

    return "";
}

enum class AnimationKind {
    gradient,
    toggle,
};

inline const char* AnimationKind_str [] {
    "gradient",
    "toggle"
};

inline AnimationKind AnimationKind_from_int [] {
    AnimationKind::gradient,
    AnimationKind::toggle
};

inline const char* AnimationKind_to_str(const AnimationKind& kind) {
    switch (kind) {
        case AnimationKind::gradient :
             return "gradient";
        case AnimationKind::toggle :
            return "toggle";
    }
    return "";
}


struct GradientInfo {
    Color start_color;
    Color end_color;
    GradientKind kind;

    int start_sample;
    int duration; // In sample count
};

Color computeGradientColor(const GradientInfo& gradient, int sample);

struct ToggleInfo {
    bool is_on;
    Color color;
};

Color computeToggleColor(const ToggleInfo& toggle, int sample);

struct AnimationDesc {
    AnimationKind kind;

    union {
        GradientInfo gradient;
        ToggleInfo toggle;
    };
};

struct Command {
    AnimationDesc animation;
    int trigger_sample;
    int group_id;
};

void retimeCommand(Command& command, int current_sample);

Color computeAnimationColor(const AnimationDesc& animation, int sample);

class LightManager {
public:
    LightManager();

    int addLight();

    int new_group(std::vector<size_t> light_ids);

    void update(int current_sample);

    void updateAnimations(int current_sample);

    void updateLightStates(int current_sample);

    // Must be ordered by trigger time. The last at [0], the first at [size-1]
    void setCommandStack(const std::vector<Command>& commands);

    void reset();

    const std::vector<Color>& getLightStates();

private:
    std::vector<AnimationDesc> lights;

    std::vector<Color> light_states;
    std::vector<std::vector<size_t>> group_ids;

    std::vector<Command> commands; // Must be ordered by trigger time. The last at [0], the first at [size-1]
};



#endif //LIGHTMANAGER_H
