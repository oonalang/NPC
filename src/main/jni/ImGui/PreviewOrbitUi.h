#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "PreviewOrbit.h"
#include "imgui_internal.h"
#include "TouchScroll.h"
namespace anime_preview {
inline void DrawOrbitControls(OrbitControl& orbit,ImDrawList* drawList,ImFont* textFont,ImVec2 min,ImVec2 max,ImVec2 viewMin,ImVec2 viewMax) {
    const ImVec2 size(max.x-min.x,max.y-min.y);
    ImGui::PushID("ESP preview camera");
    const ImRect orbitButton({max.x-73.f,min.y+9.f},{max.x-29.f,min.y+31.f});
    const auto buttonId=ImGui::GetID("Orbit toggle");
    bool hovered=false,held=false;
    if(size.x>=165.f&&ImGui::ItemAdd(orbitButton,buttonId)&&ImGui::ButtonBehavior(orbitButton,buttonId,&hovered,&held,ImGuiButtonFlags_PressedOnClickRelease)&&(!custom::touch::Applies()||custom::touch::Tap())) orbit.Toggle();
    if(size.x>=165.f) {
        drawList->AddRectFilled(orbitButton.Min,orbitButton.Max,orbit.enabled?IM_COL32(119,82,164,160):IM_COL32(255,255,255,14),9.f);
        const char* label=orbit.enabled?"Front":"Orbit";const auto measured=textFont->CalcTextSizeA(9.f,FLT_MAX,0,label);
        drawList->AddText(textFont,9.f,(orbitButton.Min+orbitButton.Max-measured)*.5f,IM_COL32(224,216,239,220),label);
    }
    const auto dragId=ImGui::GetID("Orbit drag");
    auto& input=ImGui::GetIO();bool dragHovered=false,dragHeld=false;
    if(orbit.enabled&&ImGui::ItemAdd(ImRect(viewMin,viewMax),dragId)) ImGui::ButtonBehavior(ImRect(viewMin,viewMax),dragId,&dragHovered,&dragHeld,ImGuiButtonFlags_PressedOnClick);
    const bool canceled=custom::touch::Applies()&&(custom::touch::gesture.intent==custom::touch::Intent::Vertical||custom::touch::gesture.intent==custom::touch::Intent::Canceled);
    orbit.Update(input.MouseClicked[0],input.MouseDown[0],dragHovered,input.MousePos.x,input.MousePos.y,viewMax.x-viewMin.x,canceled);
    if(!orbit.pending&&!orbit.dragging&&GImGui->ActiveId==dragId) ImGui::ClearActiveID();
    ImGui::PopID();
}
}
