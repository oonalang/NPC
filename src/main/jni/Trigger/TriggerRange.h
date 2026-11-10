#pragma once
#include <cmath>

namespace PhantomTrigger {


inline float VisibleTraceDistance(float nearPlane, float farPlane) noexcept {
    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) ||
        nearPlane < 0.f || farPlane <= nearPlane) return 0.f;
    const float distance = farPlane - nearPlane;
    return std::isfinite(distance) && distance > 0.f ? distance : 0.f;
}
}
