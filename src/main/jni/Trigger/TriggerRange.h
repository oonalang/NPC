#pragma once
#include <algorithm>
#include <cmath>

namespace PhantomTrigger {


inline float VisibleTraceDistance(float nearPlane, float farPlane) noexcept {
    if (!std::isfinite(nearPlane) || !std::isfinite(farPlane) ||
        nearPlane < 0.f || farPlane <= nearPlane) return 0.f;
    const float distance = farPlane - nearPlane;
    return std::isfinite(distance) && distance > 0.f ? distance : 0.f;
}

inline constexpr float MinimumBattleRoyaleTraceLimit = 500.f;
inline constexpr float BattleRoyaleCameraAllowance = 32.f;

inline float BattleRoyaleTraceDistance(float visibleDistance, float weaponRange) noexcept {
    if (!std::isfinite(weaponRange) || weaponRange <= 0.f) return visibleDistance;
    const float limit = std::max(MinimumBattleRoyaleTraceLimit, weaponRange + BattleRoyaleCameraAllowance);
    return std::min(visibleDistance, limit);
}
}
