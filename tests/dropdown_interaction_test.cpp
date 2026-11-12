#define IMGUI_DEFINE_MATH_OPERATORS
#include "../src/main/jni/ImGui/custom_widgets.hpp"
#include "../src/main/jni/ImGui/imgui_settings.h"
#include "../src/main/jni/BlueprintTierFilter.h"
#include <array>
#include <cassert>
#include <cstdio>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = {.69f, .43f, .96f, 1.f};
namespace font {
ImFont* inter_semibold = nullptr;
ImFont* inter_title = nullptr;
ImFont* icomoon_page = nullptr;
}

namespace {
enum class Kind { Tiers, Single, LongList };
struct Rect {
    ImVec2 min{}, max{};
    ImVec2 Center() const { return (min + max) * .5f; }
};
Rect field, popup, popupClip;
std::array<Rect, 24> rows;
Kind kind = Kind::Tiers;
bool nearBottom = false, opened = false;
int selected = 0;
float scrollMax = 0.f, scroll = 0.f;

Rect LastRect() { return {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()}; }

void Frame() {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({20,20});
    ImGui::SetNextWindowSize({760,560});
    ImGui::Begin("Dropdown interaction", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetCursorScreenPos({55,nearBottom ? 505.f : 75.f});
    ImGui::PushItemWidth(350.f);
    opened = custom::BeginCombo("Options", "Current selection",
        kind == Kind::LongList ? 24 : kind == Kind::Tiers ? 5 : 3);
    if (!opened) field = {GImGui->LastItemData.NavRect.Min, GImGui->LastItemData.NavRect.Max};
    if (opened) {
        popup = {ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize()};
        popupClip = {ImGui::GetCurrentWindow()->ClipRect.Min, ImGui::GetCurrentWindow()->ClipRect.Max};
        scrollMax = ImGui::GetScrollMaxY();
        scroll = ImGui::GetScrollY();
        if (kind == Kind::Tiers) {
            using namespace BlueprintTierFilter;
            const char* labels[] = {"All blueprints", "Mythic", "Legendary", "Epic", "Common"};
            const uint32_t bits[] = {0, Mythic, Legendary, Epic, Common};
            const ImU32 colors[] = {IM_COL32(185,182,198,255),IM_COL32(239,77,91,255),
                IM_COL32(255,208,70,255),IM_COL32(181,119,245,255),IM_COL32(74,222,128,255)};
            for (int i = 0; i < 5; ++i) {
                bool isSelected = i == 0 ? GetMask() == 0 : HasSelection(bits[i]);
                const auto decoration = i == 0 ? custom::MenuItemDecoration::Ring(colors[i])
                    : custom::MenuItemDecoration::Circle(colors[i]);
                if (custom::Selectable(labels[i], &isSelected,
                    ImGuiSelectableFlags_DontClosePopups, {}, decoration)) {
                    if (i == 0) SetMask(0); else Toggle(bits[i]);
                }
                rows[i] = LastRect();
            }
        } else {
            const int count = kind == Kind::LongList ? 24 : 3;
            for (int i = 0; i < count; ++i) {
                char label[40];
                std::snprintf(label, sizeof(label), "Option %02d", i);
                const char* text = kind == Kind::Single && i == 1
                    ? "A deliberately long selected option that must remain clipped inside the popover and leave room for its checkmark even on a narrow screen with a very long character display name"
                    : label;
                if (custom::Selectable(text, selected == i,
                    kind == Kind::Single && i == 2 ? ImGuiSelectableFlags_Disabled : 0))
                    selected = i;
                rows[i] = LastRect();
            }
        }
        custom::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::End();
    ImGui::Render();
}

void Settle() { for (int i = 0; i < 4; ++i) Frame(); }
void Click(ImVec2 point) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(point.x,point.y); Frame();
    io.AddMouseButtonEvent(0,true); Frame();
    io.AddMouseButtonEvent(0,false); Frame();
    Settle();
}
void Open() { Settle(); Click(field.Center()); assert(opened); }
void CloseOutside() { Click({780,590}); assert(!opened); }
}

int main() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {800,600};
    io.DeltaTime = 1.f / 60.f;
    unsigned char* pixels; int width,height;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    c::ApplyTheme(); c::scale = 1.f; c::widget_scale = 1.f;

    BlueprintTierFilter::SetMask(BlueprintTierFilter::Epic);
    Open();
    Click(rows[1].Center());
    assert(opened && BlueprintTierFilter::GetMask() ==
        (BlueprintTierFilter::Mythic | BlueprintTierFilter::Epic));
    Click(rows[0].Center());
    assert(opened && BlueprintTierFilter::GetMask() == 0);
    CloseOutside();

    kind = Kind::Single; selected = 0;
    Open();
    assert(rows[1].max.x <= popup.max.x);
    Click(rows[2].Center());
    assert(opened && selected == 0); // Disabled row must not select or dismiss.
    Click(rows[1].Center());
    assert(!opened && selected == 1);
    Open();
    assert(rows[1].max.x <= popup.max.x); // Selected long label reserves the check lane.
    CloseOutside();

    kind = Kind::LongList; nearBottom = true; selected = 0;
    Open(); Settle();
    assert(popup.min.y >= 0.f && popup.max.y <= io.DisplaySize.y);
    assert(popup.max.y <= field.min.y && scrollMax > 0.f);
    io.AddMousePosEvent(popup.Center().x,popup.Center().y);
    Frame();
    // This ImGui fork eases scrolling; allow repeated input and settling frames.
    for (int i = 0; i < 32 && rows[23].max.y > popupClip.max.y; ++i) {
        io.AddMouseWheelEvent(0,-1.f); Frame(); Settle();
    }
    assert(scroll > 0.f);
    assert(rows[23].min.y >= popupClip.min.y && rows[23].max.y <= popupClip.max.y);
    Click(rows[23].Center());
    assert(!opened && selected == 23);

    ImGui::DestroyContext();
    std::puts("Dropdown multi-select, clear, single-select, disabled, outside-close and scrolling passed");
}
