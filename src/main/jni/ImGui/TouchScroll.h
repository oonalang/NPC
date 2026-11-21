#pragma once
#include "imgui_internal.h"
#include <cmath>

namespace custom { namespace touch {
enum class Intent { Pending, Horizontal, Vertical, Canceled };
struct Gesture {
    ImGuiContext* context=nullptr;
    ImGuiWindow* origin=nullptr;
    ImGuiWindow* scroll=nullptr;
    ImVec2 start{},last{};
    ImGuiID slider=0;
    int frame=-1,intentFrame=-1;
    Intent intent=Intent::Pending;
    bool active=false,released=false;
};
inline Gesture gesture;
inline bool Related(ImGuiWindow* window) {
    if(!gesture.active || !gesture.origin) return false;
    for(auto* current=window;current;current=current->ParentWindow)
        if(current==gesture.origin) return true;
    return false;
}
inline void Cancel() {
    if(!gesture.active) return;
    gesture.intent=Intent::Canceled;
    if(GImGui && GImGui->ActiveIdWindow && Related(GImGui->ActiveIdWindow)) ImGui::ClearActiveID();
}
inline void OwnSlider(ImGuiID id) {
    if(gesture.active && gesture.intent==Intent::Pending && GImGui->IO.MouseClicked[0] &&
        GImGui->ActiveId==id && Related(ImGui::GetCurrentWindow())) gesture.slider=id;
}
inline void Pan(float amount) {
    for(auto* window=gesture.scroll;window && std::fabs(amount)>.001f;window=window->ParentWindow) {
        if(!(window->Flags&ImGuiWindowFlags_NoScrollWithMouse) && window->ScrollMax.y>0.f) {
            const float before=window->Scroll.y;
            window->Scroll.y=ImClamp(before+amount,0.f,window->ScrollMax.y);
            window->ScrollTarget.y=FLT_MAX;
            amount-=window->Scroll.y-before;
        }
        if(window->Flags&ImGuiWindowFlags_Popup) break;
    }
}
inline void Update() {
    auto& g=*GImGui;
    auto& io=g.IO;
    if(gesture.context!=&g) { gesture={};gesture.context=&g; }
    if(gesture.frame==g.FrameCount) return;
    gesture.frame=g.FrameCount;
    if(gesture.released) { gesture.active=false;gesture.released=false; }
    if(io.MouseSource!=ImGuiMouseSource_TouchScreen) { gesture.active=false;return; }
    if(io.MouseClicked[0]) {
        auto* origin=g.HoveredWindow;
        gesture.origin=origin;
        gesture.active=origin && (origin->Flags&(ImGuiWindowFlags_ChildWindow|ImGuiWindowFlags_Popup));
        gesture.start=gesture.last=io.MousePos;
        gesture.intent=Intent::Pending;
        gesture.scroll=nullptr;
        gesture.slider=0;
        if(gesture.active) for(auto* window=origin;window;window=window->ParentWindow) {
            if(!(window->Flags&ImGuiWindowFlags_NoScrollWithMouse) && window->ScrollMax.y>0.f) {
                gesture.scroll=window;break;
            }
        }
    }
    if(!gesture.active) return;
    const ImVec2 previous=gesture.last;
    if(ImGui::IsMousePosValid(&io.MousePos)) gesture.last=io.MousePos;
    const ImVec2 delta=gesture.last-gesture.start;
    const float threshold=ImMax(10.f,io.MouseDragThreshold*1.5f);
    if(gesture.intent==Intent::Pending && ImMax(std::fabs(delta.x),std::fabs(delta.y))>=threshold) {
        const float horizontalBias=gesture.slider ? .6f : 1.f;
        gesture.intent=std::fabs(delta.x)>=std::fabs(delta.y)*horizontalBias ? Intent::Horizontal : Intent::Vertical;
        gesture.intentFrame=g.FrameCount;
    }
    if(g.MovingWindow && (g.MovingWindow==gesture.origin || g.MovingWindow==gesture.origin->RootWindow)) {
        g.MovingWindow=nullptr;
        if(g.ActiveId==gesture.origin->RootWindow->MoveId) ImGui::ClearActiveID();
    }
    if(gesture.intent==Intent::Vertical || gesture.intent==Intent::Canceled) {
        if(g.ActiveIdWindow && Related(g.ActiveIdWindow)) ImGui::ClearActiveID();
        if(g.MovingWindow && (g.MovingWindow==gesture.origin || g.MovingWindow==gesture.origin->RootWindow)) g.MovingWindow=nullptr;
        if(gesture.intent==Intent::Vertical && gesture.scroll) {
            Pan(gesture.intentFrame==g.FrameCount ? -delta.y : previous.y-gesture.last.y);
        }
        if(io.MouseDown[0]) {
            const auto id=ImHashStr("##content_pan",0,gesture.origin->ID);
            ImGui::SetActiveID(id,gesture.origin);ImGui::KeepAliveID(id);
        } else ImGui::ClearActiveID();
    }
    if(io.MouseReleased[0] || !io.MouseDown[0]) gesture.released=true;
}
inline bool Applies() {
    return GImGui && GImGui->CurrentWindow && GImGui->IO.MouseSource==ImGuiMouseSource_TouchScreen && Related(GImGui->CurrentWindow);
}
inline bool Tap() { return Applies() && gesture.released && gesture.intent==Intent::Pending; }
inline bool Horizontal() { return Applies() && gesture.intent==Intent::Horizontal; }
inline bool Suppressed() {
    return Applies() && gesture.intent!=Intent::Horizontal && !Tap();
}
}}
