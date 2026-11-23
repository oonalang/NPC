#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../ImGui/imgui_internal.h"
#include "../ImGui/TouchScroll.h"
#include "Colors.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lite_widgets {
inline float scale = 1.f;
inline void SetFontScale() {
    const auto* parent = ImGui::GetCurrentWindow()->ParentWindow;
    ImGui::SetWindowFontScale(scale / (parent ? parent->FontWindowScale : 1.f));
}
inline ImU32 Fade(ImU32 color) {
    const auto alpha = static_cast<unsigned>(((color >> 24) & 255) * ImGui::GetStyle().Alpha);
    return (color & 0x00ffffffu) | (alpha << 24);
}
inline bool Tap(bool pressed) {
    return pressed && (!custom::touch::Applies() || custom::touch::Tap());
}

inline bool Checkbox(const char* label, bool* value) {
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float rowHeight = 28.f * scale, boxSize = 20.f * scale;
    const bool clicked = Tap(ImGui::InvisibleButton(label, {width, rowHeight}));
    if (clicked) *value = !*value;
    const bool visible = ImGui::IsItemVisible();
    ImGui::SetCursorScreenPos(pos + ImVec2(0,rowHeight));
    if (!visible) return clicked;
    const ImVec2 lo = pos + ImVec2(5.f * scale, (rowHeight - boxSize) * .5f);
    const ImVec2 hi = lo + ImVec2(boxSize, boxSize);
    draw->AddRectFilled(lo, hi, Fade(*value ? lite_colors::Accent : IM_COL32(24,24,24,255)), 5.f * scale);
    draw->AddRect(lo, hi, Fade(*value ? lite_colors::Accent : IM_COL32(38,38,38,255)), 5.f * scale);
    if (*value)
        ImGui::RenderCheckMark(draw, lo + ImVec2(5,5) * scale, Fade(IM_COL32_WHITE), 10.f * scale);
    const ImVec2 textMin = pos + ImVec2(boxSize + 13.f * scale,
        (rowHeight - ImGui::GetTextLineHeight()) * .5f);
    draw->PushClipRect(textMin, {pos.x + width, pos.y + rowHeight}, true);
    draw->AddText(textMin, Fade(*value ? lite_colors::TextBright : lite_colors::Text),
        label, ImGui::FindRenderedTextEnd(label));
    draw->PopClipRect();
    return clicked;
}

inline bool SliderFloat(const char* label, float* value, float minimum, float maximum, const char* format) {
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImMax(24.f, ImGui::GetContentRegionAvail().x);
    const float lineHeight = ImGui::GetTextLineHeight();
    const float trackHeight = 5.f * scale, knobWidth = 18.f * scale, knobHeight = 12.f * scale;
    const float trackY = pos.y + lineHeight + 6.f * scale;
    const float trackLeft = pos.x + knobWidth * .5f + scale;
    const float trackWidth = ImMax(1.f, width - knobWidth - 2.f * scale);
    if (!std::isfinite(*value)) *value = minimum;
    ImGui::PushID(label);
    ImGui::InvisibleButton("##slider", {width, lineHeight + 15.f * scale});
    const auto id=GImGui->LastItemData.ID;
    custom::touch::OwnSlider(id);
    const bool visible = ImGui::IsItemVisible();
    const bool held = ImGui::IsItemActive() && !custom::touch::Suppressed();
    bool changed = false;
    const bool finish=custom::touch::Horizontal() && custom::touch::gesture.released && GImGui->ActiveIdPreviousFrame==id;
    if (held || finish || (custom::touch::Tap() && ImGui::IsItemHovered())) {
        const float amount = ImClamp((ImGui::GetIO().MousePos.x - trackLeft) / trackWidth, 0.f, 1.f);
        const float next = minimum + amount * (maximum - minimum);
        changed = std::fabs(next - *value) > .00001f;
        *value = next;
    }
    if (visible) {
        char text[48];
        std::snprintf(text, sizeof(text), format, *value);
        const auto measured = ImGui::CalcTextSize(text);
        draw->AddText(pos, Fade(*value > minimum ? lite_colors::TextBright : lite_colors::Text),
            label, ImGui::FindRenderedTextEnd(label));
        draw->AddText({pos.x + width - measured.x, pos.y}, Fade(lite_colors::TextBright), text);
        const ImVec2 lo(trackLeft, trackY), hi(trackLeft + trackWidth, trackY + trackHeight);
        draw->AddRectFilled(lo, hi, Fade(lite_colors::SliderTrack), 25.f * scale);
        const float amount = maximum > minimum ? ImClamp((*value - minimum) / (maximum - minimum), 0.f, 1.f) : 0.f;
        if (amount > .001f) {
            draw->PushClipRect(lo, {lo.x + amount * trackWidth, hi.y}, true);
            draw->AddRectFilled(lo, hi, Fade(IM_COL32(245,245,245,255)), 25.f * scale);
            draw->PopClipRect();
        }
        const ImVec2 center(trackLeft + amount * trackWidth, trackY + trackHeight * .5f);
        draw->AddRectFilled(center - ImVec2(knobWidth,knobHeight) * .5f,
            center + ImVec2(knobWidth,knobHeight) * .5f, Fade(IM_COL32(230,230,230,255)), 20.f * scale);
    }
    ImGui::SetCursorScreenPos(pos + ImVec2(0, lineHeight + 15.f * scale));
    ImGui::PopID();
    return changed;
}

inline bool Button(const char* label, ImVec2 size) {
    auto* draw = ImGui::GetWindowDrawList();
    if (size.x <= 0) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0) size.y = 28.f * scale;
    const auto lo = ImGui::GetCursorScreenPos(), hi = lo + size;
    const bool clicked = Tap(ImGui::InvisibleButton(label, size));
    if (!ImGui::IsItemVisible()) return clicked;
    const bool held = ImGui::IsItemActive();
    const auto top = Fade(held ? lite_colors::ButtonHeldTop : lite_colors::ButtonBgTop);
    const auto bottom = Fade(held ? lite_colors::ButtonHeldBot : lite_colors::ButtonBgBot);
    draw->AddRectFilledMultiColor(lo, hi, top, top, bottom, bottom);
    draw->AddRect(lo, hi, Fade(lite_colors::ButtonBorder));
    const auto textSize = ImGui::CalcTextSize(label, nullptr, true);
    draw->PushClipRect(lo, hi, true);
    draw->AddText(lo + (size - textSize) * .5f, Fade(lite_colors::TextBright), label, ImGui::FindRenderedTextEnd(label));
    draw->PopClipRect();
    return clicked;
}

inline bool BeginPanel(const char* name, ImVec2 size, ImGuiWindowFlags flags = 0) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 16.f) * scale);
    const bool visible = ImGui::BeginChild(name, size, false,
        flags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
    SetFontScale();
    ImGui::Dummy({0, 20.f * scale});
    return visible;
}
inline void EndPanel() {
    auto* window = ImGui::GetCurrentWindow();
    auto* draw = window->DrawList;
    const auto lo = window->Pos, hi = lo + window->Size;
    const auto* slash = std::strrchr(window->Name, '/');
    const char* nameStart = slash ? slash + 1 : window->Name;
    const auto length = std::strlen(nameStart);
    const char* nameEnd = nameStart + (length >= 9 ? length - 9 : length);
    nameEnd = ImGui::FindRenderedTextEnd(nameStart, nameEnd);
    const auto titleSize = ImGui::CalcTextSize(nameStart, nameEnd);
    draw->PushClipRect(window->OuterRectClipped.Min, window->OuterRectClipped.Max, false);
    draw->AddRectFilled(lo + ImVec2(1,1), {hi.x - 1, lo.y + 36.f * scale}, Fade(lite_colors::MenuBg), 9.f * scale);
    draw->AddRect(lo+ImVec2(.5f,.5f),hi-ImVec2(.5f,.5f),Fade(lite_colors::BoxBorder),9.f*scale);
    draw->AddText({lo.x + (window->Size.x - titleSize.x) * .5f, lo.y + 10.f * scale},
        Fade(lite_colors::Section), nameStart, nameEnd);
    draw->PopClipRect();
    ImGui::EndChild();
    ImGui::PopStyleVar();
}
}
