#pragma once

#include "Call_ImGui.h"
#include "ClayGlass.h"
#include "imgui_settings.h"
#include "../System/Core/SaveConfig.h"
#include "../System/UI/TextureLoader.h"
#include "../System/UI/Logo.h"
#include "../System/UI/SkinTab.h"
#include "../Fonts/Icon.h"

#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>

extern ImFont* F50;
extern ImFont* F107;
extern float menu[4];
extern float SnowBsize;
extern float speedHackMultiplier;
extern float jumpHeightMultiplier;
extern float SlideRange;

namespace font {
    extern ImFont* inter_semibold;
}

namespace runtime_preview_menu {

struct Texture {
    GLuint id = 0;
};

struct StateRefs {
    bool &dark;
    float &tabAlpha;
    float &tabAdd;
    int &page;
    int &activeTab;
    bool &windowCollapsed;
    bool &isMenuVisible;
    double &collapseBarLastActiveTime;
    float &collapseBarOpacityAnim;
    float &collapseBarPressAnim;
};

static bool g_infoPopupRequested = false;
static bool g_priceListPopupRequested = false;
static bool g_popupFocusVisible = false;
static ImVec2 g_popupFocusMin(0.0f, 0.0f);
static ImVec2 g_popupFocusMax(0.0f, 0.0f);

static int g_activeChangelogTab = 0;
static int g_espBoxType = 1;
static int g_espLinePosition = 1;
static int g_espHealthPosition = 1;
static int g_espCrosshairStyle = 0;
static int g_espStyle = 0;

inline std::string ToUpperAscii(std::string text) {
    for (char &ch : text) {
        ch = (char)std::toupper((unsigned char)ch);
    }
    return text;
}

inline std::string BuildExpiryLabel() {
    const std::string countdown = getExpiryCountdown();
    if (!countdown.empty()) {
        return ToUpperAscii("EXPIRY " + countdown);
    }
    if (EXP == "No expiry") {
        return "NO EXPIRY";
    }
    if (!EXP.empty() && EXP != " ") {
        return ToUpperAscii(EXP);
    }
    return "NO EXPIRY";
}

inline std::string BuildFpsLabel() {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "FPS %d", (int)std::lround(ImGui::GetIO().Framerate));
    return buffer;
}

inline void ResetPopupFocusWindow() {
    g_popupFocusVisible = false;
    g_popupFocusMin = ImVec2(0.0f, 0.0f);
    g_popupFocusMax = ImVec2(0.0f, 0.0f);
}

inline void RegisterPopupFocusWindow() {
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 max(min.x + windowSize.x, min.y + windowSize.y);
    if (!g_popupFocusVisible) {
        g_popupFocusVisible = true;
        g_popupFocusMin = min;
        g_popupFocusMax = max;
        return;
    }

    g_popupFocusMin.x = ImMin(g_popupFocusMin.x, min.x);
    g_popupFocusMin.y = ImMin(g_popupFocusMin.y, min.y);
    g_popupFocusMax.x = ImMax(g_popupFocusMax.x, max.x);
    g_popupFocusMax.y = ImMax(g_popupFocusMax.y, max.y);
}

inline void DrawPopupBackdropFocusLayer(ImDrawList *drawList) {
    if (drawList == nullptr || !g_popupFocusVisible) {
        return;
    }

    drawList->PushClipRectFullScreen();
    const ImVec2 viewportMax = ImGui::GetIO().DisplaySize;
    const ImVec2 holeMin(
        ImClamp(g_popupFocusMin.x, 0.0f, viewportMax.x),
        ImClamp(g_popupFocusMin.y, 0.0f, viewportMax.y)
    );
    const ImVec2 holeMax(
        ImClamp(g_popupFocusMax.x, 0.0f, viewportMax.x),
        ImClamp(g_popupFocusMax.y, 0.0f, viewportMax.y)
    );
    const ImU32 dimColor = IM_COL32(8, 0, 14, 172);

    if (holeMin.y > 0.0f) drawList->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(viewportMax.x, holeMin.y), dimColor);
    if (holeMin.x > 0.0f) drawList->AddRectFilled(ImVec2(0.0f, holeMin.y), ImVec2(holeMin.x, holeMax.y), dimColor);
    if (holeMax.x < viewportMax.x) drawList->AddRectFilled(ImVec2(holeMax.x, holeMin.y), ImVec2(viewportMax.x, holeMax.y), dimColor);
    if (holeMax.y < viewportMax.y) drawList->AddRectFilled(ImVec2(0.0f, holeMax.y), viewportMax, dimColor);

    drawList->AddRect(
        holeMin,
        holeMax,
        ImGui::GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.18f)),
        10.0f,
        0,
        1.0f
    );
    drawList->PopClipRect();
}

#include "runtime_preview_helpers.h"
#include "../System/UI/MiscTab.h"
#include "../System/UI/SettingsTab.h"

inline void RenderComboRow(const char *label, const char *const items[], int itemCount, int *value) {
    if (label == nullptr || items == nullptr || value == nullptr || itemCount <= 0) {
        return;
    }

    *value = ImClamp(*value, 0, itemCount - 1);
    custom::Combo(label, value, items, itemCount);
}

inline bool DrawRuntimeEspColorRow(const char *label, float *color) {
    if (label == nullptr || color == nullptr) {
        return false;
    }

    auto normalize = [](float v) {
        return (v <= 1.0f) ? ImClamp(v, 0.0f, 1.0f) : ImClamp(v / 255.0f, 0.0f, 1.0f);
    };

    float col[4] = {
        normalize(color[0]),
        normalize(color[1]),
        normalize(color[2]),
        normalize(color[3])
    };

    const bool changed = custom::ColorEdit4(
        label,
        col,
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_PickerHueWheel
    );

    if (changed) {
        color[0] = col[0] * 255.0f;
        color[1] = col[1] * 255.0f;
        color[2] = col[2] * 255.0f;
        color[3] = 255.0f;
    }

    return changed;
}

inline void SyncEspOptionStateFromConfig(int boxTypeCount, int linePositionCount, int healthPositionCount, int espStyleCount) {
    g_espBoxType = ImClamp((int)Config.ESPMenu.BoxType, 0, ImMax(0, boxTypeCount - 1));
    g_espLinePosition = ImClamp((int)Config.ESPMenu.Target, 0, ImMax(0, linePositionCount - 1));
    g_espHealthPosition = ImClamp((int)Config.ESPMenu.HealthPosition, 0, ImMax(0, healthPositionCount - 1));
    g_espStyle = ImClamp((int)Config.ESPMenu.EspStyle, 0, ImMax(0, espStyleCount - 1));
}

inline void CopyLinkedEspColors(float *lineColor, float *boxColor, float *nameColor, float *healthColor, float *distanceColor, float *skeletonColor) {
    if (lineColor == nullptr) {
        return;
    }

    if (boxColor) std::memcpy(boxColor, lineColor, sizeof(float) * 4);
    if (nameColor) std::memcpy(nameColor, lineColor, sizeof(float) * 4);
    if (healthColor) std::memcpy(healthColor, lineColor, sizeof(float) * 4);
    if (distanceColor) std::memcpy(distanceColor, lineColor, sizeof(float) * 4);
    if (skeletonColor) std::memcpy(skeletonColor, lineColor, sizeof(float) * 4);
}

inline void ApplyEspOptionStateToConfig() {
    Config.ESPMenu.BoxType = (EspBoxType)g_espBoxType;
    Config.ESPMenu.Target = (LineTarget)g_espLinePosition;
    Config.ESPMenu.HealthPosition = (EspHealthPosition)g_espHealthPosition;
    Config.ESPMenu.EspStyle = (EspStyleTarget)g_espStyle;
}

inline void RenderEspTab(float childWidth, float childHeight) {
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("ESP##RUNTIME_ESP", ImVec2(childWidth, childHeight));
        custom::Checkbox("ESP Line", &Config.ESPMenu.isPlayerLine);
        custom::Checkbox("ESP Box", &Config.ESPMenu.Box);
        custom::Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
        custom::Checkbox("ESP Health", &Config.ESPMenu.Health);
        custom::Checkbox("ESP Name", &Config.ESPMenu.Name);
        custom::Checkbox("ESP Distance", &Config.ESPMenu.Distance);
        custom::Checkbox("ESP Count", &Config.ESPMenu.Count);
        custom::Checkbox("360 Alert", &Config.ESPMenu.Alert);
        custom::Checkbox("Show AimLine", &Config.ESPMenu.Aimline);
        custom::Checkbox("Yellow Wallhack", &Config.ExtraMenu.WallHack);
        custom::Checkbox("Red Wallhack", &Config.ExtraMenu.RedWallhack);
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, 10.0f);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("ESP OPTIONS##RUNTIME_ESP_OPTIONS", ImVec2(childWidth, childHeight));
        static const char *boxTypes[] = {"Fill", "Outline", "Corner", "3D"};
        static const char *linePositions[] = {"Top", "Mid", "Bottom"};
        static const char *healthPositions[] = {"Top", "Side"};
        static const char *espStyles[] = {"None", "3D Sphere", "Player Signal"};
        g_espBoxType = ImClamp((int)Config.ESPMenu.BoxType, 0, IM_ARRAYSIZE(boxTypes) - 1);
        g_espLinePosition = ImClamp((int)Config.ESPMenu.Target, 0, IM_ARRAYSIZE(linePositions) - 1);
        g_espHealthPosition = ImClamp((int)Config.ESPMenu.HealthPosition, 0, IM_ARRAYSIZE(healthPositions) - 1);
        g_espStyle = ImClamp((int)Config.ESPMenu.EspStyle, 0, IM_ARRAYSIZE(espStyles) - 1);
        RenderComboRow("Box Type", boxTypes, IM_ARRAYSIZE(boxTypes), &g_espBoxType);
        RenderComboRow("Line Position", linePositions, IM_ARRAYSIZE(linePositions), &g_espLinePosition);
        RenderComboRow("Health Position", healthPositions, IM_ARRAYSIZE(healthPositions), &g_espHealthPosition);
        RenderComboRow("ESP Style", espStyles, IM_ARRAYSIZE(espStyles), &g_espStyle);

        DrawRuntimeEspColorRow("Player ESP Color", Config.sColorsESPPLAYER.LinePLAYER);
        if (Config.sColorsESPPLAYER.BoxPLAYER) memcpy(Config.sColorsESPPLAYER.BoxPLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.NamePLAYER) memcpy(Config.sColorsESPPLAYER.NamePLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.HealthPLAYER) memcpy(Config.sColorsESPPLAYER.HealthPLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.DistancePLAYER) memcpy(Config.sColorsESPPLAYER.DistancePLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.SkeletonPLAYER) memcpy(Config.sColorsESPPLAYER.SkeletonPLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        DrawRuntimeEspColorRow("Bot ESP Color", Config.sColorsESPBOT.LineBOT);
        if (Config.sColorsESPBOT.BoxBOT) memcpy(Config.sColorsESPBOT.BoxBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.NameBOT) memcpy(Config.sColorsESPBOT.NameBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.HealthBOT) memcpy(Config.sColorsESPBOT.HealthBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.DistanceBOT) memcpy(Config.sColorsESPBOT.DistanceBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.SkeletonBOT) memcpy(Config.sColorsESPBOT.SkeletonBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        Config.ESPMenu.BoxType = (EspBoxType)g_espBoxType;
        Config.ESPMenu.Target = (LineTarget)g_espLinePosition;
        Config.ESPMenu.HealthPosition = (EspHealthPosition)g_espHealthPosition;
        Config.ESPMenu.EspStyle = (EspStyleTarget)g_espStyle;
        EndContentChild(right);
    }
    custom::EndGroup();
}

inline void RenderAimTab(float childWidth, float childHeight) {
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("AIMBOT##RUNTIME_AIMBOT", ImVec2(childWidth, childHeight));
        custom::Checkbox("Aimbot 360", &Config.Aim.Aimbot360);
        custom::Checkbox("Bullet Track", &Config.Aim.AimSilent);
        custom::SliderFloat("Aim Assist Size", &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, 10.0f);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("COMBAT OPTIONS##RUNTIME_COMBAT_OPTIONS", ImVec2(childWidth, childHeight));
        static const char *targets[] = {"Head", "Chest", "Body"};
        static const char *triggers[] = {"None", "Shooting", "Scoping"};
        static const char *targetBy[] = {"Distance", "FOV"};
        int target = ImClamp((int)Config.Aim.Target, 0, IM_ARRAYSIZE(targets) - 1);
        int trigger = ImClamp((int)Config.Aim.Trigger, 0, IM_ARRAYSIZE(triggers) - 1);
        int aimBy = ImClamp((int)Config.Aim.By, 0, IM_ARRAYSIZE(targetBy) - 1);
        RenderComboRow("Location", targets, IM_ARRAYSIZE(targets), &target);
        RenderComboRow("Trigger", triggers, IM_ARRAYSIZE(triggers), &trigger);
        RenderComboRow("Target By", targetBy, IM_ARRAYSIZE(targetBy), &aimBy);
        Config.Aim.Target = (EAimTarget)target;
        Config.Aim.Trigger = (EAimTrigger)trigger;
        Config.Aim.By = (EAim)aimBy;
        custom::SliderFloat("FOV Size", &Config.Aim.Cross, 0.0f, 100.0f, "%.0f");
        EndContentChild(right);
    }
    custom::EndGroup();
}

inline void RenderMemoryTab(float childWidth, float childHeight) {
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("MEMORY HACKS##RUNTIME_MEMORY_LEFT", ImVec2(childWidth, childHeight));
        custom::Checkbox("Hitbox", &Config.ExtraMenu.Hit);
        custom::Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
        custom::Checkbox("No Spread", &Config.ExtraMenu.Spread);
        custom::Checkbox("No Shake", &Config.ExtraMenu.Shake);
        custom::Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
        custom::Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
        custom::Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
        custom::Checkbox("Firerate", &Config.ExtraMenu.Fire);
        custom::Checkbox("Fast Dive", &Config.ExtraMenu.Diving);
        custom::Checkbox("Fast Reload", &Config.ExtraMenu.Reload);
        custom::Checkbox("Fast Scope", &Config.ExtraMenu.Scope);
        custom::Checkbox("Quick Switch", &Config.ExtraMenu.Switch);
        custom::Checkbox("Weapon Kinetic", &Config.ExtraMenu.Kinetic);
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, 10.0f);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("MISC FEATURES##RUNTIME_MEMORY_RIGHT", ImVec2(childWidth, childHeight));
        custom::SliderFloat("Snowboard Speed", &SnowBsize, 0.0f, 100.0f, "%.1f");
        custom::SliderFloat("Slide Distance", &SlideRange, 0.0f, 30.0f, "%.1f");
        custom::SliderFloat("SpeedHack", &speedHackMultiplier, 0.5f, 2.0f, "%.1fx");
        custom::SliderFloat("High Jump", &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");
        EndContentChild(right);
    }
    custom::EndGroup();
}

inline void RenderMiscTab(float contentWidth, float contentHeight) {
    const ChildFrame misc = BeginContentChild("MISC##MISC", ImVec2(contentWidth, contentHeight));
    const misc_tab::LayoutMetrics layout = misc_tab::CalculateLayout(contentWidth, g_activeChangelogTab);

    {
        const ChildFrame changelog = BeginContentChild("CHANGELOG##MISC_CHANGELOG", ImVec2(contentWidth, layout.changelogHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const float changelogTabGap = 5.0f;
        const float changelogTabH = 40.0f;
        const float changelogSafeRight = ImGui::GetStyle().ScrollbarSize + 12.0f;
        const float changelogAvailW = ImMax(1.0f, ImGui::GetContentRegionAvail().x - changelogSafeRight);
        const float changelogTabW = ImMax(1.0f, ImFloor((changelogAvailW - changelogTabGap * 2.0f) / 3.0f));
        for (int i = 0; i < IM_ARRAYSIZE(misc_tab::kChangelogTabs); ++i) {
            char buttonId[48] = {};
            std::snprintf(buttonId, sizeof(buttonId), "##changelog_%d", i);
            if (i > 0) {
                ImGui::SameLine(0.0f, changelogTabGap);
            }
            if (misc_tab::DrawChangelogTab(buttonId, misc_tab::kChangelogIcons[i], misc_tab::kChangelogTabs[i], g_activeChangelogTab == i, ImVec2(changelogTabW, changelogTabH))) {
                g_activeChangelogTab = i;
            }
        }

        misc_tab::ContentGap(12.0f);

        const misc_tab::ChangelogSelection activeSection = misc_tab::GetActiveChangelogSelection(g_activeChangelogTab);

        ImFont *iconFont = F107 ? F107 : ImGui::GetFont();
        ImGui::PushFont(iconFont);
        ImGui::TextColored(activeSection.color, "%s", activeSection.icon);
        ImGui::PopFont();
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextColored(activeSection.color, "%s", activeSection.title);
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextColored(c::text::text, "March 10, 2026");
        misc_tab::ContentGap(8.0f);
        misc_tab::DrawSectionItems(activeSection.items, activeSection.count);
        EndContentChild(changelog);
    }

    misc_tab::ContentGap(layout.rowGap);

    {
        const ChildFrame info = BeginContentChild("INFO##MISC_INFO", ImVec2(contentWidth, layout.infoHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        misc_tab::DrawInfoSummary();
        EndContentChild(info);
    }

    misc_tab::ContentGap(layout.rowGap);

    {
        const ChildFrame price = BeginContentChild("PRICELIST##MISC_PRICELIST", ImVec2(contentWidth, layout.priceHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        misc_tab::DrawPriceSummary("##runtime_misc_inline_prices");
        EndContentChild(price);
    }

    EndContentChild(misc);
}

inline void RenderSettingsTab(float contentWidth, float contentHeight) {
    ImGuiStyle &style = ImGui::GetStyle();
    const float columnGap = ImClamp(contentWidth * 0.022f, 12.0f, 18.0f);
    const float rowGap = 6.0f;
    const float leftChildWidth = ImMax(0.0f, ImFloor((contentWidth - columnGap) * 0.5f));
    const float rightChildWidth = ImMax(0.0f, contentWidth - columnGap - leftChildWidth);
    const float topChildHeight = ImMax(0.0f, ImFloor((contentHeight - rowGap) * 0.5f));
    const float bottomChildHeight = ImMax(0.0f, contentHeight - rowGap - topChildHeight);
    const float startX = ImGui::GetCursorPosX();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, rowGap));

    ImGui::SetCursorPosX(startX);
    custom::BeginGroup();
    {
        const ChildFrame licenseChild = BeginContentChild("LICENSE INFO##RUNTIME_LICENSE_INFO", ImVec2(leftChildWidth, topChildHeight));
        settings_tab::RenderLicenseCard();
        EndContentChild(licenseChild);

        const ChildFrame logoChild = BeginContentChild("LOGO SETTINGS##RUNTIME_LOGO_SETTINGS", ImVec2(leftChildWidth, bottomChildHeight));
        settings_tab::RenderLogoCard();
        EndContentChild(logoChild);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, columnGap);

    custom::BeginGroup();
    {
        const ChildFrame configChild = BeginContentChild("CONFIG MANAGEMENT##RUNTIME_CONFIG_MANAGEMENT", ImVec2(rightChildWidth, topChildHeight));
        settings_tab::RenderConfigCard();
        EndContentChild(configChild);

        const ChildFrame enhancementChild = BeginContentChild("ENHANCEMENT##RUNTIME_ENHANCEMENT", ImVec2(rightChildWidth, bottomChildHeight));
        settings_tab::RenderEnhancementCard();
        EndContentChild(enhancementChild);
    }
    custom::EndGroup();

    ImGui::PopStyleVar();
}


inline bool DrawRuntimeSidebarButton(const char *id, const char *icon, const char *label, bool active, const ImVec2 &min, const ImVec2 &size) {
    if (id == nullptr || label == nullptr) {
        return false;
    }

    ImGui::SetCursorScreenPos(min);
    ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const float pillRounding = 14.0f;
    const ImU32 bg = active
        ? IM_COL32(255, 255, 255, 18)
        : hovered ? IM_COL32(255, 255, 255, 8) : IM_COL32(0, 0, 0, 0);
    if (bg != IM_COL32(0, 0, 0, 0)) {
        clay::Surface(drawList, min + ImVec2(5,2), min + size - ImVec2(5,2), pillRounding, active);
    }
    if (active) {
        drawList->AddRect(min + ImVec2(5,2), min + size - ImVec2(5,2),
                          IM_COL32(213,184,255,44), pillRounding, 0, 1.0f);
    }

    ImFont *iconFont = custom::shell::GetIconFont();
    ImFont *textFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    const float iconSize = 18.0f;
    const float textSize = 14.0f;
    const ImU32 iconColor = active ? IM_COL32(248, 248, 252, 255) : IM_COL32(150, 150, 166, hovered ? 240 : 200);
    const ImU32 textColor = active ? IM_COL32(248, 248, 252, 255) : IM_COL32(164, 164, 178, hovered ? 235 : 205);

    if (icon != nullptr && iconFont != nullptr) {
        const ImVec2 is = iconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, icon);
        drawList->AddText(iconFont, iconSize,
            ImVec2(min.x + 15.0f, min.y + (size.y - is.y) * 0.5f), iconColor, icon);
    }

    if (textFont != nullptr) {
        const ImVec2 ts = textFont->CalcTextSizeA(textSize, FLT_MAX, 0.0f, label);
        drawList->AddText(textFont, textSize,
            ImVec2(min.x + 47.0f, min.y + (size.y - ts.y) * 0.5f), textColor, label);
    }

    return clicked;
}

struct EspPreviewState {
    bool line = false;
    bool box = false;
    bool skeleton = false;
    bool health = false;
    bool name = false;
    bool distance = false;
    bool count = false;
    bool alert = false;
    bool brClass = false;
    int boxType = 1;
    int healthPosition = 0;
    int espStyle = 0;
    int lineTarget = 0;
};

inline void DrawRuntimeEspPreviewPanel(const ImVec2 &min, const ImVec2 &size, const EspPreviewState &state = EspPreviewState()) {
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 max = min + size;

    drawList->AddRectFilled(min, max, IM_COL32(16, 16, 21, 240), 18.0f);
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 16), 18.0f, 0, 1.0f);

    ImFont *textFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    drawList->AddText(textFont, 12.5f, ImVec2(min.x + 16.0f, min.y + 14.0f), IM_COL32(240, 238, 248, 245), "ESP PREVIEW");
    drawList->AddCircleFilled(ImVec2(max.x - 18.0f, min.y + 20.0f), 3.0f, IM_COL32(88, 226, 148, 220), 12);

    const ImVec2 viewMin(min.x + 12.0f, min.y + 40.0f);
    const ImVec2 viewMax(max.x - 12.0f, max.y - 12.0f);
    drawList->AddRectFilled(viewMin, viewMax, IM_COL32(11, 11, 15, 242), 14.0f);
    drawList->AddRect(viewMin, viewMax, IM_COL32(255, 255, 255, 12), 14.0f, 0, 1.0f);
    drawList->PushClipRect(viewMin, viewMax, true);

    const float viewH = viewMax.y - viewMin.y;
    const float floorY = viewMax.y - 28.0f;
    drawList->AddRectFilledMultiColor(
        viewMin,
        ImVec2(viewMax.x, floorY),
        IM_COL32(26, 26, 35, 235),
        IM_COL32(26, 26, 35, 235),
        IM_COL32(13, 13, 19, 245),
        IM_COL32(13, 13, 19, 245)
    );
    drawList->AddRectFilled(ImVec2(viewMin.x, floorY), ImVec2(viewMax.x, viewMax.y), IM_COL32(15, 15, 21, 245), 0.0f);
    drawList->AddLine(ImVec2(viewMin.x + 6.0f, floorY), ImVec2(viewMax.x - 6.0f, floorY), IM_COL32(150, 145, 175, 40), 1.0f);

    const float cx = (viewMin.x + viewMax.x) * 0.5f;
    const float modelH = ImMin(viewH * 0.62f, 250.0f);
    const float feetY = floorY;
    const float headTop = feetY - modelH;
    const float headR = modelH * 0.072f;
    const float shoulderY = headTop + headR * 2.7f;
    const float hipY = headTop + modelH * 0.54f;
    const float kneeY = headTop + modelH * 0.79f;
    const float torsoTop = modelH * 0.115f;
    const float torsoBottom = modelH * 0.082f;
    const float legHalf = modelH * 0.045f;

    const ImU32 body = IM_COL32(56, 52, 68, 255);
    const ImU32 bodyDark = IM_COL32(41, 38, 51, 255);
    const ImU32 bodyLight = IM_COL32(70, 65, 84, 255);
    const ImU32 rim = IM_COL32(168, 158, 192, 130);

    auto fill = [&](const ImVec2 *pts, int count, ImU32 col) {
        drawList->AddConvexPolyFilled(pts, count, col);
    };

    {
        const ImVec2 pack[] = {
            ImVec2(cx - torsoTop * 1.18f, shoulderY + 6.0f),
            ImVec2(cx - torsoTop * 0.72f, shoulderY + 2.0f),
            ImVec2(cx - torsoTop * 0.55f, hipY - 8.0f),
            ImVec2(cx - torsoTop * 1.05f, hipY - 12.0f)
        };
        fill(pack, 4, bodyDark);
    }

    {
        const ImVec2 armL[] = {
            ImVec2(cx - torsoTop * 0.94f, shoulderY + 3.0f),
            ImVec2(cx - torsoTop * 1.72f, shoulderY + 18.0f),
            ImVec2(cx - torsoTop * 1.44f, hipY - 4.0f),
            ImVec2(cx - torsoTop * 0.80f, hipY - 12.0f)
        };
        fill(armL, 4, body);
        const ImVec2 armR[] = {
            ImVec2(cx + torsoTop * 0.94f, shoulderY + 3.0f),
            ImVec2(cx + torsoTop * 1.66f, shoulderY + 20.0f),
            ImVec2(cx + torsoTop * 1.40f, hipY - 2.0f),
            ImVec2(cx + torsoTop * 0.80f, hipY - 12.0f)
        };
        fill(armR, 4, body);
    }

    {
        const ImVec2 torso[] = {
            ImVec2(cx - torsoTop, shoulderY),
            ImVec2(cx + torsoTop, shoulderY),
            ImVec2(cx + torsoBottom, hipY),
            ImVec2(cx - torsoBottom, hipY)
        };
        fill(torso, 4, body);
        const ImVec2 plate[] = {
            ImVec2(cx - torsoTop * 0.74f, shoulderY + modelH * 0.035f),
            ImVec2(cx + torsoTop * 0.74f, shoulderY + modelH * 0.035f),
            ImVec2(cx + torsoBottom * 0.86f, hipY - modelH * 0.04f),
            ImVec2(cx - torsoBottom * 0.86f, hipY - modelH * 0.04f)
        };
        fill(plate, 4, bodyLight);
        drawList->AddLine(ImVec2(cx - torsoTop * 0.9f, shoulderY + 2.0f), ImVec2(cx - torsoBottom * 0.9f, hipY), rim, 1.0f);
        drawList->AddLine(ImVec2(cx - torsoBottom, hipY), ImVec2(cx + torsoBottom, hipY), bodyDark, 4.0f);
    }

    {
        const ImVec2 legL[] = {
            ImVec2(cx - torsoBottom, hipY),
            ImVec2(cx - 1.0f, hipY),
            ImVec2(cx - legHalf * 0.6f, kneeY),
            ImVec2(cx - legHalf, feetY),
            ImVec2(cx - legHalf * 2.4f, feetY),
            ImVec2(cx - legHalf * 1.5f, kneeY)
        };
        fill(legL, 6, body);
        const ImVec2 legR[] = {
            ImVec2(cx + 1.0f, hipY),
            ImVec2(cx + torsoBottom, hipY),
            ImVec2(cx + legHalf * 1.5f, kneeY),
            ImVec2(cx + legHalf * 2.4f, feetY),
            ImVec2(cx + legHalf, feetY),
            ImVec2(cx + legHalf * 0.6f, kneeY)
        };
        fill(legR, 6, body);
        drawList->AddLine(ImVec2(cx - legHalf * 1.5f, kneeY), ImVec2(cx - legHalf * 2.4f, feetY), bodyDark, 2.0f);
        drawList->AddLine(ImVec2(cx + legHalf * 1.5f, kneeY), ImVec2(cx + legHalf * 2.4f, feetY), bodyDark, 2.0f);
    }

    {
        const ImVec2 headCenter(cx, headTop + headR * 1.6f);
        drawList->AddCircleFilled(headCenter, headR, body, 24);
        drawList->AddCircleFilled(ImVec2(cx, headTop + headR * 1.25f), headR * 1.04f, bodyDark, 24);
        drawList->AddLine(
            ImVec2(cx - headR * 0.85f, headTop + headR * 1.75f),
            ImVec2(cx + headR * 0.85f, headTop + headR * 1.75f),
            IM_COL32(177, 110, 245, 220),
            2.0f
        );
        drawList->AddLine(
            ImVec2(cx - headR * 1.05f, headTop + headR * 1.5f),
            ImVec2(cx - headR * 1.05f, headTop + headR * 2.6f),
            rim,
            1.0f
        );
    }

    {
        const ImVec2 rifleA(cx + torsoTop * 0.55f, shoulderY + modelH * 0.02f);
        const ImVec2 rifleB(cx - torsoTop * 1.15f, hipY + modelH * 0.03f);
        drawList->AddLine(rifleA, rifleB, IM_COL32(30, 28, 38, 255), 4.0f);
        drawList->AddLine(rifleA, rifleB, IM_COL32(90, 84, 108, 200), 1.4f);
        drawList->AddRectFilled(
            ImVec2(rifleB.x - 3.0f, rifleB.y - 4.0f),
            ImVec2(rifleB.x + 8.0f, rifleB.y + 2.0f),
            IM_COL32(30, 28, 38, 255),
            1.5f
        );
    }

    const float boxHalf = modelH * 0.17f;
    const ImVec2 bMin(cx - boxHalf, headTop - 6.0f);
    const ImVec2 bMax(cx + boxHalf, feetY + 4.0f);
    const ImU32 enemy = IM_COL32(255, 72, 72, 255);
    const ImU32 healthGreen = IM_COL32(84, 224, 140, 255);

    if (state.line) {
        const float lineStartY = viewMax.y - 8.0f;
        const float lineEndY = state.lineTarget == 0 ? bMin.y : (state.lineTarget == 1 ? (bMin.y + bMax.y) * 0.5f : bMax.y);
        drawList->AddLine(ImVec2(cx, lineStartY), ImVec2(cx, lineEndY), enemy, 1.4f);
        drawList->AddCircleFilled(ImVec2(cx, lineStartY), 2.0f, enemy, 10);
    }

    if (state.espStyle == 1) {
        const float ellipseY = (bMin.y + bMax.y) * 0.5f;
        const float rx = boxHalf * 1.6f;
        const float ry = boxHalf * 0.42f;
        ImVec2 pts[40];
        for (int i = 0; i < 40; ++i) {
            const float a = (float)i / 40.0f * 6.2831853f;
            pts[i] = ImVec2(cx + std::cos(a) * rx, ellipseY + std::sin(a) * ry);
        }
        drawList->AddPolyline(pts, 40, IM_COL32(177, 110, 245, 110), ImDrawFlags_Closed, 1.2f);
    } else if (state.espStyle == 2) {
        const float t = (float)ImGui::GetTime();
        for (int i = 0; i < 2; ++i) {
            const float phase = fmodf(t * 0.8f + i * 0.5f, 1.0f);
            const float r = boxHalf * (0.5f + phase * 1.4f);
            const ImU32 col = ImGui::GetColorU32(ImVec4(0.69f, 0.43f, 0.96f, (1.0f - phase) * 0.35f));
            drawList->AddCircle(ImVec2(cx, feetY), r, col, 24, 1.2f);
        }
    }

    if (state.box) {
        if (state.boxType == 0) {
            drawList->AddRectFilled(bMin, bMax, IM_COL32(255, 72, 72, 28), 2.0f);
            drawList->AddRect(bMin, bMax, enemy, 2.0f, 0, 1.4f);
        } else if (state.boxType == 1) {
            drawList->AddRect(bMin, bMax, enemy, 1.0f, 0, 1.5f);
        } else if (state.boxType == 2) {
            const float len = ImMin((bMax.x - bMin.x) * 0.34f, (bMax.y - bMin.y) * 0.22f);
            drawList->AddLine(ImVec2(bMin.x, bMin.y), ImVec2(bMin.x + len, bMin.y), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMin.x, bMin.y), ImVec2(bMin.x, bMin.y + len), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMax.x - len, bMin.y), ImVec2(bMax.x, bMin.y), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMax.x, bMin.y), ImVec2(bMax.x, bMin.y + len), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMin.x, bMax.y - len), ImVec2(bMin.x, bMax.y), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMin.x, bMax.y), ImVec2(bMin.x + len, bMax.y), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMax.x - len, bMax.y), ImVec2(bMax.x, bMax.y), enemy, 1.8f);
            drawList->AddLine(ImVec2(bMax.x, bMax.y - len), ImVec2(bMax.x, bMax.y), enemy, 1.8f);
        } else {
            const ImVec2 off(6.0f, -6.0f);
            drawList->AddRect(ImVec2(bMin.x + off.x, bMin.y + off.y), ImVec2(bMax.x + off.x, bMax.y + off.y), IM_COL32(255, 72, 72, 120), 1.0f, 0, 1.2f);
            drawList->AddRect(bMin, bMax, enemy, 1.0f, 0, 1.5f);
            drawList->AddLine(bMin, ImVec2(bMin.x + off.x, bMin.y + off.y), enemy, 1.0f);
            drawList->AddLine(ImVec2(bMax.x, bMin.y), ImVec2(bMax.x + off.x, bMin.y + off.y), enemy, 1.0f);
            drawList->AddLine(ImVec2(bMin.x, bMax.y), ImVec2(bMin.x + off.x, bMax.y + off.y), enemy, 1.0f);
            drawList->AddLine(bMax, ImVec2(bMax.x + off.x, bMax.y + off.y), enemy, 1.0f);
        }
    }

    if (state.skeleton) {
        const ImU32 bone = IM_COL32(255, 190, 190, 170);
        const ImVec2 neck(cx, shoulderY - 2.0f);
        const ImVec2 head(cx, headTop + headR * 1.6f);
        const ImVec2 hip(cx, hipY);
        drawList->AddLine(head, neck, bone, 1.4f);
        drawList->AddLine(ImVec2(cx - torsoTop * 0.92f, shoulderY + 4.0f), ImVec2(cx + torsoTop * 0.92f, shoulderY + 4.0f), bone, 1.4f);
        drawList->AddLine(ImVec2(cx - torsoTop * 0.92f, shoulderY + 4.0f), ImVec2(cx - torsoTop * 1.5f, hipY - 6.0f), bone, 1.4f);
        drawList->AddLine(ImVec2(cx + torsoTop * 0.92f, shoulderY + 4.0f), ImVec2(cx + torsoTop * 1.45f, hipY - 4.0f), bone, 1.4f);
        drawList->AddLine(neck, hip, bone, 1.4f);
        drawList->AddLine(hip, ImVec2(cx - legHalf, kneeY), bone, 1.4f);
        drawList->AddLine(hip, ImVec2(cx + legHalf, kneeY), bone, 1.4f);
        drawList->AddLine(ImVec2(cx - legHalf, kneeY), ImVec2(cx - legHalf * 1.6f, feetY), bone, 1.4f);
        drawList->AddLine(ImVec2(cx + legHalf, kneeY), ImVec2(cx + legHalf * 1.6f, feetY), bone, 1.4f);
    }

    if (state.health) {
        const float ratio = 0.68f;
        if (state.healthPosition == 0) {
            const ImVec2 hMin(bMin.x, bMin.y - 9.0f);
            const ImVec2 hMax(bMax.x, bMin.y - 5.0f);
            drawList->AddRectFilled(hMin, hMax, IM_COL32(30, 30, 38, 235), 2.0f);
            drawList->AddRectFilled(hMin, ImVec2(hMin.x + (hMax.x - hMin.x) * ratio, hMax.y), healthGreen, 2.0f);
        } else {
            const ImVec2 hMin(bMin.x - 9.0f, bMin.y);
            const ImVec2 hMax(bMin.x - 5.0f, bMax.y);
            drawList->AddRectFilled(hMin, hMax, IM_COL32(30, 30, 38, 235), 2.0f);
            const float filled = (hMax.y - hMin.y) * ratio;
            drawList->AddRectFilled(ImVec2(hMin.x, hMax.y - filled), hMax, healthGreen, 2.0f);
        }
    }

    if (state.name) {
        const char *label = "Enemy";
        const ImVec2 labelSz = textFont->CalcTextSizeA(11.0f, FLT_MAX, 0.0f, label);
        drawList->AddText(textFont, 11.0f, ImVec2(cx - labelSz.x * 0.5f, bMin.y - 24.0f), enemy, label);
    }

    if (state.brClass) {
        const char *label = "ASSAULT";
        const ImVec2 labelSz = textFont->CalcTextSizeA(8.5f, FLT_MAX, 0.0f, label);
        drawList->AddText(
            textFont,
            8.5f,
            ImVec2(cx - labelSz.x * 0.5f, bMin.y - (state.name ? 38.0f : 24.0f)),
            IM_COL32(196, 170, 250, 210),
            label
        );
    }

    if (state.distance) {
        const char *label = "45m";
        const ImVec2 labelSz = textFont->CalcTextSizeA(10.0f, FLT_MAX, 0.0f, label);
        drawList->AddText(textFont, 10.0f, ImVec2(cx - labelSz.x * 0.5f, bMax.y + 5.0f), enemy, label);
    }

    if (state.count) {
        const ImVec2 badgeMin(viewMin.x + 10.0f, viewMin.y + 10.0f);
        const ImVec2 badgeMax(badgeMin.x + 30.0f, badgeMin.y + 20.0f);
        drawList->AddRectFilled(badgeMin, badgeMax, IM_COL32(255, 255, 255, 10), 6.0f);
        drawList->AddRect(badgeMin, badgeMax, IM_COL32(255, 255, 255, 22), 6.0f, 0, 1.0f);
        const char *label = "1 TARGET";
        const ImVec2 labelSz = textFont->CalcTextSizeA(8.5f, FLT_MAX, 0.0f, label);
        drawList->AddText(textFont, 8.5f, ImVec2(badgeMin.x + (30.0f - labelSz.x) * 0.5f, badgeMin.y + (20.0f - labelSz.y) * 0.5f), IM_COL32(232, 228, 244, 225), label);
    }

    if (state.alert) {
        const ImVec2 center(viewMax.x - 26.0f, viewMax.y - 26.0f);
        const float r = 13.0f;
        const float t = (float)ImGui::GetTime();
        for (int i = 0; i < 16; ++i) {
            const float a0 = t * 1.6f + (float)i / 16.0f * 6.2831853f;
            const float a1 = a0 + 0.28f;
            drawList->AddLine(
                ImVec2(center.x + std::cos(a0) * r, center.y + std::sin(a0) * r),
                ImVec2(center.x + std::cos(a1) * r, center.y + std::sin(a1) * r),
                IM_COL32(255, 86, 96, (int)(90 + 120 * ((float)i / 16.0f))),
                1.4f
            );
        }
        const char *label = "360";
        const ImVec2 labelSz = textFont->CalcTextSizeA(8.0f, FLT_MAX, 0.0f, label);
        drawList->AddText(textFont, 8.0f, ImVec2(center.x - labelSz.x * 0.5f, center.y - labelSz.y * 0.5f), IM_COL32(255, 120, 128, 235), label);
    }

    drawList->PopClipRect();
}

inline void CollapseMenu(StateRefs &state) {
    state.windowCollapsed = true;
    state.isMenuVisible = false;
    state.collapseBarLastActiveTime = ImGui::GetTime();
    state.collapseBarOpacityAnim = 1.0f;
    state.collapseBarPressAnim = 0.0f;
}

} // namespace runtime_preview_menu
