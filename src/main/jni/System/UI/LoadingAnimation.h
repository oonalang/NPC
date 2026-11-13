#pragma once
#include "StartupUI.h"
#include "StartupState.h"
#include <chrono>

namespace ui_loading {
inline startup_ui::LoadingTimeline timeline;
inline double Now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void Start(float durationSeconds=30.f) { timeline.Start(Now(),durationSeconds); }
inline bool IsActive() { return timeline.active; }
inline float GetProgress() { return timeline.Progress(Now()); }
inline bool RenderWindow(ImTextureID=nullptr) {
    if(!IsActive()) return false;
    const float scale=startup_ui::Fit({620,600});
    const ImVec2 size=ImVec2(620,600)*scale;
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),ImGuiCond_Always,{.5f,.5f});
    ImGui::SetNextWindowSize(size+ImVec2(20,20)*scale,ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    if(ImGui::Begin("##npc_loading",nullptr,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove)) {
        startup_ui::LoadingCard(ImGui::GetWindowDrawList(),ImGui::GetWindowPos()+ImVec2(10,10)*scale,
            size,scale,GetProgress(),static_cast<float>(ImGui::GetTime()));
    }
    ImGui::End(); ImGui::PopStyleVar();
    return timeline.Finish(Now());
}
}
