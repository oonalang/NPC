#pragma once
#include "../../ImGui/custom_widgets.hpp"
#include "../Audio/UiSounds.h"
#include "../Hooks/FrameRate.h"

namespace settings_controls {
inline void Draw() {
    custom::Checkbox("Clear Display",&Config.ExtraMenu.ClearDisplay);
    Config.Aim.FpsLevel1=frame_rate::Fps(Config.Aim.FpsLevel1,60);
    Config.Aim.FpsLevelUltra1=frame_rate::Fps(Config.Aim.FpsLevelUltra1,240,60);
    custom::Checkbox("Custom FPS",&Config.Aim.FpsLevel);
    if(Config.Aim.FpsLevel) custom::SliderFloat("Frame rate",&Config.Aim.FpsLevel1,30.f,360.f,"%.0f FPS");
    custom::Checkbox("Ultra FPS override",&Config.Aim.FpsLevelUltra);
    if(Config.Aim.FpsLevelUltra) custom::SliderFloat("Ultra frame rate",&Config.Aim.FpsLevelUltra1,60.f,360.f,"%.0f FPS");
    const auto options=frame_rate::Configured(Config.Aim.FpsLevel,Config.Aim.FpsLevel1,Config.Aim.FpsLevelUltra,Config.Aim.FpsLevelUltra1);
    frame_rate::Publish(options);
#ifdef __ANDROID__
    if(options.Active()&&!frame_rate::available.load()) ImGui::TextDisabled("Waiting for compatible FPS hooks");
#endif
    ImGui::Separator();
    bool effects=ui_sounds::Enabled();
    if(custom::Checkbox("Creamy UI sounds",&effects)) ui_sounds::SetEnabled(effects);
    float volume=ui_sounds::Gain()*100.f;
    if(custom::SliderFloat("Effects volume",&volume,0.f,100.f,"%.0f%%")) ui_sounds::SetGain(volume*.01f);
    ImGui::Separator();
    ImGui::BeginDisabled(true);
    custom::Button("RESET GUEST",{ImMax(1.f,ImGui::GetContentRegionAvail().x),50.f});
    ImGui::EndDisabled();
    ImGui::TextDisabled("Guest reset unavailable until the action is safely verified");
}
}
