#pragma once
#include <algorithm>
#include <cmath>

namespace startup_ui {
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
