#pragma once
#include "StartupState.h"
#include "../Audio/UiSounds.h"
#include <optional>

namespace startup_ui {
struct SoundAction {
    std::optional<ui_sounds::Cue> cue;
    bool stop=false;
    ui_sounds::Channel channel=ui_sounds::Channel::Startup;
};
inline void PlaySounds(const SoundAction& action) {
    if(action.stop) ui_sounds::Stop(ui_sounds::Channel::Startup);
    if(action.cue) ui_sounds::Play(*action.cue,action.channel);
}
class SoundObserver {
    AuthStage previous_=AuthStage::Idle;
    bool visible_=true;
public:
    SoundAction Update(const LoginTransitionFrame& frame,bool visible=true) {
        const auto previous=previous_;previous_=frame.stage;
        if(!visible) {const bool stop=visible_;visible_=false;return {{},stop};}
        if(!visible_) {visible_=true;return {};}
        if(frame.stage==previous) return {};
        using C=ui_sounds::Cue;
        switch(frame.stage) {
            case AuthStage::Idle: return {{},previous<AuthStage::Reveal};
            case AuthStage::LabelFade: return {C::AuthPress,true};
            case AuthStage::Collapse: return {C::ButtonMorph};
            case AuthStage::Spin: return previous==AuthStage::Idle?SoundAction{C::AuthPress,true}:SoundAction{};
            case AuthStage::Droplet: return {C::Droplet};
            case AuthStage::Fall: return {C::Fall};
            case AuthStage::Splash: return {C::Splash};
            case AuthStage::Wave: return {C::Wave};
            case AuthStage::Check: return {C::Success};
            case AuthStage::Suck: return {C::SuckIn};
            case AuthStage::Singularity: return {C::Singularity};
            case AuthStage::Flash: return {C::Explosion};
            case AuthStage::Calm: return {{},true};
            case AuthStage::Reveal: return {C::LoadingReveal};
            case AuthStage::Loading: return previous==AuthStage::Reveal?SoundAction{}:SoundAction{C::LoadingReveal};
            default: return {};
        }
    }
};
class LoadingSoundObserver {
    bool active_=false,visible_=true;
    int completed_=0;
public:
    SoundAction Update(bool active,float progress,bool visible=true) {
        if(!active) {const bool stop=active_;active_=false;completed_=0;visible_=visible;return {{},stop};}
        if(!std::isfinite(progress)) return {};
        const int completed=static_cast<int>(std::clamp(progress,0.f,1.f)*4.f);
        if(!visible) {const bool stop=visible_;visible_=false;active_=true;completed_=completed;return {{},stop};}
        if(!active_||!visible_||completed<completed_) {active_=visible_=true;completed_=completed;return {};}
        if(completed==completed_) return {};
        completed_=completed;
        return completed>=4?SoundAction{ui_sounds::Cue::LoadingDone,true,ui_sounds::Channel::UI}:SoundAction{ui_sounds::Cue::LoadingStep};
    }
};
}
