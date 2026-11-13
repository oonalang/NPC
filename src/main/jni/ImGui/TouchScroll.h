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
    float initialScroll=0.f;
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
        if(gesture.active) for(auto* window=origin;window;window=window->ParentWindow) {
            if(!(window->Flags&ImGuiWindowFlags_NoScrollWithMouse) && window->ScrollMax.y>0.f) {
                gesture.scroll=window;gesture.initialScroll=window->Scroll.y;break;
            }
        }
    }
    if(!gesture.active) return;
    if(io.MousePos.x>-10000.f && io.MousePos.y>-10000.f) gesture.last=io.MousePos;
    const ImVec2 delta=gesture.last-gesture.start;
    const float threshold=ImMax(7.f,g.IO.MouseDragThreshold);
    if(gesture.intent==Intent::Pending && ImLengthSqr(delta)>=threshold*threshold) {
        gesture.intent=std::fabs(delta.y)>=std::fabs(delta.x)?Intent::Vertical:Intent::Horizontal;
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
            gesture.scroll->Scroll.y=ImClamp(gesture.initialScroll-delta.y,0.f,gesture.scroll->ScrollMax.y);
            gesture.scroll->ScrollTarget.y=FLT_MAX;
        }
        if(io.MouseDown[0]) {
            const auto id=ImHashStr("##content_pan",0,gesture.origin->ID);
            ImGui::SetActiveID(id,gesture.origin);ImGui::KeepAliveID(id);
        } else ImGui::ClearActiveID();
    }
    if(io.MouseReleased[0] || !io.MouseDown[0]) gesture.released=true;
}
inline bool Applies() {
    return GImGui && GImGui->IO.MouseSource==ImGuiMouseSource_TouchScreen && Related(ImGui::GetCurrentWindow());
}
inline bool Tap() { return Applies() && gesture.released && gesture.intent==Intent::Pending; }
inline bool Horizontal() { return Applies() && gesture.intent==Intent::Horizontal; }
inline bool Suppressed() {
    return Applies() && gesture.intent!=Intent::Horizontal && !Tap();
}
}}
