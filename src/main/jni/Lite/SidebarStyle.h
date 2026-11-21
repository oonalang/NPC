#pragma once
#include "Colors.h"
#include "../ImGui/imgui_internal.h"

namespace lite_sidebar {
inline constexpr float radius=20.f, shadowSpread=28.f;
inline constexpr int shadowLayers=12;
inline unsigned ShadowAlpha(int first=1, float opacity=1.f) {
    float remaining=1.f;
    for(int index=shadowLayers;index>=first;--index) {
        const float t=float(index)/shadowLayers;
        const unsigned alpha=static_cast<unsigned>(opacity*.18f*(1.f-t)*(1.f-t)*255.f);
        remaining*=1.f-float(alpha)/255.f;
    }
    return static_cast<unsigned>((1.f-remaining)*255.f+.5f);
}
inline void Shadow(ImVec2 lo, ImVec2 size, float opacity) {
    auto* draw=ImGui::GetBackgroundDrawList();
    const float step=shadowSpread/shadowLayers;
    for(int index=shadowLayers;index>=1;--index) {
        const float expand=(index-.5f)*step;
        const unsigned alpha=ShadowAlpha(index,opacity);
        if(alpha) draw->AddRect(lo-ImVec2(expand,expand),lo+size+ImVec2(expand,expand),
            IM_COL32(0,0,0,alpha),radius+expand*.6f,0,step);
    }
}
inline void Surface(ImVec2 lo, ImVec2 size, float width, ImTextureID texture, ImVec2 uv0, ImVec2 uv1) {
    auto* draw=ImGui::GetWindowDrawList();
    const ImVec2 side(lo.x+width,lo.y+size.y);
    if(texture) {
        draw->AddImageRounded(texture,lo,side,uv0,uv1,IM_COL32_WHITE,radius,ImDrawFlags_RoundCornersLeft);
        draw->AddRectFilled(lo,side,IM_COL32(0,0,0,ShadowAlpha()),radius,ImDrawFlags_RoundCornersLeft);
        draw->AddRectFilled(lo,side,IM_COL32(8,8,10,160),radius,ImDrawFlags_RoundCornersLeft);
    } else draw->AddRectFilled(lo,side,IM_COL32(14,14,16,255),radius,ImDrawFlags_RoundCornersLeft);
    draw->AddRectFilled({lo.x+width,lo.y},lo+size,lite_colors::MenuBg,radius,ImDrawFlags_RoundCornersRight);
    draw->AddRectFilled({lo.x+width,lo.y},{lo.x+width+1.f,lo.y+size.y},lite_colors::SideStrip);
}
}
