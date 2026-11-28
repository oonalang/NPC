#pragma once
#include "Colors.h"
#include "../ImGui/imgui_internal.h"

namespace lite_sidebar {
inline constexpr float radius=14.f;
inline constexpr int shadowLayers=1;
inline unsigned ShadowAlpha(int first=1, float opacity=1.f) {
    (void)first;
    return static_cast<unsigned>(ImClamp(opacity,0.f,1.f)*105.f+.5f);
}
inline void Shadow(ImVec2 lo, ImVec2 size, float opacity) {
    const unsigned alpha=ShadowAlpha(1,opacity);
    if(!alpha) return;
    auto* draw=ImGui::GetBackgroundDrawList();
    const ImVec2 offset(5.f*ImGui::GetStyle().Alpha,6.f*ImGui::GetStyle().Alpha);
    draw->AddRectFilled(lo+offset,lo+size+offset,IM_COL32(90,79,216,alpha),radius);
}
inline void Surface(ImVec2 lo, ImVec2 size, float width, float bodyHeight, ImTextureID texture, ImVec2 uv0, ImVec2 uv1) {
    auto* draw=ImGui::GetWindowDrawList();
    const ImVec2 side(lo.x+width,lo.y+size.y);
    draw->AddRectFilled(lo,lo+size,lite_colors::MenuBg,radius);
    draw->AddRectFilled(lo+ImVec2(3,4),lo+size+ImVec2(3,4),IM_COL32(90,79,216,70),radius);
    if(texture) {
        draw->AddImageRounded(texture,lo,side,uv0,uv1,IM_COL32_WHITE,radius,ImDrawFlags_RoundCornersLeft);
        draw->AddRectFilled(lo,side,IM_COL32(14,12,32,155),radius,ImDrawFlags_RoundCornersLeft);
    } else draw->AddRectFilled(lo,side,lite_colors::TitleBg,radius,ImDrawFlags_RoundCornersLeft);
    draw->AddRectFilled({lo.x+width,lo.y},{lo.x+width+2.f,lo.y+ImMin(bodyHeight,size.y)},lite_colors::SideStrip);
    draw->AddRect(lo+ImVec2(.5f,.5f),lo+size-ImVec2(.5f,.5f),IM_COL32(90,79,216,180),radius,0,ImMax(1.f,ImGui::GetStyle().Alpha*1.4f));
}
}
