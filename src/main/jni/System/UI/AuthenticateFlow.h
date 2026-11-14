#pragma once
#include "StartupState.h"
#include "AuthenticateScene.h"

namespace startup_ui {
inline auth_scene::Frame SceneFrame(const LoginTransitionFrame& frame,ImVec2 center={.5f,.5f}) {
    auth_scene::Mode mode=auth_scene::Mode::Card;
    switch(frame.stage) {
        case AuthStage::Suck: mode=auth_scene::Mode::Suck; break;
        case AuthStage::Singularity: mode=auth_scene::Mode::Singularity; break;
        case AuthStage::Flash: mode=auth_scene::Mode::Flash; break;
        case AuthStage::Explosion: mode=auth_scene::Mode::Explosion; break;
        case AuthStage::Calm: mode=auth_scene::Mode::Empty; break;
        default: break;
    }
    return {mode,frame.sceneProgress,center};
}
}
