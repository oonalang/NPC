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
struct AuthTiming {
    static constexpr double labelFadeMs=150., buttonCollapseMs=300., spinMs=2000.;
    static constexpr double dropletMorphMs=250., dropletHoverMs=100., fallMs=350.;
    static constexpr double splashMs=450., waveMs=800., checkDrawMs=350., checkHoldMs=500.;
    static constexpr double suckMs=700., singularityMs=150., flashMs=150., explosionMs=1800.;
    static constexpr double calmMs=350., revealMs=450., settledReviewMs=750.;
    static constexpr double revealStaggerMs=55., revealGroupMs=230.;
    static constexpr double spinTurns=1.;
    static constexpr double labelEndMs=labelFadeMs;
    static constexpr double collapseEndMs=labelEndMs+buttonCollapseMs;
    static constexpr double spinEndMs=collapseEndMs+spinMs;
    static constexpr double morphEndMs=spinEndMs+dropletMorphMs;
    static constexpr double hoverEndMs=morphEndMs+dropletHoverMs;
    static constexpr double fallEndMs=hoverEndMs+fallMs;
    static constexpr double splashEndMs=fallEndMs+splashMs;
    static constexpr double waveEndMs=splashEndMs+waveMs;
    static constexpr double checkEndMs=waveEndMs+checkDrawMs;
    static constexpr double suckStartMs=checkEndMs+checkHoldMs;
    static constexpr double suckEndMs=suckStartMs+suckMs;
    static constexpr double singularityEndMs=suckEndMs+singularityMs;
    static constexpr double flashEndMs=singularityEndMs+flashMs;
    static constexpr double explosionEndMs=flashEndMs+explosionMs;
    static constexpr double revealStartMs=explosionEndMs+calmMs;
    static constexpr double revealEndMs=revealStartMs+revealMs;
    static constexpr double reviewEndMs=revealEndMs+settledReviewMs;
};
enum class ButtonStage { Idle,LabelFade,Collapse,Spin,Droplet,Hover,Fall,Splash,Wave,Check };
struct ButtonFrame {
    ButtonStage stage=ButtonStage::Idle;
    float labelOpacity=1.f, collapse=0.f, spinnerRotation=0.f;
    float dropletMorph=0.f, fall=0.f, splash=0.f, wave=0.f, check=0.f;
};
enum class AuthStage { Idle,LabelFade,Collapse,Spin,Droplet,Hover,Fall,Splash,Wave,Check,
                       Suck,Singularity,Flash,Explosion,Calm,Reveal,Loading };
inline float Unit(double value) { return static_cast<float>(std::clamp(value,0.,1.)); }
inline float Smooth(float value) { return value*value*(3.f-2.f*value); }
struct LoadingReveal {
    float progress=1.f;
    float Group(int index) const {
        return Smooth(Unit((progress*AuthTiming::revealMs-index*AuthTiming::revealStaggerMs)/AuthTiming::revealGroupMs));
    }
    float Scale() const { return .9f+.1f*Smooth(progress); }
};
struct LoginTransitionFrame {
    AuthStage stage=AuthStage::Idle;
    ButtonFrame button;
    float sceneProgress=0.f, loadingReveal=0.f;
    float morph=0.f, rotation=0.f, fill=0.f, check=0.f, opacity=1.f;
    double elapsedMs=0.;
    bool loginVisible=true, loadingVisible=false, inputBlocked=false, granted=false, ready=false;
};
struct LoginTransition {
    double started=0., accepted=0., delayMs=0.;
    bool active=false, succeeded=false, reducedMotion=false, checkPresented=false;
    void Reset() { *this={}; }
    bool Start(double now,bool reduced=false) {
        if(active || !std::isfinite(now)) return false;
        started=now; accepted=0.; delayMs=0.;
        active=true; succeeded=false; reducedMotion=reduced; checkPresented=false;
        return true;
    }
    void Resolve(double now,bool success) {
        if(!active || succeeded || !std::isfinite(now)) return;
        if(!success) { Reset(); return; }
        accepted=now;
        delayMs=std::max(0.,(now-started)*1000.-AuthTiming::spinEndMs);
        succeeded=true;
    }
    void Presented(const LoginTransitionFrame& frame) {
        if(reducedMotion && succeeded && frame.stage==AuthStage::Check) checkPresented=true;
    }
    LoginTransitionFrame At(double now) const {
        LoginTransitionFrame frame;
        if(!active || !std::isfinite(now)) return frame;
        frame.inputBlocked=true;
        double t=std::max(0.,(now-started)*1000.);
        if(reducedMotion) {
            frame.button.stage=ButtonStage::Spin;frame.button.labelOpacity=0.f;frame.button.collapse=1.f;
            frame.stage=AuthStage::Spin;
            if(!succeeded) return frame;
            frame.granted=true;
            frame.button.stage=ButtonStage::Check; frame.button.wave=1.f; frame.button.check=1.f;
            frame.stage=AuthStage::Check;
            if(checkPresented) {
                frame.stage=AuthStage::Loading; frame.loginVisible=false; frame.loadingVisible=true;
                frame.loadingReveal=1.f; frame.inputBlocked=false; frame.ready=true;
            }
            return frame;
        }
        const bool waiting=!succeeded && t>=AuthTiming::spinEndMs;
        t=waiting?AuthTiming::spinEndMs:std::max(0.,t-delayMs);
        frame.elapsedMs=t;
        const auto phase=[&](double start,double duration) { return Unit((t-start)/duration); };
        auto& b=frame.button;
        b.labelOpacity=1.f-phase(0.,AuthTiming::labelFadeMs);
        b.collapse=phase(AuthTiming::labelEndMs,AuthTiming::buttonCollapseMs);
        if(t<AuthTiming::labelEndMs) { frame.stage=AuthStage::LabelFade; b.stage=ButtonStage::LabelFade; }
        else if(t<AuthTiming::collapseEndMs) { frame.stage=AuthStage::Collapse; b.stage=ButtonStage::Collapse; }
        else if(t<AuthTiming::spinEndMs || waiting) {
            frame.stage=AuthStage::Spin; b.stage=ButtonStage::Spin;
            b.spinnerRotation=static_cast<float>(6.283185307179586*AuthTiming::spinTurns*Smooth(phase(AuthTiming::collapseEndMs,AuthTiming::spinMs)));
        } else if(t<AuthTiming::morphEndMs) { frame.stage=AuthStage::Droplet; b.stage=ButtonStage::Droplet; }
        else if(t<AuthTiming::hoverEndMs) { frame.stage=AuthStage::Hover; b.stage=ButtonStage::Hover; }
        else if(t<AuthTiming::fallEndMs) { frame.stage=AuthStage::Fall; b.stage=ButtonStage::Fall; }
        else if(t<AuthTiming::splashEndMs) { frame.stage=AuthStage::Splash; b.stage=ButtonStage::Splash; }
        else if(t<AuthTiming::waveEndMs) { frame.stage=AuthStage::Wave; b.stage=ButtonStage::Wave; }
        else if(t<AuthTiming::suckStartMs) { frame.stage=AuthStage::Check; b.stage=ButtonStage::Check; }
        else if(t<AuthTiming::suckEndMs) {
            frame.stage=AuthStage::Suck; b.stage=ButtonStage::Check;
            frame.sceneProgress=phase(AuthTiming::suckStartMs,AuthTiming::suckMs);
        } else {
            frame.loginVisible=false;
            b.stage=ButtonStage::Check;
            if(t<AuthTiming::singularityEndMs) { frame.stage=AuthStage::Singularity; frame.sceneProgress=phase(AuthTiming::suckEndMs,AuthTiming::singularityMs); }
            else if(t<AuthTiming::flashEndMs) { frame.stage=AuthStage::Flash; frame.sceneProgress=phase(AuthTiming::singularityEndMs,AuthTiming::flashMs); }
            else if(t<AuthTiming::explosionEndMs) { frame.stage=AuthStage::Explosion; frame.sceneProgress=phase(AuthTiming::flashEndMs,AuthTiming::explosionMs); }
            else if(t<AuthTiming::revealStartMs) frame.stage=AuthStage::Calm;
            else {
                frame.loadingVisible=true;
                frame.loadingReveal=phase(AuthTiming::revealStartMs,AuthTiming::revealMs);
                frame.stage=t<AuthTiming::revealEndMs?AuthStage::Reveal:AuthStage::Loading;
                frame.ready=t>=AuthTiming::revealEndMs;
                frame.inputBlocked=!frame.ready;
            }
        }
        b.dropletMorph=phase(AuthTiming::spinEndMs,AuthTiming::dropletMorphMs);
        b.fall=phase(AuthTiming::hoverEndMs,AuthTiming::fallMs);
        b.splash=phase(AuthTiming::fallEndMs,AuthTiming::splashMs);
        b.wave=phase(AuthTiming::splashEndMs,AuthTiming::waveMs);
        b.check=phase(AuthTiming::waveEndMs,AuthTiming::checkDrawMs);
        frame.granted=succeeded && t>=AuthTiming::waveEndMs;
        frame.morph=b.collapse; frame.rotation=b.spinnerRotation; frame.fill=b.wave; frame.check=b.check;
        return frame;
    }
};
}
