#pragma once
#include "Widgets.h"
#include "SidebarStyle.h"
#include "LogoVideo.h"
#include "RestoreGesture.h"
#include "WindowPlacement.h"
#include "StartupView.h"
#include "../System/UI/MemoryTab.h"
#include "../System/UI/Keyboard.h"
#include "../System/Hooks/FrameRate.h"
#include "../Trigger/TriggerRuntime.h"
#include "../Trigger/TriggerDotStyle.h"
#include <cstdint>

namespace lite_ui {

using lite_widgets::Fade;

enum class Tab { Home, Esp, Aim, Trigger, Memory, Skins, Settings };
struct State {
    Tab tab = Tab::Home;
    int section = 0;
    bool minimized = false, music = false, hideRecord = false, handcam = false;
    float zoom = 1.f;
    float resizeZoom = 1.f;
    ImVec2 resizeDisplay{};
    bool sizing = false;
    int skinCategory = 0;
    lite_restore::Swipe restore;
    lite_placement::Placement placement;
    char configName[64] = "lite";
};
struct View {
    bool authorized = false, pending = false, ready = false, interactive = true;
    ImTextureID sidebarTexture = nullptr;
    ImVec2 sidebarUv0{0,1},sidebarUv1{1,0};
    lite_startup::Frame startup;
    float preparation = 0.f, fps = 0.f;
    std::uint64_t uptime = 0;
    const char* error = "";
    const char* expiry = "";
    const char* plan = "";
    ImFont* titleFont = nullptr;
    void (*skins)(int) = nullptr;
    void (*music)() = nullptr;
};
struct Actions {
    bool submit = false, paste = false, loadKey = false;
    bool saveConfig = false, loadConfig = false, retryTrigger = false;
};
struct Layout {
    ImVec2 size;
    float scale = 1.f, rail = 56.f, footer = 40.f;
    float zoom = 1.f, maximumZoom = 1.35f;
    bool narrow = false;
};
struct Page { const char* title; const char* subtitle; };
inline const Page pages[]{
    {"Home","Session, licence and music"},
    {"Players","What you see through walls and on screen"},
    {"Aimbot","Aim assistance and targeting"},
    {"Trigger","Automatic fire on target"},
    {"Memory","Movement, weapon and world control"},
    {"Skins","Blueprints and skin categories"},
    {"Settings","Configuration and interface"},
};
inline const char* const sectionsHome[]{"Music"};
inline const char* const sectionsEsp[]{"ESP","Display","Extra"};
inline const char* const sectionsAim[]{"Aimbot","Combat"};
inline const char* const sectionsTrigger[]{"Triggerbot","Crosshair"};
inline const char* const sectionsMemory[]{"Movement","Tuning","Weapon","World"};
inline const char* const sectionsSkins[]{"Blueprints","Category"};
inline const char* const sectionsSettings[]{"Menu","Configs","FPS"};
inline const char* const* Sections(Tab tab,int& count) {
    switch(tab) {
        case Tab::Home: count=1; return sectionsHome;
        case Tab::Esp: count=3; return sectionsEsp;
        case Tab::Aim: count=2; return sectionsAim;
        case Tab::Trigger: count=2; return sectionsTrigger;
        case Tab::Memory: count=4; return sectionsMemory;
        case Tab::Skins: count=2; return sectionsSkins;
        default: count=3; return sectionsSettings;
    }
}
inline const char* const icons[]{"\xef\x80\x95","\xef\x94\x80","\xef\x81\x9b","\xef\x83\xa7",
    "\xef\x80\x93","\xef\x87\xbc","\xef\x81\xbb"};

inline Layout Fit(ImVec2 display,float zoom=1.f) {
    Layout layout;
    display.x=std::isfinite(display.x)?ImMax(1.f,display.x):1.f;
    display.y=std::isfinite(display.y)?ImMax(1.f,display.y):1.f;
    layout.narrow = display.x < 620.f;
    const ImVec2 available(ImMax(1.f,display.x-(layout.narrow?16.f:24.f)),
        ImMax(1.f,display.y-(layout.narrow?16.f:24.f)));
    if (layout.narrow) {
        layout.size = available;
        layout.scale = ImClamp(layout.size.x / 420.f, .85f, 1.f);
        layout.rail = 46.f * layout.scale;
    } else {
        const float fit=ImMin(available.x/880.f,available.y/550.f);
        layout.scale = ImMin(fit,ImClamp(fit,.5f,1.4f));
        layout.size = ImVec2(880,550) * layout.scale;
        layout.rail = 56.f * layout.scale;
    }
    layout.footer = 40.f * layout.scale;
    layout.maximumZoom=ImMin(1.35f,ImMax(1.f,ImMin(available.x/layout.size.x,available.y/layout.size.y)));
    layout.zoom=ImClamp(std::isfinite(zoom)?zoom:1.f,.85f,layout.maximumZoom);
    layout.size=layout.size*layout.zoom;
    layout.scale*=layout.zoom;
    layout.rail=ImMin(layout.size.x*.4f,layout.rail*layout.zoom);
    layout.footer*=layout.zoom;
    return layout;
}
inline Layout MenuLayout(const State& state,ImVec2 display) {
    return Fit(display,state.sizing?state.resizeZoom:state.zoom);
}
inline char* editBuffer = nullptr;
inline std::size_t editCapacity = 0;
inline bool keyboard = false;

inline void EnableHandcam(State& state) {
    state.handcam = true;
    state.hideRecord = true;
    state.minimized = true;
    keyboard = false;
}

inline void Input(const char* label, char* buffer, std::size_t capacity, ImGuiInputTextFlags flags = 0) {
    ImGui::TextUnformatted(label, ImGui::FindRenderedTextEnd(label));
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##value", buffer, capacity, flags);
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
        (!custom::touch::Applies() || custom::touch::Tap())) {
        editBuffer = buffer;
        editCapacity = capacity;
        keyboard = true;
    }
    ImGui::PopID();
}

inline void Theme() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 14.f;
    style.ChildRounding = 12.f;
    style.FrameRounding = 8.f;
    style.PopupRounding = 10.f;
    style.WindowBorderSize = 0.f;
    style.ChildBorderSize = style.FrameBorderSize = 1.f;
    style.ScrollbarSize = 4.f;
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;
    style.ItemSpacing = {0, 6};
    style.FramePadding = {10, 6};
    style.Colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(lite_colors::Bg);
    style.Colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(lite_colors::Bg);
    style.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(lite_colors::Text);
    style.Colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(lite_colors::TextDim);
    style.Colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(lite_colors::InputBg);
    style.Colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(lite_colors::Card);
    style.Colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(lite_colors::DropdownGradTop);
    style.Colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(lite_colors::CardBorder);
    style.Colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(lite_colors::DropdownBg);
    style.Colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(lite_colors::BindItemBg);
    style.Colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(lite_colors::DropdownGradTop);
    style.Colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(lite_colors::AccentDark);
    style.Colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(lite_colors::ButtonBgBot);
    style.Colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(lite_colors::Accent);
    style.Colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(lite_colors::AccentHover);
    style.Colors[ImGuiCol_PlotHistogram] = ImGui::ColorConvertU32ToFloat4(lite_colors::Accent);
    style.Colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(lite_colors::Section);
    style.Colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(lite_colors::AccentHover);
    style.Colors[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4(lite_colors::AccentDark);
    style.Colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(lite_colors::Section);
    lite_colors::Accent = IM_COL32(255,95,168,255);
    c::accent = ImGui::ColorConvertU32ToFloat4(lite_colors::Accent);
    c::text::text_active = ImGui::ColorConvertU32ToFloat4(lite_colors::TextBright);
    c::text::text = ImGui::ColorConvertU32ToFloat4(lite_colors::Text);
    c::text::text_hov = ImGui::ColorConvertU32ToFloat4(lite_colors::TextBright);
    c::elements::background = ImGui::ColorConvertU32ToFloat4(lite_colors::InputBg);
    c::elements::background_hovered = ImGui::ColorConvertU32ToFloat4(lite_colors::BindItemBg);
}

template<class Draw>
inline void Panel(const char* label, const char* title, float height, Draw draw) {
    ImGui::TableNextColumn();
    if (lite_widgets::BeginPanel(label, {0, height}, 0, title)) draw();
    lite_widgets::EndPanel();
}

inline void Rail(State& state, const View& view) {
    const auto scale = lite_widgets::scale;
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float logo = 26.f * scale;
    const ImVec2 logoLo(origin.x + (width - logo) * .5f, origin.y + 14.f * scale);
    const ImVec2 logoHi = logoLo + ImVec2(logo, logo);
    const float logoRounding = 8.f * scale;
    if (const auto texture = lite_logo::Ensure()) {
        ImVec2 uv0, uv1;
        lite_logo::Uv(lite_logo::Frame(ImGui::GetTime(), view.startup.reducedMotion), uv0, uv1);
        draw->AddImageRounded(texture, logoLo, logoHi, uv0, uv1, IM_COL32_WHITE, logoRounding);
    } else draw->AddRectFilled(logoLo, logoHi, Fade(lite_colors::Card), logoRounding);
    draw->AddRect(logoLo, logoHi, Fade(lite_colors::CardBorder), logoRounding, 0, ImMax(1.f, scale));
    const float pitch = 42.f * scale;
    float y = origin.y + 54.f * scale;
    for (int index = 0; index < 7; ++index) {
        const auto tab = static_cast<Tab>(index);
        const bool active = state.tab == tab;
        ImGui::SetCursorScreenPos({origin.x, y});
        ImGui::PushID(index);
        const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton("##rail", {width, pitch}));
        ImGui::PopID();
        const bool hovered = ImGui::IsItemHovered();
        if (active || hovered) draw->AddRectFilled({origin.x + 7.f * scale, y + 2.f * scale},
            {origin.x + width - 7.f * scale, y + pitch - 2.f * scale},
            active ? IM_COL32(90, 79, 216, 70) : IM_COL32(255, 255, 255, 8), 10.f * scale);
        if (active) draw->AddRectFilled({origin.x, y + 9.f * scale}, {origin.x + 3.f * scale, y + pitch - 9.f * scale},
            Fade(lite_colors::Accent), 2.f * scale);
        const auto glyph = ImGui::CalcTextSize(icons[index]);
        draw->AddText({origin.x + (width - glyph.x) * .5f, y + (pitch - glyph.y) * .5f},
            active ? Fade(lite_colors::Accent) : Fade(hovered ? lite_colors::Text : lite_colors::TextDim),
            icons[index]);
        if (pressed) { state.tab = tab; state.section = 0; keyboard = false; }
        y += pitch;
    }
}

inline float Header(State& state, const Layout& layout) {
    const auto scale = lite_widgets::scale;
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const auto* font = ImGui::GetFont();
    const auto& page = pages[static_cast<int>(state.tab)];
    draw->AddText(font, 21.f * scale, origin, Fade(lite_colors::TextBright), page.title);
    draw->AddText(font, 12.f * scale, {origin.x, origin.y + 27.f * scale}, Fade(lite_colors::TextDim), page.subtitle);
    int count = 0;
    const char* const* labels = Sections(state.tab, count);
    state.section = ImClamp(state.section, 0, count - 1);
    if (count <= 1) return 72.f * scale;
    const float pillHeight = 26.f * scale, gap = 4.f * scale;
    const auto titleSize = font->CalcTextSizeA(21.f * scale, FLT_MAX, 0, page.title);
    const auto subtitleSize = font->CalcTextSizeA(12.f * scale, FLT_MAX, 0, page.subtitle);
    const float titleWidth = ImMax(titleSize.x, subtitleSize.x);
    float widths[8]{}, total = 0.f;
    for (int index = 0; index < count; ++index) {
        widths[index] = font->CalcTextSizeA(12.f * scale, FLT_MAX, 0, labels[index]).x + 24.f * scale;
        total += widths[index] + (index ? gap : 0.f);
    }
    float x = origin.x + width - total;
    float y = origin.y + 1.f * scale;
    if (x < origin.x + titleWidth + 18.f * scale) { x = origin.x; y = origin.y + 46.f * scale; }
    const ImVec2 groupLo(x - 4.f * scale, y - 3.f * scale), groupHi(x + total + 4.f * scale, y + pillHeight + 3.f * scale);
    draw->AddRectFilled(groupLo + ImVec2(3.f, 4.f) * scale, groupHi + ImVec2(3.f, 4.f) * scale,
        IM_COL32(90, 79, 216, 55), 15.f * scale);
    draw->AddRectFilled(groupLo, groupHi, Fade(lite_colors::Card), 15.f * scale);
    draw->AddRect(groupLo, groupHi, Fade(lite_colors::CardBorder), 15.f * scale);
    float targetX = x;
    for (int index = 0; index < state.section; ++index) targetX += widths[index] + gap;
    const auto tab = sakuga::TabMotion(targetX, widths[state.section], state.section,
        {targetX + widths[state.section] * .5f, y + pillHeight * .5f}, pillHeight);
    const ImVec2 indicator(tab.x, y - 4.f * scale + tab.lift);
    draw->AddRectFilled(indicator + ImVec2(2.f, 3.f) * scale,
        indicator + ImVec2(tab.width, pillHeight + 6.f * scale) + ImVec2(2.f, 3.f) * scale,
        IM_COL32(90, 79, 216, 90), 11.f * scale);
    draw->AddRectFilled(indicator, indicator + ImVec2(tab.width, pillHeight + 6.f * scale),
        Fade(lite_colors::Card), 11.f * scale);
    draw->AddRect(indicator, indicator + ImVec2(tab.width, pillHeight + 6.f * scale),
        Fade(lite_colors::Accent), 11.f * scale, 0, ImMax(1.f, 1.3f * scale));
    for (int index = 0; index < count; ++index) {
        const auto measured = font->CalcTextSizeA(12.f * scale, FLT_MAX, 0, labels[index]);
        const ImVec2 size(widths[index], pillHeight);
        ImGui::SetCursorScreenPos({x, y});
        ImGui::PushID(index);
        const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton("##section", size));
        ImGui::PopID();
        const bool hovered = ImGui::IsItemHovered();
        const bool active = state.section == index;
        if (!active && hovered) draw->AddRectFilled({x, y}, {x + size.x, y + size.y}, IM_COL32(255, 255, 255, 12), 11.f * scale);
        draw->AddText(font, 12.f * scale, {x + (size.x - measured.x) * .5f, y + (pillHeight - measured.y) * .5f},
            active ? Fade(lite_colors::TextBright) : Fade(hovered ? lite_colors::Soft : lite_colors::TextDim),
            labels[index]);
        if (pressed) { state.section = index; keyboard = false; }
        x += size.x + gap;
    }
    return y > origin.y + 40.f * scale ? 118.f * scale : 76.f * scale;
}

inline void Footer(State& state, const View&) {
    const auto scale = lite_widgets::scale;
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetWindowSize();
    draw->AddLine(origin, {origin.x + size.x, origin.y}, Fade(lite_colors::CardBorder));
    const auto* font = ImGui::GetFont();
    const float middle = origin.y + size.y * .5f;
    const char* name = "SHAINIME";
    const auto nameSize = font->CalcTextSizeA(12.f * scale, FLT_MAX, 0, name);
    draw->AddText(font, 12.f * scale, {origin.x + 18.f * scale, middle - nameSize.y * .5f},
        Fade(lite_colors::TextBright), name);
    const char* version = "v1.0";
    const auto versionSize = font->CalcTextSizeA(11.f * scale, FLT_MAX, 0, version);
    draw->AddText(font, 11.f * scale,
        {origin.x + 18.f * scale + nameSize.x + 12.f * scale, middle - versionSize.y * .5f},
        Fade(lite_colors::TextDim), version);
    const char* hint = "swipe up from the bottom";
    const auto hintSize = font->CalcTextSizeA(11.f * scale, FLT_MAX, 0, hint);
    const bool showHint = size.x > hintSize.x + 190.f * scale;
    if (showHint) draw->AddText(font, 11.f * scale,
        {origin.x + size.x - 18.f * scale - hintSize.x, middle - hintSize.y * .5f},
        Fade(lite_colors::TextDim), hint);
    const char* label = "Hide";
    const auto labelSize = font->CalcTextSizeA(11.f * scale, FLT_MAX, 0, label);
    const ImVec2 badge(labelSize.x + 20.f * scale, 22.f * scale);
            ImGui::SetCursorScreenPos({origin.x + size.x - 18.f * scale - (showHint ? hintSize.x + 12.f * scale : 0.f) - badge.x,
        middle - badge.y * .5f});
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton("##hide_lite", badge));
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 badgeLo = ImGui::GetItemRectMin();
    draw->AddRectFilled(badgeLo, badgeLo + badge, hovered ? IM_COL32(255, 255, 255, 16) : IM_COL32(255, 255, 255, 8),
        7.f * scale);
    draw->AddRect(badgeLo, badgeLo + badge, Fade(lite_colors::CardBorder), 7.f * scale);
    draw->AddText(font, 11.f * scale, {badgeLo.x + 10.f * scale, badgeLo.y + (badge.y - labelSize.y) * .5f},
        Fade(hovered ? lite_colors::TextBright : lite_colors::Text), label);
    if (pressed) { state.minimized = true; keyboard = false; }
}

inline void StatusPanel(const View& view) {
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%.0f FPS", view.fps);
    lite_widgets::InfoRow("Framerate", buffer, lite_colors::TextBright);
    std::snprintf(buffer, sizeof(buffer), "%02llu:%02llu:%02llu",
        static_cast<unsigned long long>(view.uptime / 3600),
        static_cast<unsigned long long>(view.uptime / 60 % 60),
        static_cast<unsigned long long>(view.uptime % 60));
    lite_widgets::InfoRow("Uptime", buffer, lite_colors::TextBright);
    lite_widgets::InfoRow("Plan", view.plan && *view.plan ? view.plan : "Free", lite_colors::Accent);
    lite_widgets::InfoRow("Expires", view.expiry && *view.expiry ? view.expiry : "No active license",
        lite_colors::Text);
}

inline void Combo(const char* label, int& value, const char* const* items, int count) {
    ImGui::PushID(label);
    value = ImClamp(value, 0, count - 1);
    custom::Combo(label, &value, items, count);
    ImGui::PopID();
}

inline void EspColors(const char* label, float* line, float* box, float* name, float* health, float* distance, float* skeleton) {
    if (!line) return;
    const auto normalize = [](float value) { return ImClamp(value <= 1.f ? value : value / 255.f,0.f,1.f); };
    float color[]{normalize(line[0]), normalize(line[1]), normalize(line[2])};
    if (ImGui::ColorEdit3(label, color, ImGuiColorEditFlags_NoInputs)) {
        float* targets[]{line,box,name,health,distance,skeleton};
        for (auto* target : targets) if (target) {
            for (int channel = 0; channel < 3; ++channel) target[channel] = color[channel]*255.f;
            target[3] = 255.f;
        }
    }
}

inline void PreparePlacement(State& state,const View& view) {
    const auto& io=ImGui::GetIO();
    const auto limits=Fit(io.DisplaySize);
    state.zoom=ImClamp(std::isfinite(state.zoom)?state.zoom:1.f,.85f,limits.maximumZoom);
    if(state.sizing && (!view.ready || !view.interactive || state.minimized || state.tab!=Tab::Settings ||
        (!io.MouseDown[0] && !io.MouseReleased[0]) ||
        state.resizeDisplay.x!=io.DisplaySize.x || state.resizeDisplay.y!=io.DisplaySize.y)) state.sizing=false;
    const auto layout=MenuLayout(state,io.DisplaySize);
    const auto pos=state.placement.Position(io.DisplaySize,layout.size);
    const ImRect header(pos,pos+ImVec2(layout.size.x,96.f*layout.scale));
    state.placement.Update(ImGui::GetFrameCount(),view.ready && view.interactive && !state.minimized && !state.restore.Captures(),
        io.DisplaySize,layout.size,header,io.MousePos,io.MouseClicked[0],io.MouseDown[0],ImMax(7.f,7.f*layout.scale));
    if(state.placement.Tracking()) custom::CancelTouchScroll();
}

inline Actions Draw(State& state, const View& view, char* key, std::size_t capacity) {
    Actions actions;
    if (!view.ready) sakuga::ClearEffects();
    Theme();
    PreparePlacement(state,view);
    const auto layout = MenuLayout(state,ImGui::GetIO().DisplaySize);
    lite_widgets::scale = layout.scale;
    sakuga::scale = layout.scale;
    sakuga::opacity = ImClamp(view.startup.reveal, 0.f, 1.f);
    c::scale = lite_widgets::scale;
    const auto& pointer=ImGui::GetIO();
    const auto restore=lite_restore::Fit(pointer.DisplaySize,layout.scale);
    if(state.restore.Update(view.ready && view.interactive && state.minimized,restore,pointer.MousePos,
        pointer.MouseClicked[0],pointer.MouseDown[0],pointer.MouseReleased[0])) {
        state.minimized=false;keyboard=false;ImGui::ClearActiveID();
    }
    if (view.ready && state.minimized) {
        ImGui::SetNextWindowPos(restore.hit.Min,ImGuiCond_Always);
        ImGui::SetNextWindowSize(restore.hit.GetSize(),ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::Begin("##lite_restore", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowFontScale(1.f);
        if (!state.handcam) {
            const unsigned alpha=static_cast<unsigned>(160.f+95.f*state.restore.Progress());
            ImGui::GetWindowDrawList()->AddRectFilled(restore.line.Min,restore.line.Max,
                IM_COL32(255,255,255,alpha),restore.line.GetHeight()*.5f);
        }
        ImGui::End();
        ImGui::PopStyleVar();
        return actions;
    }

    if (!view.ready) {
        const auto display = ImGui::GetIO().DisplaySize;
        const bool error = view.error && *view.error;
        const ImVec2 size(ImMin(view.authorized?360.f:440.f,display.x-24.f),
            ImMin(view.authorized?300.f:(error?374.f:330.f),display.y-24.f));
        const float alpha = view.authorized && view.startup.active && !view.startup.reducedMotion ?
            lite_startup::Ease(float(view.startup.elapsed/lite_startup::introFadeSeconds))*(1.f-lite_startup::Ease((view.startup.finish-.88f)/.12f)) : 1.f;
        ImGui::SetNextWindowPos(display*.5f, ImGuiCond_Always, {.5f,.5f});
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {24,22});
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*alpha);
        ImGui::Begin("SHAINIME##login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowFontScale(1.f);
        lite_widgets::scale = 1.f;
        lite_startup::Card(view.startup.reducedMotion);
        if (view.authorized) lite_startup::Loading(view.titleFont,view.preparation,view.startup);
        else {
            const auto pos = ImGui::GetWindowPos();
            const ImVec2 center(pos.x+size.x*.5f,pos.y);
            lite_startup::Header(view.titleFont,view.pending);
            ImGui::SetCursorPos({24,118});
            ImGui::BeginDisabled(view.pending);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{12,10});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,10.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.6f);
            Input("Access key",key,capacity);
            ImGui::PopStyleVar(3);
            ImGui::SetCursorPos({24,184});
            const float half = (ImGui::GetContentRegionAvail().x-8.f)*.5f;
            actions.paste = lite_startup::Secondary("Paste key",{half,28});
            ImGui::SameLine(0,8);
            actions.loadKey = lite_startup::Secondary("Load saved",{half,28});
            ImGui::EndDisabled();
            ImGui::SetCursorPos({24,226});
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled,view.pending);
            actions.submit = lite_startup::Primary("Authenticate",view.pending,view.startup.reducedMotion,
                {ImGui::GetContentRegionAvail().x,38});
            ImGui::PopItemFlag();
            if (error) {
                ImGui::SetCursorPos({24,284});
                ImGui::BeginChild("##login_error",{size.x-48,ImMax(20.f,size.y-296)},false,
                    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
                ImGui::TextWrapped("%s",view.error);
                ImGui::EndChild();
            } else lite_startup::Centered(nullptr,11,{center.x,pos.y+294},"Secure access",lite_startup::Ink(100));
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    } else {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled,!view.interactive || state.restore.Captures());
        const auto menuPos=state.placement.Position(ImGui::GetIO().DisplaySize,layout.size);
        lite_sidebar::Shadow(menuPos,layout.size,view.startup.reveal);
        ImGui::SetNextWindowPos(menuPos,ImGuiCond_Always);
        ImGui::SetNextWindowSize(layout.size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::Begin("SHAINIME##menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        lite_widgets::SetFontScale();
        const auto pos = ImGui::GetWindowPos();
        const float bodyHeight = ImMax(1.f, layout.size.y - layout.footer);
        lite_sidebar::Surface(pos,layout.size,layout.rail,bodyHeight,view.sidebarTexture,view.sidebarUv0,view.sidebarUv1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
        ImGui::BeginChild("##sidebar", {layout.rail,bodyHeight}, false,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
        lite_widgets::SetFontScale();
        Rail(state,view);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::SameLine(0,0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
            ImVec2((layout.narrow?12.f:22.f)*layout.scale,16.f*layout.scale));
        ImGui::BeginChild("##content", {0,bodyHeight}, false,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
        lite_widgets::SetFontScale();
        const ImVec2 contentTop = ImGui::GetCursorScreenPos();
        const float headerHeight = Header(state,layout);
        ImGui::SetCursorScreenPos({contentTop.x,contentTop.y+headerHeight});
        const float cardHeight = ImMax(160.f*layout.scale, bodyHeight - 36.f*layout.scale - headerHeight);
        const bool skins = state.tab == Tab::Skins;
        const float topHeight = skins ? ImMin(220.f*layout.scale, ImMax(150.f*layout.scale, cardHeight*.46f)) : cardHeight;
        const auto fit = [&](float) { return topHeight; };
        const float sideHeight = skins ? topHeight : (layout.narrow ? ImMin(200.f*layout.scale, cardHeight) : cardHeight);
        const float cardGap = 10.f*layout.scale;
        const int columns = layout.narrow ? 1 : 2;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0,0));
        if (ImGui::BeginTable("##cards", columns, ImGuiTableFlags_SizingStretchSame)) {
            switch (state.tab) {
                case Tab::Home:
                    Panel("music", "Music", fit(320), [&] {
                        lite_widgets::Checkbox("Music player",&state.music);
                        if (state.music && view.music) view.music();
                        else ImGui::TextWrapped("Enable to browse local or Cloud songs.");
                    });
                    break;
                case Tab::Esp:
                    if (state.section == 0) Panel("players", "ESP", fit(430), [&] {
                        lite_widgets::Checkbox("ESP Line",&Config.ESPMenu.isPlayerLine);
                        lite_widgets::Checkbox("ESP Box",&Config.ESPMenu.Box);
                        lite_widgets::Checkbox("ESP Skeleton",&Config.ESPMenu.Skeleton);
                        lite_widgets::Checkbox("ESP Health",&Config.ESPMenu.Health);
                        lite_widgets::Checkbox("ESP Name",&Config.ESPMenu.Name);
                        lite_widgets::Checkbox("ESP Distance",&Config.ESPMenu.Distance);
                        lite_widgets::Checkbox("ESP Count",&Config.ESPMenu.Count);
                        lite_widgets::Checkbox("360 Alert",&Config.ESPMenu.Alert);
                        lite_widgets::Checkbox("Throwable Alert",&Config.ESPMenu.GrenadeWarn);
                        lite_widgets::Checkbox("BRClass",&Config.ESPMenu.BRClass);
                        lite_widgets::Checkbox("Show AimLine",&Config.ESPMenu.Aimline);
                    });
                    else if (state.section == 1) Panel("display options", "Display", fit(410), [&] {
                        static const char* boxTypes[]{"Fill","Outline","Corner","3D"};
                        static const char* positions[]{"Top","Center","Bottom"};
                        static const char* healthPositions[]{"Top","Side"};
                        static const char* styles[]{"None","3D sphere","Player signal"};
                        int box=static_cast<int>(Config.ESPMenu.BoxType), line=static_cast<int>(Config.ESPMenu.Target);
                        int health=static_cast<int>(Config.ESPMenu.HealthPosition), style=static_cast<int>(Config.ESPMenu.EspStyle);
                        Combo("Box type",box,boxTypes,4); Combo("Line position",line,positions,3);
                        Combo("Health position",health,healthPositions,2); Combo("ESP style",style,styles,3);
                        Config.ESPMenu.BoxType=static_cast<EspBoxType>(box);
                        Config.ESPMenu.Target=static_cast<LineTarget>(line);
                        Config.ESPMenu.HealthPosition=static_cast<EspHealthPosition>(health);
                        Config.ESPMenu.EspStyle=static_cast<EspStyleTarget>(style);
                        EspColors("Player color",Config.sColorsESPPLAYER.LinePLAYER,Config.sColorsESPPLAYER.BoxPLAYER,
                            Config.sColorsESPPLAYER.NamePLAYER,Config.sColorsESPPLAYER.HealthPLAYER,Config.sColorsESPPLAYER.DistancePLAYER,Config.sColorsESPPLAYER.SkeletonPLAYER);
                        EspColors("Bot color",Config.sColorsESPBOT.LineBOT,Config.sColorsESPBOT.BoxBOT,
                            Config.sColorsESPBOT.NameBOT,Config.sColorsESPBOT.HealthBOT,Config.sColorsESPBOT.DistanceBOT,Config.sColorsESPBOT.SkeletonBOT);
                    });
                    else Panel("extras", "Extra", fit(240), [&] {
                        lite_widgets::Checkbox("Yellow Wallhack",&Config.ExtraMenu.WallHack);
                        lite_widgets::Checkbox("Red Wallhack",&Config.ExtraMenu.RedWallhack);
                        if (Config.ExtraMenu.RedWallhack)
                            lite_widgets::SliderFloat("Red range",&Config.ExtraMenu.RedWallhackRange,5.f,200.f,"%.0f");
                    });
                    break;
                case Tab::Aim:
                    if (state.section == 0) Panel("aimbot", "Aimbot", fit(300), [&] {
                        lite_widgets::Checkbox("Aimbot 360",&Config.Aim.Aimbot360);
                        lite_widgets::Checkbox("Bullet Track",&Config.Aim.AimSilent);
                        lite_widgets::SliderFloat("Aim Assist Size",&Config.Aim.AimAssistSize,0,100,"%.0f");
                        lite_widgets::Checkbox("Show FOV",&Config.ESPMenu.ShowFov);
                    });
                    else Panel("combat options", "Combat", fit(300), [&] {
                        static const char* triggers[]{"None","Shooting","Scoping"};
                        static const char* targets[]{"Head","Chest","Body"};
                        static const char* by[]{"Distance","Crosshair"};
                        int trigger=static_cast<int>(Config.Aim.Trigger), target=static_cast<int>(Config.Aim.Target), method=static_cast<int>(Config.Aim.By);
                        Combo("Trigger",trigger,triggers,3); Combo("Target",target,targets,3); Combo("Target by",method,by,2);
                        Config.Aim.Trigger=static_cast<EAimTrigger>(trigger); Config.Aim.Target=static_cast<EAimTarget>(target); Config.Aim.By=static_cast<EAim>(method);
                        lite_widgets::SliderFloat("FOV Size",&Config.Aim.Cross,0,100,"%.0f");
                    });
                    break;
                case Tab::Trigger:
                    if (state.section == 0) Panel("triggerbot", "Triggerbot", fit(210), [&] {
                        auto settings=PhantomTrigger::GetSettings();
                        const bool changed=lite_widgets::Checkbox("Triggerbot",&settings.enabled);
                        static const char* modes[]{"Both","ADS Only"};
                        int mode=settings.mode==PhantomTrigger::Mode::ADSOnly?1:0;
                        Combo("Mode",mode,modes,2); settings.mode=mode?PhantomTrigger::Mode::ADSOnly:PhantomTrigger::Mode::Both;
                        PhantomTrigger::SetSettings(settings);
                        actions.retryTrigger=changed&&settings.enabled;
                    });
                    else Panel("crosshair", "Crosshair", fit(330), [&] {
                        auto settings=PhantomTrigger::GetSettings();
                        lite_widgets::Checkbox("Dot Crosshair",&settings.showDot); PhantomTrigger::SetSettings(settings);
                        auto& dot=TriggerDotStyle::appearance;
                        lite_widgets::SliderFloat("Dot Size",&dot.radius,1,12,"%.1f");
                        lite_widgets::SliderFloat("Outline Size",&dot.outline,0,5,"%.1f");
                        lite_widgets::SliderFloat("Dot Opacity",&dot.opacity,.1f,1,"%.2f");
                        ImGui::ColorEdit3("Dot Color",dot.color,ImGuiColorEditFlags_NoInputs);
                        TriggerDotStyle::Normalize(dot);
                    });
                    break;
                case Tab::Memory:
                    if (state.section == 0) Panel("movement", "Movement", fit(430), [] { memory_tab::Movement(); });
                    else if (state.section == 1) Panel("movement tuning", "Tuning", fit(330), [] { memory_tab::MovementTuning(); });
                    else if (state.section == 2) Panel("weapon", "Weapon", fit(430), [] { memory_tab::WeaponControl(); memory_tab::WeaponHandling(); });
                    else Panel("world", "World", fit(330), [] { memory_tab::Protection(); memory_tab::Camera(); });
                    break;
                case Tab::Skins:
                    if (state.section == 0) Panel("blueprints", "Blueprints", fit(330), [] { memory_tab::Blueprints(); });
                    else Panel("category", "Category", fit(220), [&] {
                        static const char* categories[]{"Character","Watch","Deadbox","Plane","Weapon","Camo","Snowboard"};
                        Combo("Skin category",state.skinCategory,categories,7);
                    });
                    break;
                case Tab::Settings:
                    if (state.section == 0) Panel("menu", "Menu", fit(340), [&] {
                        lite_widgets::SliderFloat("Menu size",&state.zoom,.85f,layout.maximumZoom,"%.2fx");
                        if(ImGui::IsItemActive() && ImGui::GetIO().MouseDown[0] && !state.sizing) {
                            state.resizeZoom=layout.zoom;state.resizeDisplay=ImGui::GetIO().DisplaySize;state.sizing=true;
                        }
                        if(!ImGui::GetIO().MouseDown[0]) state.sizing=false;
                        if (lite_widgets::Checkbox("Handcam Mode",&state.handcam) && state.handcam)
                            EnableHandcam(state);
                        lite_widgets::Checkbox("Hide Record",&state.hideRecord);
                        lite_widgets::Checkbox("Clear Display",&Config.ExtraMenu.ClearDisplay);
                        lite_widgets::Checkbox("Reset Guest",&Config.ExtraMenu.ResetGuest);
                        ImGui::Dummy({0,6.f*layout.scale});
                        if (lite_widgets::Button("Hide menu",{0,0})) state.minimized=true;
                    });
                    else if (state.section == 1) Panel("configs", "Configs", fit(320), [&] {
                        Input("Configuration name",state.configName,sizeof(state.configName));
                        ImGui::Dummy({0,6.f*layout.scale});
                        actions.saveConfig=lite_widgets::Button("Save configuration",{0,0});
                        ImGui::Dummy({0,6.f*layout.scale});
                        actions.loadConfig=lite_widgets::Button("Load configuration",{0,0});
                        if (view.error&&*view.error) ImGui::TextWrapped("%s",view.error);
                    });
                    else Panel("frame rate", "Frame rate", fit(320), [] {
                        Config.Aim.FpsLevel1 = frame_rate::Fps(Config.Aim.FpsLevel1,60);
                        Config.Aim.FpsLevelUltra1 = frame_rate::Fps(Config.Aim.FpsLevelUltra1,240,60);
                        lite_widgets::Checkbox("Custom FPS",&Config.Aim.FpsLevel);
                        if(Config.Aim.FpsLevel) lite_widgets::SliderFloat("FPS",&Config.Aim.FpsLevel1,30,360,"%.0f");
                        lite_widgets::Checkbox("Ultra FPS override",&Config.Aim.FpsLevelUltra);
                        if(Config.Aim.FpsLevelUltra) lite_widgets::SliderFloat("Ultra FPS",&Config.Aim.FpsLevelUltra1,60,360,"%.0f");
                        frame_rate::Publish(frame_rate::Configured(Config.Aim.FpsLevel,Config.Aim.FpsLevel1,Config.Aim.FpsLevelUltra,Config.Aim.FpsLevelUltra1));
                    });
                    break;
            }
            if (columns > 1) {
                ImGui::TableNextColumn();
                ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(cardGap, 0));
                if (lite_widgets::BeginPanel("status", {0, sideHeight}, 0, "Status")) StatusPanel(view);
                lite_widgets::EndPanel();
            } else Panel("status", "Status", sideHeight, [&] { StatusPanel(view); });
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        if (skins && view.skins) {
            const float libraryHeight = ImMax(80.f*layout.scale, cardHeight - topHeight - cardGap);
            if (lite_widgets::BeginPanel("skin library", {0, libraryHeight}, 0, "Skin library")) view.skins(state.skinCategory);
            lite_widgets::EndPanel();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::BeginChild("##footer", {layout.size.x,layout.footer}, false,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse);
        lite_widgets::SetFontScale();
        Footer(state,view);
        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
        if (view.startup.reveal < 1.f) for (auto* window : GImGui->Windows) {
            if (!window->Active || std::strcmp(window->RootWindow->Name,"SHAINIME##menu")) continue;
            for (auto& vertex : window->DrawList->VtxBuffer) {
                const unsigned alpha = static_cast<unsigned>((vertex.col>>24)*view.startup.reveal);
                vertex.col = (vertex.col&0x00ffffffu)|(alpha<<24);
            }
        }
    }
    sakuga::Draw(ImGui::GetForegroundDrawList());
    if (keyboard && editBuffer) RenderVirtualKeyboard("##lite_keyboard",editBuffer,editCapacity,&keyboard);
    return actions;
}

inline bool EspEnabled() {
    const auto& value=Config.ESPMenu;
    return value.isPlayerLine||value.Box||value.Skeleton||value.Health||value.Name||value.Distance||value.Count||
        value.Alert||value.BRClass||value.Aimline||value.ShowFov||value.Armor||value.GrenadeWarn||value.Vehicle||value.Crosshair||
        value.EspStyle!=EspStyleTarget::EspStyleNone;
}
}
