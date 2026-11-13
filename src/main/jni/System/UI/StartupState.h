#pragma once
#include <algorithm>
#include <cmath>

namespace startup_ui {
inline constexpr const char* loadingTips[]{
    "triggerbot auto shoots for you when your crosshair is aiming at the enemy.",
    "One of the best mods out there.",
    "My mod aims for meeting user requests."
};
struct LoadingTip { int index; float opacity; };
inline LoadingTip TipAt(double elapsed) {
    const double safe=std::isfinite(elapsed)?std::max(0.0,elapsed):0.0;
    const double cycle=std::fmod(safe,7.5);
    const int index=static_cast<int>(cycle/2.5);
    const double phase=std::fmod(cycle,2.5);
    const float incoming=safe<2.5?1.f:static_cast<float>(std::min(1.0,phase/.22));
    const float outgoing=static_cast<float>(std::min(1.0,(2.5-phase)/.22));
    return {index,std::clamp(std::min(incoming,outgoing),0.f,1.f)};
}
struct LoadingTimeline {
    double started = 0.0, duration = 7.0;
    bool active = false;
    void Start(double now, double seconds) {
        started = std::isfinite(now) ? now : 0.0;
        duration = std::isfinite(seconds) ? std::max(5.0, seconds) : 7.0;
        active = true;
    }
    float Progress(double now) const {
        if (!std::isfinite(now)) return 0.f;
        return static_cast<float>(std::clamp((now - started) / duration, 0.0, 1.0));
    }
    bool Finish(double now) {
        if (!active || Progress(now) < 1.f) return false;
        active = false;
        return true;
    }
};
}
