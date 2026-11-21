#include "Widgets.h"
#include "../ImGui/custom_widgets.hpp"

namespace custom {
void UpdateTouchScroll() { touch::Update(); }
void CancelTouchScroll() { touch::Cancel(); }
bool Checkbox(const char* label, bool* value) { return lite_widgets::Checkbox(label, value); }
bool CheckboxClicked(const char* label, bool* value) { return Checkbox(label, value); }
bool SliderFloat(const char* label, float* value, float low, float high, const char* format, ImGuiSliderFlags) {
    return lite_widgets::SliderFloat(label, value, low, high, format);
}
bool SliderInt(const char* label, int* value, int low, int high, const char*, ImGuiSliderFlags) {
    float number = static_cast<float>(*value);
    const bool changed = lite_widgets::SliderFloat(label, &number, static_cast<float>(low), static_cast<float>(high), "%.0f");
    if (changed) *value = static_cast<int>(std::round(number));
    return changed;
}
bool Button(const char* label, const ImVec2& size) { return lite_widgets::Button(label, size); }
bool Page(bool selected, const char* label, const ImVec2& size, bool, bool) {
    if (selected) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(lite_colors::TextBright));
    const bool pressed = lite_widgets::Button(label, size);
    if (selected) ImGui::PopStyleColor();
    return pressed;
}
void BeginGroup() { ImGui::BeginGroup(); }
void EndGroup() { ImGui::EndGroup(); }
bool Child(const char* name, const ImVec2& size, bool, ImGuiWindowFlags flags) {
    return lite_widgets::BeginPanel(name, size, flags);
}
void EndChild() { lite_widgets::EndPanel(); }
bool BeginCombo(const char* label, const char* preview, int maxItems, bool, ImGuiComboFlags flags) {
    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const char* labelEnd = ImGui::FindRenderedTextEnd(label);
    if (labelEnd > label) ImGui::TextUnformatted(label,labelEnd);
    const auto start = ImGui::GetCursorScreenPos();
    const float width = ImMin(140.f*lite_widgets::scale,ImGui::GetContentRegionAvail().x);
    const ImVec2 pos(start.x+(ImGui::GetContentRegionAvail().x-width)*.5f,start.y);
    ImGui::SetCursorScreenPos(pos);
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton(label,{width,24.f*lite_widgets::scale}));
    const auto box = GImGui->LastItemData.Rect;
    const bool visible = ImGui::IsItemVisible();
    const auto popup = ImHashStr("##ComboPopup",0,window->GetID(label));
    if (pressed) {
        if (ImGui::IsPopupOpen(popup,ImGuiPopupFlags_None)) ImGui::ClosePopupToLevel(GImGui->BeginPopupStack.Size,true);
        else ImGui::OpenPopupEx(popup,ImGuiPopupFlags_None);
    }
    const bool open = ImGui::IsPopupOpen(popup,ImGuiPopupFlags_None);
    if (visible) {
        auto* draw = window->DrawList;
        const float scale = lite_widgets::scale;
        draw->AddRectFilled(box.Min,box.Max,lite_widgets::Fade(lite_colors::DropdownGradTop),4.f*scale);
        draw->AddRect(box.Min,box.Max,lite_widgets::Fade(lite_colors::ButtonBorder),4.f*scale);
        draw->PushClipRect(pos+ImVec2(6*scale,0),box.Max-ImVec2(18*scale,0),true);
        draw->AddText(pos+ImVec2(8*scale,(box.GetHeight()-ImGui::GetTextLineHeight())*.5f),
            lite_widgets::Fade(lite_colors::Text),preview?preview:"");
        draw->PopClipRect();
        const ImVec2 arrow(box.Max.x-14*scale,pos.y+box.GetHeight()*.5f-scale);
        const float offset = open ? 3.f*scale : 0.f, tip = open ? 0.f : 3.f*scale;
        draw->AddLine(arrow+ImVec2(0,offset),arrow+ImVec2(3*scale,tip),lite_widgets::Fade(lite_colors::TextDim),1.5f*scale);
        draw->AddLine(arrow+ImVec2(6*scale,offset),arrow+ImVec2(3*scale,tip),lite_widgets::Fade(lite_colors::TextDim),1.5f*scale);
    }
    ImGui::SetCursorScreenPos({start.x,start.y+27.f*lite_widgets::scale});
    if (!visible && open) ImGui::ClosePopupToLevel(GImGui->BeginPopupStack.Size,true);
    if (!visible || !open) return false;
    const float height = ImGui::GetTextLineHeightWithSpacing()*ImClamp(maxItems,1,8)+12.f*lite_widgets::scale;
    ImGui::SetNextWindowSizeConstraints({width,0},{width,ImMin(height,ImGui::GetIO().DisplaySize.y-16.f)});
    const auto extent = ImGui::GetPopupAllowedExtentRect(window);
    ImRect avoid = box;
    avoid.Expand(ImVec2(0,2.f*lite_widgets::scale));
    ImGuiDir direction = flags & ImGuiComboFlags_PopupAlignLeft ? ImGuiDir_Left : ImGuiDir_Down;
    const auto popupPos = ImGui::FindBestWindowPosForPopupEx(avoid.GetBL(),{width,height},
        &direction,extent,avoid,ImGuiPopupPositionPolicy_ComboBox);
    ImGui::SetNextWindowPos(popupPos);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(6,6)*lite_widgets::scale);
    const bool shown = ImGui::BeginPopupEx(popup,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PopStyleVar();
    if (shown) lite_widgets::SetFontScale();
    return shown;
}
void EndCombo() { ImGui::EndCombo(); }
bool Combo(const char* label, int* value, const char* (*getter)(void*, int), void* data, int count, int maxHeight) {
    bool changed = false;
    const char* preview = *value >= 0 && *value < count ? getter(data, *value) : "";
    if (BeginCombo(label, preview, maxHeight > 0 ? maxHeight : count, false, 0)) {
        ImGuiListClipper clipper;
        clipper.Begin(count);
        while (clipper.Step()) for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index) {
            ImGui::PushID(index);
            if (custom::Selectable(getter(data,index),*value==index)) { *value=index; changed=true; }
            ImGui::PopID();
        }
        EndCombo();
    }
    return changed;
}
bool Combo(const char* label, int* value, const char* const items[], int count, int maxHeight) {
    return Combo(label,value,[](void* data,int index) { return static_cast<const char* const*>(data)[index]; },
        const_cast<void*>(static_cast<const void*>(items)),count,maxHeight);
}
bool Combo(const char* label, int* value, const char* items, int maxHeight) {
    int count = 0;
    for (const char* at = items; *at; at += std::strlen(at)+1) ++count;
    return Combo(label,value,[](void* data,int index) {
        auto* at = static_cast<const char*>(data);
        for (int item=0;item<index;++item) at += std::strlen(at)+1;
        return at;
    },const_cast<char*>(items),count,maxHeight);
}
bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size, const MenuItemDecoration&) {
    return lite_widgets::Tap(ImGui::Selectable(label, selected, flags, size));
}
bool Selectable(const char* label, bool* value, ImGuiSelectableFlags flags, const ImVec2& size, const MenuItemDecoration& decoration) {
    if (!Selectable(label, *value, flags, size, decoration)) return false;
    *value = !*value;
    return true;
}
bool ColorButton(const char* label, const ImVec4& color, ImGuiColorEditFlags flags, const ImVec2& size) {
    return ImGui::ColorButton(label, color, flags, size);
}
bool ColorEdit4(const char* label, float color[4], ImGuiColorEditFlags flags) { return ImGui::ColorEdit4(label, color, flags); }
void Separator_line() { ImGui::Separator(); }
void Separator() { ImGui::Separator(); }
}
