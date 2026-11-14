#pragma once
#include "StartupState.h"
#include "../Audio/UiSounds.h"
#include <optional>

namespace startup_ui {
class SoundObserver {
    AuthStage previous_=AuthStage::Idle;
public:
    std::optional<ui_sounds::Cue> Update(const LoginTransitionFrame& frame) {
        if(frame.stage==previous_) return {};
        previous_=frame.stage;
        switch(frame.stage) {
            case AuthStage::Splash: return ui_sounds::Cue::Drop;
            case AuthStage::Check: return ui_sounds::Cue::Success;
            case AuthStage::Suck: return ui_sounds::Cue::Whoosh;
            case AuthStage::Reveal: return ui_sounds::Cue::Open;
            default: return {};
        }
    }
};
}
