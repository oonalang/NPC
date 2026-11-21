#pragma once
#include <algorithm>
#include <cmath>

namespace lite_startup {
inline constexpr double introFadeSeconds = .16;
inline constexpr double authenticatePulseSeconds = 1.04;
inline constexpr double minimumLoadingSeconds = .90;
inline constexpr double finishSeconds = .44;
inline constexpr double revealSeconds = .18;
inline constexpr double orbitSeconds = 1.65;
inline constexpr double breatheSeconds = 2.4;

inline float Ease(float value) {
    value = std::clamp(value,0.f,1.f);
    return value*value*(3.f-2.f*value);
}
struct Frame {
    double elapsed = 0;
    float finish = 0, reveal = 1;
    bool showMenu = false, interactive = false, reducedMotion = false, active = false;
};
class Motion {
    double start_ = 0, finishAt_ = 0, menuAt_ = 0, previous_ = 0;
    bool started_ = false, finishing_ = false, menu_ = false;
public:
    Frame Update(double now, bool authorized, bool prepared, bool reduced = false) {
        if (!authorized) { *this = {}; Frame frame; frame.reducedMotion = reduced; return frame; }
        if (!std::isfinite(now)) now = started_ ? previous_ : 0;
        if (started_ && now < previous_) {
            const auto shift = now-previous_;
            start_ += shift; finishAt_ += shift; menuAt_ += shift;
        }
        previous_ = now;
        if (!started_) { start_ = now; started_ = true; }
        Frame frame;
        frame.active = true;
        frame.elapsed = std::max(0.,now-start_);
        frame.reducedMotion = reduced;
        if (reduced) {
            frame.showMenu = frame.interactive = prepared;
            frame.finish = prepared ? 1.f : 0.f;
            return frame;
        }
        if (!prepared) { finishing_ = menu_ = false; return frame; }
        if (!finishing_ && frame.elapsed >= minimumLoadingSeconds) {
            finishing_ = true; finishAt_ = now;
        }
        if (finishing_) frame.finish = std::clamp(float((now-finishAt_)/finishSeconds),0.f,1.f);
        if (frame.finish >= 1.f) {
            if (!menu_) { menuAt_ = now; menu_ = true; }
            frame.showMenu = true;
            frame.reveal = Ease(float((now-menuAt_)/revealSeconds));
            frame.interactive = now-menuAt_ >= revealSeconds;
        }
        return frame;
    }
};
}
