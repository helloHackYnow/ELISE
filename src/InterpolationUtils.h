//
// Created by victor on 27/05/25.
//

#ifndef INTERPOLATIONUTILS_H
#define INTERPOLATIONUTILS_H

#include <cmath>

inline float m_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Map linear [0, 1] to ease_in [0, 1]
inline float ease_in(float t) {
    return t * t * t;
}

inline float ease_out(float t) {
    return (1 - t) * (1 - t) * (1 - t);
}

inline float ease_in_out(float t) {
    if (t < 0.5) {
        return 0.5 * pow(2*t, 3);
    } else {
        float f = 2*(1-t);
        return 1 - (0.5 * pow(f, 3));
    }
}

#endif //INTERPOLATIONUTILS_H
