//
// Created by victor on 27/05/25.
//

#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H
#include <vector>

struct Color {
    int r;
    int g;
    int b;
    int a;
};

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

struct GradientInfo {
    Color start;
    Color end;
    GradientKind kind;

    int start_sample;
    int duration; // In sample count
};

class GradientAnimation : public Animation {
    GradientInfo info{};

    Color get_color_at_sample(int sample) override;
};

class ToogleAnimation : public Animation {
    bool is_on = false;
    Color color{};

    Color get_color_at_sample(int sample) override;
};


class LightManager {
public:
    LightManager();
    ~LightManager();

    std::vector<Color> getLightStates();
};



#endif //LIGHTMANAGER_H
