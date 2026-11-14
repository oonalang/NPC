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
    float label=1.f,morph=0.f,rotation=0.f;
    float droplet=0.f,fall=0.f,splash=0.f,wave=0.f,check=0.f;
    float opacity=1.f,suck=0.f,hole=0.f,flash=0.f,shock=0.f;
    bool granted=false,ready=false;
};
struct LoginTransition {
    static constexpr double labelSeconds=.15,morphSeconds=.30,spinSeconds=2.;
    static constexpr double dropSeconds=.25,hoverSeconds=.10,fallSeconds=.35;
    static constexpr double splashSeconds=.45,waveSeconds=.80,checkSeconds=.35;
    static constexpr double holdSeconds=.20,suckSeconds=.70,holeSeconds=.15;
    static constexpr double flashSeconds=.15,shockSeconds=1.80,pauseSeconds=.35;
    static constexpr double sequenceStart=labelSeconds+morphSeconds+spinSeconds;
    static constexpr double spinAt=labelSeconds+morphSeconds;
    static constexpr double fallAt=dropSeconds+hoverSeconds;
    static constexpr double splashAt=fallAt+fallSeconds;
    static constexpr double waveAt=splashAt+splashSeconds;
    static constexpr double checkAt=waveAt+waveSeconds;
    static constexpr double suckAt=checkAt+checkSeconds+holdSeconds;
    static constexpr double holeAt=suckAt+suckSeconds;
    static constexpr double flashAt=holeAt+holeSeconds;
    static constexpr double shockAt=flashAt+flashSeconds;
    static constexpr double pauseAt=shockAt+shockSeconds;
    static constexpr double readyAt=pauseAt+pauseSeconds;
    double started=0.,accepted=0.;
    bool active=false,succeeded=false;
    void Reset() { *this={}; }
    bool Start(double now) {
        if(active || !std::isfinite(now)) return false;
        started=now;accepted=0.;active=true;succeeded=false;
        return true;
    }
    void Resolve(double now,bool success) {
        if(!active || succeeded || !std::isfinite(now)) return;
        if(!success) { Reset(); return; }
        accepted=std::max(now,started+sequenceStart);
        succeeded=true;
    }
    LoginTransitionFrame At(double now) const {
        LoginTransitionFrame frame;
        if(!active || !std::isfinite(now)) return frame;
        const double elapsed=std::max(0.,now-started);
        const auto unit=[](double value) { return static_cast<float>(std::clamp(value,0.,1.)); };
        frame.label=1.f-unit(elapsed/labelSeconds);
        frame.morph=unit((elapsed-spinAt)/morphSeconds);
        const double spin=std::fmod(std::max(0.,elapsed-spinAt),spinSeconds)/spinSeconds;
        frame.rotation=static_cast<float>(spin*spin*(3.-2.*spin)*6.283185307179586);
        if(!succeeded || now<accepted) return frame;
        const double t=now-accepted;
        frame.granted=true;
        frame.rotation=0.f;
        frame.droplet=unit(t/dropSeconds);
        frame.fall=unit((t-fallAt)/fallSeconds);
        frame.splash=unit((t-splashAt)/splashSeconds);
        frame.wave=unit((t-waveAt)/waveSeconds);
        frame.check=unit((t-checkAt)/checkSeconds);
        frame.suck=unit((t-suckAt)/suckSeconds);
        frame.hole=unit((t-holeAt)/holeSeconds);
        frame.flash=unit((t-flashAt)/flashSeconds);
        frame.shock=unit((t-shockAt)/shockSeconds);
        frame.opacity=1.f-frame.suck;
        frame.ready=t>=readyAt;
        return frame;
    }
};
}
