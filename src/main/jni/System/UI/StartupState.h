#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>

namespace startup_ui {
inline double Now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
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
struct LoginTransitionFrame {
    float morph=0.f, rotation=0.f, fill=0.f, check=0.f, opacity=1.f;
    bool ready=false;
};
struct LoginTransition {
    static constexpr double morphSeconds=.24, spinSeconds=2., fillSeconds=.32;
    static constexpr double checkSeconds=.20, holdSeconds=.45, fadeSeconds=.22;
    double started=0., accepted=0.;
    bool active=false, succeeded=false;
    void Reset() { *this={}; }
    bool Start(double now) {
        if(active || !std::isfinite(now)) return false;
        started=now;
        accepted=0.;
        active=true;
        succeeded=false;
        return true;
    }
    void Resolve(double now,bool success) {
        if(!active || succeeded || !std::isfinite(now)) return;
        if(!success) { Reset(); return; }
        accepted=std::max(now,started+morphSeconds+spinSeconds);
        succeeded=true;
    }
    LoginTransitionFrame At(double now) const {
        LoginTransitionFrame frame;
        if(!active || !std::isfinite(now)) return frame;
        const double elapsed=std::max(0.,now-started);
        const auto unit=[](double value) { return static_cast<float>(std::clamp(value,0.,1.)); };
        frame.morph=unit(elapsed/morphSeconds);
        frame.rotation=static_cast<float>(std::fmod(std::max(0.,elapsed-morphSeconds),1.)*6.283185307179586);
        if(!succeeded || now<accepted) return frame;
        const double reveal=now-accepted;
        frame.fill=unit(reveal/fillSeconds);
        frame.check=unit((reveal-fillSeconds)/checkSeconds);
        frame.opacity=1.f-unit((reveal-fillSeconds-checkSeconds-holdSeconds)/fadeSeconds);
        frame.ready=reveal>=fillSeconds+checkSeconds+holdSeconds+fadeSeconds;
        return frame;
    }
};
}
