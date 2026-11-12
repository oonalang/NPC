#ifndef IMGUI_SETTINGS_H
#define IMGUI_SETTINGS_H

#include "imgui.h"
#include "../System/Texture/box_shadow.h"

extern float menu[4];
extern ImFont* F50;
extern ImFont* F107;

namespace font
{
    extern ImFont* inter_semibold;
}

namespace c
{
    inline float scale = 1.5f;
    inline float widget_scale = 1.0f;
    inline ImVec4 accent = ImColor(177, 110, 245);
    inline ImVec4 separator = ImColor(34, 34, 42);

    namespace bg
    {
        inline ImVec4 background = ImColor(8, 8, 11);
        inline ImVec2 size = ImVec2(450, 370);
        inline float rounding = 6.f;
    }

    namespace child
    {
        inline ImVec4 background = ImColor(16, 16, 21);
        inline ImVec4 cap = ImColor(22, 22, 28);
        inline float rounding = 18.f;
        inline float padding = 13.f;
        inline float spacing = 13.f;
    }

    namespace page
    {
        inline ImVec4 background_active = ImColor(32, 30, 42);
        inline ImVec4 background = ImColor(19, 19, 24);

        inline ImVec4 text_hov = ImColor(238, 238, 244);
        inline ImVec4 text = ImColor(140, 140, 152);

        inline float rounding = 4.f;
    }

    namespace elements
    {
        inline ImVec4 background_hovered = ImColor(30, 30, 38);
        inline ImVec4 background = ImColor(19, 19, 24);
        inline float rounding = 4.f;
    }

    namespace checkbox
    {
        inline ImVec4 mark = ImColor(8, 8, 11);
        inline ImVec4 background_on = ImColor(177, 110, 245);
        inline ImVec4 background_off = ImColor(46, 46, 55);
        inline ImVec4 circle_inactive = ImColor(140, 140, 152);
        inline float rounding = 4.f;
    }

    namespace text
    {
        inline ImVec4 text_active = ImColor(244, 244, 248);
        inline ImVec4 text_hov = ImColor(198, 198, 210);
        inline ImVec4 text = ImColor(140, 140, 152);
    }

    namespace widget
    {
        inline ImVec2 size = ImVec2(0, 34.f);
        inline ImVec4 background = ImColor(20, 20, 26);
        inline ImVec4 outlinecolor = ImColor(58, 58, 70);
        inline float rounding = 4.f;
        inline float outline = 1.f;
    }

    namespace button
    {
        inline ImVec4 background = ImColor(20, 20, 26);
        inline ImVec4 background_hovered = ImColor(34, 34, 42);
        inline ImVec4 background_active = ImColor(28, 28, 36);
        inline ImVec4 outline = ImColor(70, 70, 84);
        inline float rounding = 4.f;
    }

    namespace scrollbar
    {
        inline float hitbox_area = 24.f;
        inline float hitbox_extra = 24.f;
        inline bool left_side = false;
        inline float gutter_spacing = 4.f;
    }

    inline void ApplyMainWindowStyle(ImGuiStyle& style)
    {
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.ItemSpacing = ImVec2(10.0f * scale, 10.0f * scale);
        style.WindowBorderSize = 0.0f;
        style.ScrollbarSize = 8.0f * scale;
    }

    inline float MainTopAreaHeight()
    {
        return 40.0f * scale;
    }

    inline void UpdateTheme(bool dark_mode, const float* accent_rgba, float dt)
    {
        const ImVec4 accent_target = accent_rgba
            ? ImVec4(accent_rgba[0], accent_rgba[1], accent_rgba[2], 1.0f)
            : ImColor(177, 110, 245).Value;
        accent = ImLerp(accent, accent_target, dt * 12.0f);

        bg::background = ImLerp(bg::background, dark_mode ? ImColor(8, 8, 11) : ImColor(246, 246, 249), dt * 12.0f);
        separator = ImLerp(separator, dark_mode ? ImColor(34, 34, 42) : ImColor(206, 206, 216), dt * 12.0f);

        elements::background_hovered = ImLerp(elements::background_hovered, dark_mode ? ImColor(30, 30, 38) : ImColor(226, 226, 233), dt * 25.0f);
        elements::background = ImLerp(elements::background, dark_mode ? ImColor(19, 19, 24) : ImColor(238, 238, 243), dt * 25.0f);

        widget::background = ImLerp(widget::background, dark_mode ? ImColor(20, 20, 26) : ImColor(238, 238, 243), dt * 25.0f);
        widget::outlinecolor = ImLerp(widget::outlinecolor, dark_mode ? ImColor(58, 58, 70) : ImColor(196, 196, 208), dt * 25.0f);
        button::background = ImLerp(button::background, dark_mode ? ImColor(20, 20, 26) : ImColor(238, 238, 243), dt * 25.0f);
        button::background_hovered = ImLerp(button::background_hovered, dark_mode ? ImColor(34, 34, 42) : ImColor(224, 224, 232), dt * 25.0f);
        button::background_active = ImLerp(button::background_active, dark_mode ? ImColor(28, 28, 36) : ImColor(212, 212, 222), dt * 25.0f);
        button::outline = ImLerp(button::outline, dark_mode ? ImColor(70, 70, 84) : ImColor(186, 186, 200), dt * 25.0f);

        checkbox::mark = ImLerp(checkbox::mark, dark_mode ? ImColor(8, 8, 11) : ImColor(255, 255, 255), dt * 12.0f);
        checkbox::background_off = ImLerp(checkbox::background_off, dark_mode ? ImColor(46, 46, 55) : ImColor(208, 208, 218), dt * 25.0f);
        checkbox::circle_inactive = ImLerp(checkbox::circle_inactive, dark_mode ? ImColor(140, 140, 152) : ImColor(120, 120, 134), dt * 25.0f);

        child::background = ImLerp(child::background, dark_mode ? ImColor(16, 16, 21) : ImColor(240, 240, 245), dt * 12.0f);
        child::cap = ImLerp(child::cap, dark_mode ? ImColor(22, 22, 28) : ImColor(230, 230, 236), dt * 12.0f);
        child::padding = 13.0f;
        child::spacing = 13.0f;

        page::text_hov = ImLerp(page::text_hov, dark_mode ? ImColor(238, 238, 244) : ImColor(24, 24, 30), dt * 12.0f);
        page::text = ImLerp(page::text, dark_mode ? ImColor(140, 140, 152) : ImColor(120, 120, 134), dt * 12.0f);
        page::background_active = ImLerp(page::background_active, dark_mode ? ImColor(32, 30, 42) : ImColor(222, 218, 236), dt * 25.0f);
        page::background = ImLerp(page::background, dark_mode ? ImColor(19, 19, 24) : ImColor(238, 238, 243), dt * 25.0f);

        text::text_active = ImLerp(text::text_active, dark_mode ? ImColor(244, 244, 248) : ImColor(18, 18, 24), dt * 12.0f);
        text::text_hov = ImLerp(text::text_hov, dark_mode ? ImColor(198, 198, 210) : ImColor(64, 64, 76), dt * 12.0f);
        text::text = ImLerp(text::text, dark_mode ? ImColor(140, 140, 152) : ImColor(108, 108, 122), dt * 12.0f);
    }

    inline void ApplyTheme()
    {
        accent = ImColor(177, 110, 245);
        separator = ImColor(34, 34, 42);

        bg::background = ImColor(8, 8, 11, 232);
        child::background = ImColor(16, 16, 21, 224);
        child::cap = ImColor(22, 22, 28, 232);
        child::padding = 13.0f;
        child::spacing = 13.0f;

        page::background_active = ImColor(32, 30, 42, 255);
        page::background = ImColor(19, 19, 24, 210);
        page::text_hov = ImColor(244, 244, 248);
        page::text = ImColor(140, 140, 152);

        elements::background_hovered = ImColor(30, 30, 38, 214);
        elements::background = ImColor(19, 19, 24, 190);

        checkbox::mark = ImColor(8, 8, 11);
        checkbox::background_on = ImColor(177, 110, 245);
        checkbox::background_off = ImColor(46, 46, 55);
        checkbox::circle_inactive = ImColor(140, 140, 152);

        text::text_active = ImColor(244, 244, 248);
        text::text_hov = ImColor(198, 198, 210);
        text::text = ImColor(140, 140, 152);

        widget::background = ImColor(20, 20, 26, 210);
        widget::outlinecolor = ImColor(58, 58, 70, 190);

        button::background = ImColor(20, 20, 26, 210);
        button::background_hovered = ImColor(34, 34, 42, 224);
        button::background_active = ImColor(28, 28, 36, 240);
        button::outline = ImColor(70, 70, 84, 190);

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScrollbarSize = 4.0f;
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.16f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.26f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.69f, 0.43f, 0.96f, 0.9f);
    }

    inline void DrawWindowShadow(const ImVec2& menuSize)
    {
        RectangleShadowSettings shadowSettings;
        shadowSettings.rectPos = ImVec2(0.0f, 0.0f);
        shadowSettings.rectSize = menuSize;
        shadowSettings.sigma = 14.0f;
        shadowSettings.padding = ImVec2(0.0f, 0.0f);
        shadowSettings.rings = 5;
        shadowSettings.spacingBetweenRings = 2;
        shadowSettings.samplesPerCornerSide = 2;
        // Warm dark shadow
        shadowSettings.shadowColor = ImGui::ColorConvertU32ToFloat4(IM_COL32(8, 12, 16, 200));
        shadowSettings.shadowSize = ImVec2(0.0f, 0.0f);
        drawRectangleShadowVerticesAdaptive(shadowSettings);
    }

    inline void DrawMenuBackdrop(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding, ImTextureID backgroundTexture = nullptr)
    {
        if (drawList == nullptr) {
            return;
        }

        if (backgroundTexture != nullptr) {
            drawList->AddImageRounded(
                backgroundTexture,
                min,
                max,
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 1.0f),
                IM_COL32(255, 255, 255, 255),
                rounding
            );
        } else {
            // Warm slate background
            drawList->AddRectFilled(min, max, IM_COL32(15, 20, 24, 255), rounding);
        }

        // Subtle warm tint overlay
        drawList->AddRectFilled(min, max, IM_COL32(20, 14, 8, 40), rounding);
        // Warm amber border
        drawList->AddRect(min, max, IM_COL32(255, 255, 255, 40), rounding, 0, 1.0f);
    }
}

namespace main_runtime_theme
{
    inline float g_menuHue = 0.75f;

    // ============================================================
    // THEME SYSTEM
    // ============================================================
    enum ThemeID {
        THEME_DARK_SLATE  = 0,  // default — deep slate + amber
        THEME_MIDNIGHT    = 1,  // near-black + cool blue
        THEME_FOREST      = 2,  // dark green tones
        THEME_BLOOD       = 3,  // crimson dark
        THEME_LIGHT       = 4,  // light warm grey
        THEME_COUNT       = 5
    };
    inline int g_activeTheme = THEME_DARK_SLATE;

    struct ThemePalette {
        const char* name;
        // Backgrounds
        ImU32 bgWindow;       // main menu bg
        ImU32 bgChild;        // child panel
        ImU32 bgCap;          // header/cap
        ImU32 bgContent;      // content host
        ImU32 bgTabBar;       // tab bar
        ImU32 bgStatusBar;    // bottom bar
        // Text
        ImU32 textActive;
        ImU32 textMuted;
        // Borders
        ImU32 border;
        // Accent hue (HSV hue 0-1)
        float accentHue;
        float accentSat;
        float accentVal;
    };

    inline ThemePalette g_themes[THEME_COUNT] = {
        {
            "SLATE",
            IM_COL32(8, 8, 11, 232),
            IM_COL32(16, 16, 21, 224),
            IM_COL32(22, 22, 28, 232),
            IM_COL32(11, 11, 15, 198),
            IM_COL32(12, 12, 16, 220),
            IM_COL32(12, 12, 16, 214),
            IM_COL32(244, 244, 248, 255),
            IM_COL32(140, 140, 152, 190),
            IM_COL32(40, 40, 50, 180),
            0.75f, 0.55f, 0.96f,
        },
        {
            "NIGHT",
            IM_COL32(5, 5, 9, 238),
            IM_COL32(12, 12, 18, 226),
            IM_COL32(16, 16, 24, 234),
            IM_COL32(8, 8, 13, 202),
            IM_COL32(9, 9, 14, 222),
            IM_COL32(9, 9, 14, 216),
            IM_COL32(238, 238, 246, 255),
            IM_COL32(126, 128, 148, 190),
            IM_COL32(34, 34, 46, 180),
            0.75f, 0.55f, 0.96f,
        },
        {
            "FOREST",
            IM_COL32(9, 11, 10, 236),
            IM_COL32(16, 19, 18, 226),
            IM_COL32(21, 25, 23, 234),
            IM_COL32(11, 14, 12, 202),
            IM_COL32(12, 15, 13, 222),
            IM_COL32(12, 15, 13, 216),
            IM_COL32(240, 244, 242, 255),
            IM_COL32(134, 144, 140, 190),
            IM_COL32(38, 46, 42, 180),
            0.75f, 0.55f, 0.96f,
        },
        {
            "BLOOD",
            IM_COL32(12, 8, 9, 236),
            IM_COL32(20, 14, 16, 226),
            IM_COL32(26, 18, 21, 234),
            IM_COL32(14, 10, 11, 202),
            IM_COL32(15, 11, 12, 222),
            IM_COL32(15, 11, 12, 216),
            IM_COL32(246, 240, 242, 255),
            IM_COL32(148, 132, 138, 190),
            IM_COL32(48, 38, 42, 180),
            0.75f, 0.55f, 0.96f,
        },
        {
            "LIGHT",
            IM_COL32(246, 246, 249, 238),
            IM_COL32(238, 238, 243, 228),
            IM_COL32(242, 242, 246, 238),
            IM_COL32(250, 250, 252, 206),
            IM_COL32(244, 244, 247, 222),
            IM_COL32(244, 244, 247, 216),
            IM_COL32(18, 18, 24, 255),
            IM_COL32(120, 120, 134, 190),
            IM_COL32(206, 206, 216, 180),
            0.75f, 0.55f, 0.96f,
        },
    };

    inline void ApplyThemePreset(int themeId)
    {
        if (themeId < 0 || themeId >= THEME_COUNT) return;
        g_activeTheme = themeId;
        const ThemePalette& t = g_themes[themeId];

        // Apply accent hue
        g_menuHue = t.accentHue;
        ImGui::ColorConvertHSVtoRGB(t.accentHue, t.accentSat, t.accentVal,
            menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
        c::accent = ImGui::ColorConvertU32ToFloat4(
            IM_COL32((int)(menu[0]*255),(int)(menu[1]*255),(int)(menu[2]*255),255));

        // Apply backgrounds
        c::bg::background   = ImGui::ColorConvertU32ToFloat4(t.bgWindow);
        c::child::background= ImGui::ColorConvertU32ToFloat4(t.bgChild);
        c::child::cap       = ImGui::ColorConvertU32ToFloat4(t.bgCap);

        // Apply text
        c::text::text_active= ImGui::ColorConvertU32ToFloat4(t.textActive);
        c::text::text       = ImGui::ColorConvertU32ToFloat4(t.textMuted);

        // Separator
        c::separator        = ImGui::ColorConvertU32ToFloat4(t.border);
    }
    // ============================================================

    inline ImVec4 GetAccentVec4(float alpha = 1.0f)
    {
        return ImVec4(menu[0], menu[1], menu[2], alpha);
    }

    inline ImU32 GetAccentU32(float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(GetAccentVec4(alpha));
    }

    inline ImVec4 GetAccentTint(float strength, float alpha = 1.0f)
    {
        return ImVec4(menu[0] * strength, menu[1] * strength, menu[2] * strength, alpha);
    }

    inline ImU32 GetAccentTintU32(float strength, float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(GetAccentTint(strength, alpha));
    }

    inline void ApplyAccentFromHue()
    {
        ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.55f, 0.96f, menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
    }

    inline float GetContentPadding()
    {
        return 10.0f;
    }

    inline float GetColumnGap()
    {
        return 10.0f;
    }

    inline float GetChildPadding()
    {
        return 10.0f;
    }

    inline ImVec4 GetSidebarShellBackgroundColor()
    {
        return ImColor(0, 0, 0, 130);
    }

    inline ImVec4 GetActiveTabBackgroundColor()
    {
        return ImColor(0, 0, 0, 150);
    }

    inline void ApplyThemeState()
    {
        c::scale = 1.15f;
        c::widget_scale = 1.45f;
        const float childPadding = GetChildPadding();
        c::accent = ImColor(GetAccentVec4());
        c::separator = ImColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Read from active theme palette for base background colors
        const ThemePalette& tp = g_themes[g_activeTheme];
        const ImVec4 bgW  = ImGui::ColorConvertU32ToFloat4(tp.bgWindow);
        const ImVec4 bgCh = ImGui::ColorConvertU32ToFloat4(tp.bgChild);
        const ImVec4 bgCo = ImGui::ColorConvertU32ToFloat4(tp.bgContent);
        const ImVec4 tAct = ImGui::ColorConvertU32ToFloat4(tp.textActive);
        const ImVec4 tMut = ImGui::ColorConvertU32ToFloat4(tp.textMuted);

        c::bg::background = ImColor(bgW.x, bgW.y, bgW.z, 0.50f);
        c::child::background = GetActiveTabBackgroundColor();
        c::child::cap = ImColor(bgCh.x, bgCh.y, bgCh.z, 0.88f);
        c::child::padding = childPadding / c::scale;
        c::child::spacing = childPadding / c::scale;

        c::page::background_active = ImColor(GetAccentTint(0.28f, 0.55f));
        c::page::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.60f);
        c::page::text_hov = ImColor(tAct.x, tAct.y, tAct.z, 1.0f);
        c::page::text = ImColor(tMut.x, tMut.y, tMut.z, 0.96f);

        // Elements tinted from theme bg
        c::elements::background_hovered = ImColor(
            ImClamp(bgCh.x + 0.04f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.04f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.04f, 0.f, 1.f), 0.86f);
        c::elements::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.70f);

        c::checkbox::mark = ImColor(bgW.x, bgW.y, bgW.z, 1.0f);
        c::checkbox::background_on = ImColor(GetAccentTint(0.92f, 0.96f));
        c::checkbox::background_off = ImColor(
            ImClamp(bgCh.x + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.06f, 0.f, 1.f), 0.90f);
        c::checkbox::circle_inactive = ImColor(
            ImClamp(tMut.x * 0.75f, 0.f, 1.f),
            ImClamp(tMut.y * 0.75f, 0.f, 1.f),
            ImClamp(tMut.z * 0.75f, 0.f, 1.f), 0.90f);

        c::text::text_active = ImColor(tAct.x, tAct.y, tAct.z, 1.0f);
        c::text::text_hov = ImColor(GetAccentTint(0.90f, 0.92f));
        c::text::text = ImColor(tMut.x, tMut.y, tMut.z, 0.95f);

        c::widget::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.76f);
        c::widget::outlinecolor = ImColor(
            ImClamp(bgCh.x + 0.08f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.08f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.08f, 0.f, 1.f), 0.68f);

        c::button::background = ImColor(bgCo.x, bgCo.y, bgCo.z, 0.80f);
        c::button::background_hovered = ImColor(
            ImClamp(bgCh.x + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.06f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.06f, 0.f, 1.f), 0.88f);
        c::button::background_active = ImColor(
            ImClamp(bgCh.x + 0.10f, 0.f, 1.f),
            ImClamp(bgCh.y + 0.10f, 0.f, 1.f),
            ImClamp(bgCh.z + 0.10f, 0.f, 1.f), 0.92f);
        c::button::outline = ImColor(GetAccentTint(0.55f, 0.48f));

        // Scrollbar tinted from accent
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScrollbarSize = 5.0f;
        const ImVec4& ac = c::accent;
        style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(bgW.x, bgW.y, bgW.z, 0.72f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(ac.x*0.60f, ac.y*0.60f, ac.z*0.60f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(ac.x*0.80f, ac.y*0.80f, ac.z*0.80f, 0.90f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(ac.x,       ac.y,       ac.z,       1.0f);
    }
}

#endif // IMGUI_SETTINGS_H
