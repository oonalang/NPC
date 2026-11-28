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
inline ImVec2 Rotate(ImVec2 point, ImVec2 center, float angle) {
    const float c = std::cos(angle), s = std::sin(angle);
    const auto local = point - center;
    return center + ImVec2(local.x * c - local.y * s, local.x * s + local.y * c);
}
inline void Panel(ImDrawList* draw, ImVec2 center, ImVec2 size, float angle, ImU32 fill, ImU32 border, float width = 2.f) {
    const auto half = size * .5f;
    const ImVec2 corners[4]{center + ImVec2(-half.x, -half.y), center + ImVec2(half.x, -half.y),
        center + ImVec2(half.x, half.y), center + ImVec2(-half.x, half.y)};
    ImVec2 points[4];
    for (int index = 0; index < 4; ++index) points[index] = Rotate(corners[index], center, angle);
    draw->AddQuadFilled(points[0], points[1], points[2], points[3], fill);
    draw->AddPolyline(points, 4, border, ImDrawFlags_Closed, width);
}
inline void Sparkle(ImDrawList* draw, ImVec2 center, float radius, ImU32 color) {
    const float inner = radius * .22f;
    const ImVec2 points[8]{center + ImVec2(0, -radius), center + ImVec2(inner, -inner),
        center + ImVec2(radius, 0), center + ImVec2(inner, inner), center + ImVec2(0, radius),
        center + ImVec2(-inner, inner), center + ImVec2(-radius, 0), center + ImVec2(-inner, -inner)};
    draw->AddConvexPolyFilled(points, 8, color);
}
inline void Brush(ImDrawList* draw, ImVec2 center, float scale, float angle, ImU32 color) {
    if (sakuga_art::rectIds[0] < 0 || sakuga_art::uv1[0].x <= 0.f) return;
    const ImVec2 half(sakuga_art::donWidth * scale * .5f, sakuga_art::donHeight * scale * .5f);
    const ImVec2 corners[4]{center + ImVec2(-half.x, -half.y), center + ImVec2(half.x, -half.y),
        center + ImVec2(half.x, half.y), center + ImVec2(-half.x, half.y)};
    ImVec2 points[4];
    for (int index = 0; index < 4; ++index) points[index] = Rotate(corners[index], center, angle);
    draw->AddImageQuad(ImGui::GetIO().Fonts->TexID, points[0], points[1], points[2], points[3],
        sakuga_art::uv0[0], {sakuga_art::uv1[0].x, sakuga_art::uv0[0].y},
        sakuga_art::uv1[0], {sakuga_art::uv0[0].x, sakuga_art::uv1[0].y}, color);
}
inline void Card(bool reducedMotion) {
    auto* window = ImGui::GetCurrentWindow();
    auto* draw = window->DrawList;
    const auto pos = window->Pos, size = window->Size;
    const float time = reducedMotion ? 0.f : static_cast<float>(ImGui::GetTime());
    const float wave = .5f + .5f * std::sin(time * 4.6f);
    const float beat = 1.f + .075f * wave * wave * wave * wave;
    const float angle = -.16f + (reducedMotion ? 0.f : std::sin(time * 1.8f) * .018f);
    const auto sticker = pos + ImVec2(size.x - 70.f, 57.f);
    draw->PushClipRect(pos - ImVec2(12, 12), pos + size + ImVec2(12, 12), false);
    draw->AddRectFilled(pos + ImVec2(7, 8), pos + size + ImVec2(7, 8), IM_COL32(5, 4, 16, 225), 19);
    draw->AddRectFilled(pos, pos + size, IM_COL32(22, 18, 50, 255), 19);
    draw->AddRect(pos + ImVec2(.5f, .5f), pos + size - ImVec2(.5f, .5f),
        lite_widgets::Fade(IM_COL32(184, 179, 255, 220)), 19, 0, 2.2f);
    draw->AddLine(pos + ImVec2(22, 2), pos + ImVec2(size.x - 22, 2), IM_COL32(255, 255, 255, 45), 1.f);
    for (int index = 0; index < 11; ++index) {
        const float ray = -2.95f + index * .215f;
        const ImVec2 direction(std::cos(ray), std::sin(ray));
        const float inner = 52.f, outer = inner + 13.f + (index % 3) * 5.f;
        draw->AddLine(sticker + direction * inner, sticker + direction * outer,
            lite_widgets::Fade(IM_COL32(184, 179, 255, 70)), 2.f);
    }
    Panel(draw, sticker + ImVec2(4, 5), {116, 78}, angle, IM_COL32(7, 5, 20, 230), IM_COL32(7, 5, 20, 230));
    Panel(draw, sticker, {116, 78}, angle, lite_widgets::Fade(lite_colors::Accent),
        lite_widgets::Fade(IM_COL32(14, 10, 32, 255)), 2.4f);
    Brush(draw, sticker, .58f * beat, angle, lite_widgets::Fade(lite_colors::AccentInk));
    Sparkle(draw, sticker + ImVec2(-66, -38), 10.f + wave * 2.f, lite_widgets::Fade(lite_colors::Section));
    Sparkle(draw, sticker + ImVec2(64, 43), 7.f + wave * 2.f, lite_widgets::Fade(lite_colors::Soft));
    for (int row = 0; row < 3; ++row) for (int column = 0; column < 6; ++column)
        draw->AddCircleFilled(pos + ImVec2(25.f + column * 9.f, size.y - 43.f + row * 9.f), 1.35f,
            lite_widgets::Fade(IM_COL32(184, 179, 255, 75)), 8);
    draw->PopClipRect();
}
inline void Header(ImFont* titleFont, bool pending) {
    if (!titleFont) titleFont = ImGui::GetFont();
    const auto pos = ImGui::GetWindowPos();
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddText(titleFont, 27, pos + ImVec2(25, 21), Ink(242), "SHAINIME");
    draw->AddText(ImGui::GetFont(), 12, pos + ImVec2(27, 62), Ink(150),
        pending ? "Checking your access" : "Sign in to continue");
}
inline bool Secondary(const char* label, ImVec2 size) {
    const auto lo = ImGui::GetCursorScreenPos();
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton(label,size));
    const bool held = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered();
    const auto shift = held ? ImVec2(1, 2) : ImVec2(0, 0);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo + ImVec2(3, 4), lo + size + ImVec2(3, 4), IM_COL32(5, 4, 16, 235), 10);
    draw->AddRectFilled(lo + shift, lo + size + shift,
        lite_widgets::Fade(hovered ? lite_colors::BindItemBg : lite_colors::Card), 10);
    draw->AddRect(lo + shift, lo + size + shift,
        lite_widgets::Fade(hovered ? lite_colors::Accent : lite_colors::CardBorder), 10, 0, 1.8f);
    Centered(nullptr, 13, {lo.x + size.x * .5f + shift.x, lo.y + (size.y - 13.f) * .5f + shift.y}, label, Ink(225));
    return pressed;
}
inline bool Primary(const char* label, bool pending, bool reduced, ImVec2 size) {
    const auto lo = ImGui::GetCursorScreenPos();
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton("Authenticate",size));
    const bool held = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered();
    const auto shift = held ? ImVec2(2, 2) : ImVec2(0, 0);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo + ImVec2(5, 5), lo + size + ImVec2(5, 5), IM_COL32(5, 4, 16, 245), 12);
    draw->AddRectFilled(lo + shift, lo + size + shift,
        lite_widgets::Fade(hovered ? lite_colors::AccentHover : lite_colors::Accent), 12);
    draw->AddRect(lo + shift, lo + size + shift, IM_COL32(14, 10, 32, 255), 12, 0, 2.2f);
    const auto center = lo + size * .5f + shift;
    if (pending) {
        const float time = reduced ? 0.f : float(ImGui::GetTime());
        for (int index = 0; index < 3; ++index) {
            const float pulse = reduced ? 1.f : .5f + .5f * std::sin(time * 2.f * pi / authenticatePulseSeconds - index * .9f);
            Sparkle(draw, center + ImVec2((index - 1) * 12.f, -2.5f * pulse), 3.5f + pulse, Ink(24));
        }
    } else {
        Centered(nullptr, 15, {center.x, center.y - 7.5f}, label, lite_colors::AccentInk);
        Sparkle(draw, lo + size - ImVec2(19, size.y * .5f) + shift, 6.5f, lite_colors::AccentInk);
    }
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
    Arc(draw,center,radius,theta,pi*.65f,Ink(230,1.f-finish),1.5);
    Arc(draw,center,radius,-theta+pi,pi*.22f,Ink(100,1.f-finish),1);
    for (int index=0;index<4;++index) {
        const float angle = theta*.5f+index*pi*.5f;
        const float distance = (18.f+2.f*std::sin(theta-index*pi*.5f))*(1.f-finish);
        const ImVec2 pivot = center+ImVec2(std::cos(angle)*distance,std::sin(angle)*distance);
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
    Centered(titleFont,24,{center.x,center.y+1.f},"SHAINIME",Ink(242));
    Centered(nullptr,13,{center.x,center.y+38.f},frame.finish>0?"You're all set":"Preparing your interface",Ink(145));
    auto* draw = ImGui::GetWindowDrawList();
    const float width = ImMin(168.f,size.x-64.f);
    const ImVec2 lo(center.x-width*.5f,center.y+94.f), hi=lo+ImVec2(width,7.f);
    draw->AddRectFilled(lo+ImVec2(3,4),hi+ImVec2(3,4),IM_COL32(5,4,16,235),5);
    draw->AddRectFilled(lo,hi,Ink(32),5);
    draw->AddRect(lo,hi,Ink(150),5,0,1.4f);
    if (frame.active && !frame.reducedMotion) progress = ImMin(progress,.9f*Ease(float(frame.elapsed/minimumLoadingSeconds)));
    const float amount = frame.finish>0 ? progress+(1.f-progress)*Ease(frame.finish) : progress;
    if (amount>0) draw->AddRectFilled(lo,{lo.x+width*ImClamp(amount,0.f,1.f),hi.y},lite_widgets::Fade(lite_colors::Accent),5);
}
}
