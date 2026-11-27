#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../ImGui/imgui_internal.h"
#include "../ImGui/TouchScroll.h"
#include "Colors.h"
#include "Sakuga.h"
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
    const float rowHeight = 28.f * scale;
    const float trackWidth = 38.f * scale, trackHeight = 20.f * scale, knob = 15.f * scale;
    const bool clicked = Tap(ImGui::InvisibleButton(label, {width, rowHeight}));
    if (clicked) *value = !*value;
    const bool visible = ImGui::IsItemVisible();
    const auto id = GImGui->LastItemData.ID;
    const bool held = ImGui::IsItemActive();
    const auto pose = sakuga::ControlPose(id, false, clicked, 1.f, .55f, true);
    ImGui::SetCursorScreenPos(pos + ImVec2(0, rowHeight));
    if (!visible) return clicked;
    const ImVec2 desiredCenter(pos.x + width - trackWidth * .5f, pos.y + rowHeight * .5f);
    sakuga::Behind(draw, id);
    if (clicked) sakuga::CheckboxImpact(id, desiredCenter, *value);
    if (ImGui::IsItemHovered()) draw->AddRectFilled(pos + ImVec2(0, 1.f * scale),
        {pos.x + width, pos.y + rowHeight - 1.f * scale}, IM_COL32(255, 255, 255, 8), 6.f * scale);
    ImVec2 trackLo(pos.x + width - trackWidth, pos.y + (rowHeight - trackHeight) * .5f);
    ImVec2 trackHi = trackLo + ImVec2(trackWidth, trackHeight);
    if (pose.active) {
        const ImVec2 center = (trackLo + trackHi) * .5f;
        trackLo = center + (trackLo - center) * ImVec2(pose.x, pose.y);
        trackHi = center + (trackHi - center) * ImVec2(pose.x, pose.y);
    }
    const ImU32 track = *value ? lite_colors::Primary : lite_colors::ToggleOff;
    draw->AddRectFilled(trackLo, trackHi, Fade(track), trackHeight * .5f);
    draw->AddRect(trackLo, trackHi, Fade(*value ? lite_colors::Accent : lite_colors::CbBorder),
        trackHeight * .5f, 0, ImMax(1.f, 1.4f * scale));
    const float centerY = (trackLo.y + trackHi.y) * .5f;
    const float radius = knob * .5f * ImMax(.75f, pose.y);
    const float knobX = *value ? trackHi.x - trackHeight * .5f : trackLo.x + trackHeight * .5f;
    draw->AddCircleFilled({knobX, centerY}, radius, Fade(lite_colors::Section), 18);
    const ImVec2 textMin = pos + ImVec2(0, (rowHeight - ImGui::GetTextLineHeight()) * .5f);
    draw->PushClipRect(textMin, {pos.x + width - trackWidth - 6.f * scale, pos.y + rowHeight}, true);
    draw->AddText(textMin, Fade(*value ? lite_colors::TextBright : lite_colors::Text),
        label, ImGui::FindRenderedTextEnd(label));
    draw->PopClipRect();
    (void)held;
    return clicked;
}

inline bool SliderFloat(const char* label, float* value, float minimum, float maximum, const char* format) {
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImMax(24.f, ImGui::GetContentRegionAvail().x);
    const float lineHeight = ImGui::GetTextLineHeight();
    const float trackHeight = 5.f * scale, knobWidth = 13.f * scale, knobHeight = 13.f * scale;
    const float trackY = pos.y + lineHeight + 9.f * scale;
    const float trackLeft = pos.x + knobWidth * .5f;
    const float trackWidth = ImMax(1.f, width - knobWidth);
    if (!std::isfinite(*value)) *value = minimum;
    ImGui::PushID(label);
    ImGui::InvisibleButton("##slider", {width, lineHeight + 22.f * scale});
    const auto id = GImGui->LastItemData.ID;
    custom::touch::OwnSlider(id);
    const bool visible = ImGui::IsItemVisible();
    const bool held = ImGui::IsItemActive() && !custom::touch::Suppressed();
    bool changed = false;
    const bool finish = custom::touch::Horizontal() && custom::touch::gesture.released && GImGui->ActiveIdPreviousFrame == id;
    if (held || finish || (custom::touch::Tap() && ImGui::IsItemHovered())) {
        const float amount = ImClamp((ImGui::GetIO().MousePos.x - trackLeft) / trackWidth, 0.f, 1.f);
        const float next = minimum + amount * (maximum - minimum);
        changed = std::fabs(next - *value) > .00001f;
        *value = next;
    }
    const float amount = maximum > minimum ? ImClamp((*value - minimum) / (maximum - minimum), 0.f, 1.f) : 0.f;
    const ImVec2 rawCenter(trackLeft + amount * trackWidth, trackY + trackHeight * .5f);
    const auto motion = sakuga::SliderMotion(id, amount, held, finish, rawCenter);
    if (visible) {
        char text[48];
        std::snprintf(text, sizeof(text), format, *value);
        const auto measured = ImGui::CalcTextSize(text);
        draw->AddText(pos, Fade(*value > minimum ? lite_colors::TextBright : lite_colors::Text),
            label, ImGui::FindRenderedTextEnd(label));
        draw->AddText({pos.x + width - measured.x, pos.y}, Fade(lite_colors::Accent), text);
        const ImVec2 lo(trackLeft, trackY), hi(trackLeft + trackWidth, trackY + trackHeight);
        draw->AddRectFilled(lo, hi, Fade(lite_colors::SliderTrack), trackHeight * .5f);
        const float shown = ImClamp(motion.position, 0.f, 1.f);
        if (shown > .001f) {
            draw->PushClipRect(lo, {lo.x + shown * trackWidth, hi.y}, true);
            draw->AddRectFilled(lo, hi, Fade(lite_colors::Primary), trackHeight * .5f);
            draw->PopClipRect();
        }
        const ImVec2 center(trackLeft + motion.position * trackWidth, trackY + trackHeight * .5f);
        const ImVec2 thumb(knobWidth * motion.stretch * .5f, knobHeight / std::sqrt(motion.stretch) * .5f);
        draw->AddRectFilled(center - thumb, center + thumb, Fade(lite_colors::Section), 5.f * scale);
        draw->AddRect(center - thumb, center + thumb, Fade(lite_colors::AccentDark), 5.f * scale, 0, ImMax(1.f, 1.3f * scale));
    }
    ImGui::SetCursorScreenPos(pos + ImVec2(0, lineHeight + 22.f * scale));
    ImGui::PopID();
    return changed;
}

inline bool Button(const char* label, ImVec2 size) {
    auto* draw = ImGui::GetWindowDrawList();
    if (size.x <= 0) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0) size.y = 34.f * scale;
    ImVec2 lo = ImGui::GetCursorScreenPos(), hi = lo + size;
    const bool clicked = Tap(ImGui::InvisibleButton(label, size));
    const bool held = ImGui::IsItemActive();
    const bool activated = ImGui::IsItemActivated();
    const auto id = GImGui->LastItemData.ID;
    const auto pose = sakuga::ControlPose(id, held, clicked);
    if (!ImGui::IsItemVisible()) return clicked;
    const ImVec2 center = (lo + hi) * .5f;
    if (activated) sakuga::ButtonDown(center, size.x * .5f, size.y * .5f);
    if (clicked) sakuga::ButtonImpact(id, lo, hi);
    if (pose.active) {
        lo = center + (lo - center) * ImVec2(pose.x, pose.y);
        hi = center + (hi - center) * ImVec2(pose.x, pose.y);
    }
    const bool invert = sakuga::Inverting(id);
    sakuga::Behind(draw, id);
    if (!invert) draw->AddRectFilled(lo + ImVec2(4.f, 5.f) * scale, hi + ImVec2(4.f, 5.f) * scale,
        Fade(lite_colors::Soft), 9.f * scale);
    draw->AddRectFilled(lo, hi, Fade(invert ? lite_colors::Section : held ? lite_colors::BindItemBg : lite_colors::Card),
        9.f * scale);
    draw->AddRect(lo, hi, Fade(ImGui::IsItemHovered() || held ? lite_colors::Accent : lite_colors::AccentDark),
        9.f * scale, 0, ImMax(1.f, 1.6f * scale));
    const auto textSize = ImGui::CalcTextSize(label, nullptr, true);
    draw->PushClipRect(lo, hi, true);
    draw->AddText(ImVec2(lo.x + (hi.x - lo.x - textSize.x) * .5f, lo.y + (hi.y - lo.y - textSize.y) * .5f),
        Fade(invert ? lite_colors::AccentInk : lite_colors::TextBright), label, ImGui::FindRenderedTextEnd(label));
    draw->PopClipRect();
    if (clicked) {
        const ImVec2 c((lo.x + hi.x) * .5f, (lo.y + hi.y) * .5f);
        draw->AddCircleFilled(c, 1.f, Fade(lite_colors::Accent), 6);
    }
    return clicked;
}

inline void InfoRow(const char* label, const char* value, ImU32 color) {
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float rowHeight = 26.f * scale;
    ImGui::Dummy({width, rowHeight});
    if (!ImGui::IsItemVisible()) { ImGui::SetCursorScreenPos(pos + ImVec2(0, rowHeight)); return; }
    const float textY = pos.y + (rowHeight - ImGui::GetTextLineHeight()) * .5f;
    draw->AddText({pos.x, textY}, Fade(lite_colors::TextDim), label);
    const auto measured = ImGui::CalcTextSize(value);
    draw->PushClipRect({pos.x, pos.y}, {pos.x + width, pos.y + rowHeight}, true);
    draw->AddText({pos.x + width - measured.x, textY}, Fade(color), value);
    draw->PopClipRect();
}

inline bool BeginPanel(const char* name, ImVec2 size, ImGuiWindowFlags flags = 0, const char* title = nullptr) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 16.f) * scale);
    const bool visible = ImGui::BeginChild(name, size, false,
        flags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
    SetFontScale();
    auto* window = ImGui::GetCurrentWindow();
    auto* draw = window->DrawList;
    const auto lo = window->Pos, hi = lo + window->Size;
    const float radius = 12.f * scale;
    draw->PushClipRect(window->OuterRectClipped.Min, window->OuterRectClipped.Max, false);
    draw->AddRectFilled(lo + ImVec2(3.f, 4.f) * scale, hi + ImVec2(3.f, 4.f) * scale,
        Fade(IM_COL32(90, 79, 216, 55)), radius);
    draw->AddRectFilled(lo, hi, Fade(lite_colors::Card), radius);
    draw->AddRect(lo + ImVec2(.5f, .5f), hi - ImVec2(.5f, .5f), Fade(lite_colors::CardBorder), radius, 0,
        ImMax(1.f, 1.2f * scale));
    const auto* slash = std::strrchr(window->Name, '/');
    const char* nameStart = slash ? slash + 1 : window->Name;
    const auto length = std::strlen(nameStart);
    const char* nameEnd = nameStart + (length >= 9 ? length - 9 : length);
    nameEnd = ImGui::FindRenderedTextEnd(nameStart, nameEnd);
    const char* header = title ? title : nameStart;
    const char* headerEnd = title ? title + std::strlen(title) : nameEnd;
    if (headerEnd > header)
        draw->AddText(ImGui::GetFont(), 15.f * scale, {lo.x + 16.f * scale, lo.y + 12.f * scale},
            Fade(lite_colors::TextBright), header, headerEnd);
    draw->PopClipRect();
    ImGui::Dummy({0, 24.f * scale});
    return visible;
}
inline void EndPanel() { ImGui::EndChild(); ImGui::PopStyleVar(); }
}
