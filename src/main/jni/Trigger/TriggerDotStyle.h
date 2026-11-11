#pragma once

#include <cmath>

namespace TriggerDotStyle {

struct Appearance {
    float radius = 3.0f;
    float outline = 1.0f;
    float opacity = 1.0f;
    float color[3] = {1.0f, 1.0f, 1.0f};
};

inline Appearance appearance{};

inline float ClampFinite(float value, float lower, float upper, float fallback) noexcept {
    if (!std::isfinite(value)) return fallback;
    if (value < lower) return lower;
    if (value > upper) return upper;
    return value;
}

inline void Normalize(Appearance& value) noexcept {
    value.radius = ClampFinite(value.radius, 1.0f, 12.0f, 3.0f);
    value.outline = ClampFinite(value.outline, 0.0f, 5.0f, 1.0f);
    value.opacity = ClampFinite(value.opacity, 0.1f, 1.0f, 1.0f);
    for (float& channel : value.color)
        channel = ClampFinite(channel, 0.0f, 1.0f, 1.0f);
}

}
