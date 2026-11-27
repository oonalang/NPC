#pragma once
#include "Widgets.h"
#include "StartupMotion.h"

namespace lite_startup {
inline constexpr float pi = 3.14159265358979323846f;
inline ImU32 Ink(int gray, float alpha = 1.f) {
    return IM_COL32(gray,gray,gray,int(255.f*ImClamp(alpha*ImGui::GetStyle().Alpha,0.f,1.f)));
}
inline void Centered(ImFont* font, float size, ImVec2 center, const char* text, ImU32 color) {
    if (!font) font = ImGui::GetFont();
    const auto measured = font->CalcTextSizeA(size,FLT_MAX,0,text);
    ImGui::GetWindowDrawList()->AddText(font,size,{center.x-measured.x*.5f,center.y},color,text);
}
inline void Edition(ImVec2 center, float y) {
    const char* text = "LITE";
    auto* font = ImGui::GetFont();
    constexpr float size = 10.f, tracking = 3.f;
    float width = -tracking;
    for (const char* at=text;*at;++at) width += font->FindGlyph(*at)->AdvanceX*size/font->FontSize+tracking;
    float x = center.x-width*.5f;
    for (const char* at=text;*at;++at) {
        char glyph[]{*at,0};
        ImGui::GetWindowDrawList()->AddText(font,size,{x,y},Ink(136),glyph);
        x += font->FindGlyph(*at)->AdvanceX*size/font->FontSize+tracking;
    }
}
inline void Card() {
    auto* window = ImGui::GetCurrentWindow();
    auto* draw = window->DrawList;
    draw->PushClipRect(window->OuterRectClipped.Min,window->OuterRectClipped.Max,false);
    draw->AddRectFilled(window->Pos,window->Pos+window->Size,lite_colors::MenuBg,16);
    draw->AddRect(window->Pos+ImVec2(.5f,.5f),window->Pos+window->Size-ImVec2(.5f,.5f),lite_widgets::Fade(lite_colors::CardBorder),16);
    draw->PopClipRect();
}
inline bool Secondary(const char* label, ImVec2 size) {
    const auto lo = ImGui::GetCursorScreenPos();
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton(label,size));
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo,lo+size,ImGui::IsItemHovered()?lite_widgets::Fade(lite_colors::BindItemBg):lite_widgets::Fade(lite_colors::Card),8);
    draw->AddRect(lo,lo+size,lite_widgets::Fade(ImGui::IsItemHovered()?lite_colors::Accent:lite_colors::CardBorder),8);
    Centered(nullptr,13,{lo.x+size.x*.5f,lo.y+(size.y-13.f)*.5f},label,Ink(210));
    return pressed;
}
inline bool Primary(const char* label, bool pending, bool reduced, ImVec2 size) {
    const auto lo = ImGui::GetCursorScreenPos();
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton("Authenticate",size));
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo,lo+size,ImGui::IsItemActive()?lite_colors::AccentHover:lite_colors::Accent,9);
    draw->AddRect(lo,lo+size,lite_colors::AccentHover,9,0,ImMax(1.f,1.4f*ImGui::GetStyle().Alpha));
    if (pending) {
        const auto center = lo+size*.5f;
        const float time = reduced ? 0.f : float(ImGui::GetTime());
        for (int index=0;index<3;++index) {
            const float pulse = reduced ? 1.f : .5f+.5f*std::sin(time*2.f*pi/authenticatePulseSeconds-index*.9f);
            draw->AddCircleFilled(center+ImVec2((index-1)*10.f,-2.5f*pulse),2.f+.5f*pulse,Ink(24),12);
        }
    } else Centered(nullptr,15,{lo.x+size.x*.5f,lo.y+(size.y-15.f)*.5f},label,lite_colors::AccentInk);
    return pressed;
}
inline void Arc(ImDrawList* draw, ImVec2 center, float radius, float angle, float length, ImU32 color, float width) {
    draw->PathArcTo(center,radius,angle,angle+length,24);
    draw->PathStroke(color,0,width);
}
inline void Mark(ImVec2 center, const Frame& frame) {
    auto* draw = ImGui::GetWindowDrawList();
    const float time = frame.reducedMotion ? 0.f : float(std::fmod(frame.elapsed,orbitSeconds)/orbitSeconds);
    const float finish = Ease(frame.finish);
    const float theta = time*2.f*pi;
    const float breath = frame.reducedMotion ? 0.f : std::sin(float(std::fmod(frame.elapsed,breatheSeconds)/breatheSeconds)*2.f*pi);
    const float radius = 35.f+breath;
    Arc(draw,center,radius,0,2.f*pi,Ink(44,1.f-finish),1);
    Arc(draw,center,radius,theta,pi*.65f,Ink(230,1.f-finish),1.5f);
    Arc(draw,center,radius,-theta+pi,pi*.22f,Ink(100,1.f-finish),1);
    for (int index=0;index<4;++index) {
        const float angle = theta*.5f+index*pi*.5f;
        const float distance = (18.f+2.f*std::sin(theta-index*pi*.5f))*(1.f-finish);
        const ImVec2 pivot = center+ImVec2(std::cos(angle),std::sin(angle))*distance;
        const float turn = angle+pi*.25f;
        const ImVec2 axis(std::cos(turn)*7.f,std::sin(turn)*7.f);
        const ImVec2 side(-std::sin(turn)*4.f,std::cos(turn)*4.f);
        const ImVec2 points[]{pivot-axis-side,pivot+axis-side,pivot+axis+side,pivot-axis+side};
        const float alpha = (.55f+.45f*(.5f+.5f*std::sin(theta-index*pi*.5f)))*(1.f-finish);
        draw->AddConvexPolyFilled(points,4,Ink(240,alpha));
    }
    if (frame.finish > 0) {
        draw->AddCircleFilled(center,29.f*Ease(ImClamp(frame.finish*3.f,0.f,1.f)),lite_widgets::Fade(lite_colors::Section),48);
        const float trace = Ease(ImClamp((frame.finish-.12f)/.42f,0.f,1.f));
        const ImVec2 a=center+ImVec2(-10,0), b=center+ImVec2(-3,7), c=center+ImVec2(12,-8);
        if (trace > 0) {
            draw->AddLine(a,a+(b-a)*ImMin(trace/.34f,1.f),Ink(18,finish),2.8f);
            if (trace > .34f) draw->AddLine(b,b+(c-b)*((trace-.34f)/.66f),Ink(18,finish),2.8f);
        }
    }
}
inline void Loading(ImFont* titleFont, float progress, const Frame& frame) {
    const auto pos = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
    const auto center = pos+size*.5f;
    Mark({center.x,center.y-57.f},frame);
    Centered(titleFont,24,{center.x,center.y+1.f},"ETHNIR",Ink(242));
    Edition(center,center.y+32.f);
    Centered(nullptr,13,{center.x,center.y+64.f},frame.finish>0?"You're all set":"Preparing your interface",Ink(145));
    auto* draw = ImGui::GetWindowDrawList();
    const float width = ImMin(168.f,size.x-64.f);
    const ImVec2 lo(center.x-width*.5f,center.y+94.f), hi=lo+ImVec2(width,2.f);
    draw->AddRectFilled(lo,hi,Ink(43),2);
    if (frame.active && !frame.reducedMotion) progress = ImMin(progress,.9f*Ease(float(frame.elapsed/minimumLoadingSeconds)));
    const float amount = frame.finish>0 ? progress+(1.f-progress)*Ease(frame.finish) : progress;
    if (amount>0) draw->AddRectFilled(lo,{lo.x+width*ImClamp(amount,0.f,1.f),hi.y},Ink(225),2);
}
}
