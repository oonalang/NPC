#pragma once
#include <cmath>

namespace PhantomTrigger {


inline float VisibleTraceDistance(float nearPlane, float farPlane) noexcept {
    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) ||
        nearPlane < 0.f || farPlane <= nearPlane) return 0.f;
    const float distance = farPlane - nearPlane;
    return std::isfinite(distance) && distance > 0.f ? distance : 0.f;
}

// A ray that keeps restarting past bullet-passthrough surfaces (brush, glass,
// fences) re-issues the whole trace each time. Two restarts is the budget; the
// depth limit is measured from the *first* obstruction along the ray, not from
// the ray origin, so a target 40 m away behind cover still costs almost no
// depth and stays reachable while a deep run of foliage stops restarting.
inline constexpr int MaxPassthroughRestarts = 2;
inline constexpr float MaxPassthroughDepth = 6.0f;
inline constexpr float PassthroughStep = 0.12f;

inline bool WithinPassthroughDepth(float surfaceDistance,
                                   float firstSurfaceDistance) noexcept {
    if (!std::isfinite(surfaceDistance) || !std::isfinite(firstSurfaceDistance))
        return false;
    if (firstSurfaceDistance < 0.f || surfaceDistance < firstSurfaceDistance)
        return false;
    return surfaceDistance + PassthroughStep - firstSurfaceDistance <= MaxPassthroughDepth;
}
}
