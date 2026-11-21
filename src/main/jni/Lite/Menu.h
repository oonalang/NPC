#pragma once
#include "Widgets.h"
#include "SidebarStyle.h"
#include "RestoreGesture.h"
#include "WindowPlacement.h"
#include "StartupView.h"
#include "../System/UI/MemoryTab.h"
#include "../System/UI/Keyboard.h"
#include "../System/UI/TriggerDiagnostics.h"
#include "../System/Hooks/FrameRate.h"
#include "../Trigger/TriggerRuntime.h"
#include "../Trigger/TriggerDotStyle.h"
#include <cstdint>

namespace lite_ui {

enum class Tab { Home, Esp, Aim, Trigger, Memory, Skins, Settings };
struct State {
    Tab tab = Tab::Home;
    bool minimized = false, music = false, hideRecord = false;
    float zoom = 1.f;
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
    float scale = 1.f, sidebar = 180.f;
    bool narrow = false;
};
inline Layout Fit(ImVec2 display) {
    Layout layout;
    layout.narrow = display.x < 620.f;
    if (layout.narrow) {
        layout.size = {ImMax(160.f, display.x - 16.f), ImMax(160.f, display.y - 16.f)};
        layout.scale = ImClamp(layout.size.x / 420.f, .85f, 1.f);
        layout.sidebar = 120.f * layout.scale;
    } else {
        layout.scale = ImClamp(ImMin((display.x - 24.f) / 705.f, (display.y - 24.f) / 500.f), .5f, 1.45f);
        layout.size = ImVec2(705,500) * layout.scale;
        layout.sidebar = 180.f * layout.scale;
    }
    return layout;
}
inline char* editBuffer = nullptr;
inline std::size_t editCapacity = 0;
inline bool keyboard = false;

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
    style.WindowRounding = 20.f;
    style.ChildRounding = 9.f;
    style.FrameRounding = 3.f;
    style.PopupRounding = 4.f;
    style.WindowBorderSize = 0.f;
    style.ChildBorderSize = style.FrameBorderSize = 1.f;
    style.ScrollbarSize = 6.f;
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;
    style.ItemSpacing = {0, 2};
    style.FramePadding = {6, 4};
    style.Colors[ImGuiCol_WindowBg] = {18.f/255,18.f/255,18.f/255,1};
    style.Colors[ImGuiCol_ChildBg] = {18.f/255,18.f/255,18.f/255,1};
    style.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(lite_colors::Text);
    style.Colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(lite_colors::TextDim);
    style.Colors[ImGuiCol_FrameBg] = {31.f/255,31.f/255,31.f/255,1};
    style.Colors[ImGuiCol_FrameBgHovered] = {.15f,.15f,.15f,1};
    style.Colors[ImGuiCol_FrameBgActive] = {.18f,.18f,.18f,1};
    style.Colors[ImGuiCol_Border] = {42.f/255,42.f/255,42.f/255,1};
    style.Colors[ImGuiCol_PopupBg] = {.075f,.075f,.075f,1};
    style.Colors[ImGuiCol_Header] = {.15f,.15f,.15f,1};
    style.Colors[ImGuiCol_HeaderHovered] = {.19f,.19f,.19f,1};
    style.Colors[ImGuiCol_HeaderActive] = {.22f,.22f,.22f,1};
    style.Colors[ImGuiCol_Button] = {.14f,.14f,.14f,1};
    style.Colors[ImGuiCol_ButtonHovered] = {.20f,.20f,.20f,1};
    style.Colors[ImGuiCol_ButtonActive] = {.25f,.25f,.25f,1};
    style.Colors[ImGuiCol_PlotHistogram] = {.9f,.9f,.9f,1};
    style.Colors[ImGuiCol_SliderGrab] = {.85f,.85f,.85f,1};
    style.Colors[ImGuiCol_SliderGrabActive] = {.95f,.95f,.95f,1};
    style.Colors[ImGuiCol_TextSelectedBg] = {.4f,.4f,.4f,.4f};
    style.Colors[ImGuiCol_CheckMark] = {.95f,.95f,.95f,1};
    lite_colors::Accent = IM_COL32(180,180,180,255);
    c::accent = {.85f,.85f,.85f,1};
    c::text::text_active = {.95f,.95f,.95f,1};
    c::text::text = {.8f,.8f,.8f,1};
    c::text::text_hov = {.85f,.85f,.85f,1};
    c::elements::background = {.12f,.12f,.12f,1};
    c::elements::background_hovered = {.15f,.15f,.15f,1};
}

template<class Draw>
inline void Panel(const char* label, float height, Draw draw) {
    ImGui::TableNextColumn();
    if (lite_widgets::BeginPanel(label, {0, height * lite_widgets::scale})) draw();
    lite_widgets::EndPanel();
}

inline bool SideTab(const char* label, Tab tab, State& state) {
    const auto lo = ImGui::GetCursorScreenPos();
    const auto scale=lite_widgets::scale;
    const ImVec2 size(ImGui::GetContentRegionAvail().x,26.f*scale);
    const bool pressed = lite_widgets::Tap(ImGui::InvisibleButton(label, size));
    const bool active = state.tab == tab;
    auto* draw = ImGui::GetWindowDrawList();
    if (active || ImGui::IsItemHovered()) draw->AddRectFilled(lo+ImVec2(10.f*scale,0),lo+size-ImVec2(12.f*scale,0),
        active ? IM_COL32(36,36,36,200) : IM_COL32(28,28,28,190),5.f*scale);
    static constexpr const char* icons[]{"\xef\x80\x95","\xef\x94\x80","\xef\x81\x9b","\xef\x83\xa7",
        "\xef\x80\x93","\xef\x87\xbc","\xef\x81\xbb"};
    const auto* icon = icons[static_cast<int>(tab)];
    const auto iconPos = lo + ImVec2(20,5)*scale;
    draw->AddText(iconPos,active ? lite_colors::Accent : IM_COL32(58,58,58,255),icon);
    draw->AddText(iconPos + ImVec2(ImGui::CalcTextSize(icon).x + 6.f*lite_widgets::scale,0),
        active ? lite_colors::TextBright : lite_colors::TextDim, label);
    if (pressed) { state.tab = tab; keyboard = false; }
    ImGui::SetCursorScreenPos(lo+ImVec2(0,31.f*scale));
    return pressed;
}

inline void Sidebar(State& state, const View& view) {
    const auto scale = lite_widgets::scale;
    auto* draw = ImGui::GetWindowDrawList();
    auto* titleFont = view.titleFont ? view.titleFont : ImGui::GetFont();
    const auto lo = ImGui::GetCursorScreenPos();
    const float width=ImGui::GetContentRegionAvail().x;
    const auto original=titleFont->CalcTextSizeA(30.f*scale,FLT_MAX,0,"ethnir lite");
    const float titleSize=30.f*scale*ImMin(1.f,ImMax(1.f,width-28.f*scale)/original.x);
    const auto measured=titleFont->CalcTextSizeA(titleSize,FLT_MAX,0,"ethnir lite");
    draw->AddText(titleFont,titleSize,{lo.x+(width-measured.x)*.5f,lo.y+44.f*scale-measured.y*.5f},
        lite_colors::Accent,"ethnir lite");
    ImGui::SetCursorScreenPos(lo+ImVec2(0,92.f*scale));
    SideTab("home", Tab::Home, state);
    const auto group=[&](const char* label) {
        const auto pos=ImGui::GetCursorScreenPos()+ImVec2(0,4.f*scale);
        draw->AddText(pos+ImVec2(14.f*scale,0),lite_colors::TextDim,label);
        ImGui::SetCursorScreenPos(pos+ImVec2(0,ImGui::GetTextLineHeight()+6.f*scale));
    };
    group("visuals");
    SideTab("players", Tab::Esp, state);
    group("combat");
    SideTab("aimbot", Tab::Aim, state);
    SideTab("trigger", Tab::Trigger, state);
    group("misc");
    SideTab("memory", Tab::Memory, state);
    SideTab("skins", Tab::Skins, state);
    group("system");
    SideTab("configs", Tab::Settings, state);
    ImGui::Dummy({0,6.f*scale});
    const auto hide=ImGui::GetCursorScreenPos();
    if(lite_widgets::Tap(ImGui::InvisibleButton("##hide_lite",{width,26.f*scale}))) { state.minimized=true;keyboard=false; }
    draw->AddText(hide+ImVec2(20,5)*scale,lite_colors::TextDim,"hide menu");
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
    const auto layout=Fit(io.DisplaySize);
    const auto pos=state.placement.Position(io.DisplaySize,layout.size);
    const ImRect header(pos,pos+ImVec2(layout.sidebar,92.f*layout.scale*state.zoom));
    state.placement.Update(ImGui::GetFrameCount(),view.ready && view.interactive && !state.minimized && !state.restore.Captures(),
        io.DisplaySize,layout.size,header,io.MousePos,io.MouseClicked[0],io.MouseDown[0],ImMax(7.f,7.f*layout.scale));
    if(state.placement.Tracking()) custom::CancelTouchScroll();
}

inline Actions Draw(State& state, const View& view, char* key, std::size_t capacity) {
    Actions actions;
    Theme();
    const auto layout = Fit(ImGui::GetIO().DisplaySize);
    lite_widgets::scale = layout.scale * state.zoom;
    c::scale = lite_widgets::scale;
    PreparePlacement(state,view);
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
        const unsigned alpha=static_cast<unsigned>(160.f+95.f*state.restore.Progress());
        ImGui::GetWindowDrawList()->AddRectFilled(restore.line.Min,restore.line.Max,
            IM_COL32(255,255,255,alpha),restore.line.GetHeight()*.5f);
        ImGui::End();
        ImGui::PopStyleVar();
        return actions;
    }

    if (!view.ready) {
        const auto display = ImGui::GetIO().DisplaySize;
        const bool error = view.error && *view.error;
        const ImVec2 size(ImMin(view.authorized?340.f:380.f,display.x-24.f),
            ImMin(view.authorized?288.f:(error?344.f:302.f),display.y-24.f));
        const float alpha = view.authorized && view.startup.active && !view.startup.reducedMotion ?
            lite_startup::Ease(float(view.startup.elapsed/lite_startup::introFadeSeconds))*(1.f-lite_startup::Ease((view.startup.finish-.88f)/.12f)) : 1.f;
        ImGui::SetNextWindowPos(display*.5f, ImGuiCond_Always, {.5f,.5f});
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {24,22});
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha*alpha);
        ImGui::Begin("ETHNIR LITE##login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowFontScale(1.f);
        lite_widgets::scale = 1.f;
        lite_startup::Card();
        if (view.authorized) lite_startup::Loading(view.titleFont,view.preparation,view.startup);
        else {
            const auto pos = ImGui::GetWindowPos();
            const ImVec2 center(pos.x+size.x*.5f,pos.y);
            lite_startup::Centered(view.titleFont,26,{center.x,pos.y+23},"ETHNIR",lite_startup::Ink(242));
            lite_startup::Edition(center,pos.y+56);
            lite_startup::Centered(nullptr,12,{center.x,pos.y+80},view.pending?"Checking your access":"Sign in to continue",lite_startup::Ink(136));
            ImGui::SetCursorPos({24,105});
            ImGui::BeginDisabled(view.pending);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{12,10});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,6.f);
            Input("Access key",key,capacity);
            ImGui::PopStyleVar(2);
            ImGui::SetCursorPos({24,171});
            const float half = (ImGui::GetContentRegionAvail().x-8.f)*.5f;
            actions.paste = lite_startup::Secondary("Paste key",{half,28});
            ImGui::SameLine(0,8);
            actions.loadKey = lite_startup::Secondary("Load saved",{half,28});
            ImGui::EndDisabled();
            ImGui::SetCursorPos({24,211});
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled,view.pending);
            actions.submit = lite_startup::Primary("Authenticate",view.pending,view.startup.reducedMotion,
                {ImGui::GetContentRegionAvail().x,38});
            ImGui::PopItemFlag();
            if (error) {
                ImGui::SetCursorPos({24,263});
                ImGui::BeginChild("##login_error",{size.x-48,ImMax(20.f,size.y-275)},false,
                    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
                ImGui::TextWrapped("%s",view.error);
                ImGui::EndChild();
            } else lite_startup::Centered(nullptr,11,{center.x,pos.y+270},"Secure access",lite_startup::Ink(100));
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
        ImGui::Begin("ETHNIR LITE##menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        lite_widgets::SetFontScale();
        const auto pos = ImGui::GetWindowPos();
        lite_sidebar::Surface(pos,layout.size,layout.sidebar,view.sidebarTexture,view.sidebarUv0,view.sidebarUv1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
        ImGui::BeginChild("##sidebar", {layout.sidebar,layout.size.y}, false,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
        lite_widgets::SetFontScale();
        Sidebar(state,view);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::SameLine(0,0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(layout.narrow ? 12.f : 26.f,12.f)*layout.scale);
        ImGui::BeginChild("##content", {0,layout.size.y}, false,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
        lite_widgets::SetFontScale();
        const int columns = layout.narrow || state.zoom > 1.1f ? 1 : 2;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(13,4)*layout.scale);
        if (ImGui::BeginTable("##panels", columns, ImGuiTableFlags_SizingStretchSame)) {
        switch (state.tab) {
            case Tab::Home:
                Panel("session", 180, [&] {
                    ImGui::Text("%.0f FPS",view.fps);
                    ImGui::Text("Uptime %02llu:%02llu:%02llu",
                        static_cast<unsigned long long>(view.uptime/3600),
                        static_cast<unsigned long long>(view.uptime/60%60),
                        static_cast<unsigned long long>(view.uptime%60));
                    ImGui::TextUnformatted(view.plan);
                    ImGui::TextWrapped("Expires %s",view.expiry);
                });
                Panel("music", 330, [&] {
                    lite_widgets::Checkbox("Music player",&state.music);
                    if (state.music && view.music) view.music();
                    else ImGui::TextWrapped("Enable to browse local or Cloud songs.");
                });
                break;
            case Tab::Esp:
                Panel("players", 420, [&] {
                    lite_widgets::Checkbox("ESP Line",&Config.ESPMenu.isPlayerLine);
                    lite_widgets::Checkbox("ESP Box",&Config.ESPMenu.Box);
                    lite_widgets::Checkbox("ESP Skeleton",&Config.ESPMenu.Skeleton);
                    lite_widgets::Checkbox("ESP Health",&Config.ESPMenu.Health);
                    lite_widgets::Checkbox("ESP Name",&Config.ESPMenu.Name);
                    lite_widgets::Checkbox("ESP Distance",&Config.ESPMenu.Distance);
                    lite_widgets::Checkbox("ESP Count",&Config.ESPMenu.Count);
                    lite_widgets::Checkbox("360 Alert",&Config.ESPMenu.Alert);
                    lite_widgets::Checkbox("BRClass",&Config.ESPMenu.BRClass);
                    lite_widgets::Checkbox("Show AimLine",&Config.ESPMenu.Aimline);
                    lite_widgets::Checkbox("Yellow Wallhack",&Config.ExtraMenu.WallHack);
                    lite_widgets::Checkbox("Red Wallhack",&Config.ExtraMenu.RedWallhack);
                });
                Panel("display options", 420, [&] {
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
                break;
            case Tab::Aim:
                Panel("aimbot", 300, [&] {
                    lite_widgets::Checkbox("Aimbot 360",&Config.Aim.Aimbot360);
                    lite_widgets::Checkbox("Bullet Track",&Config.Aim.AimSilent);
                    lite_widgets::SliderFloat("Aim Assist Size",&Config.Aim.AimAssistSize,0,100,"%.0f");
                    lite_widgets::Checkbox("Show FOV",&Config.ESPMenu.ShowFov);
                });
                Panel("combat options", 300, [&] {
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
                Panel("triggerbot", 300, [&] {
                    auto settings=PhantomTrigger::GetSettings();
                    const bool changed=lite_widgets::Checkbox("Triggerbot",&settings.enabled);
                    static const char* modes[]{"Both","ADS Only"};
                    int mode=settings.mode==PhantomTrigger::Mode::ADSOnly?1:0;
                    Combo("Mode",mode,modes,2); settings.mode=mode?PhantomTrigger::Mode::ADSOnly:PhantomTrigger::Mode::Both;
                    PhantomTrigger::SetSettings(settings);
                    actions.retryTrigger=changed&&settings.enabled;
                    ImGui::TextWrapped("%s",PhantomTrigger::DecisionStatusText(PhantomTrigger::GetDecisionStatus()));
                    if (lite_widgets::Button("Retry setup",{0,0})) actions.retryTrigger=true;
                    if(settings.enabled) RenderTriggerDiagnostics(PhantomTrigger::GetDecisionStatus());
                });
                Panel("crosshair", 300, [&] {
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
                Panel("movement", 350, [] { memory_tab::Movement(); });
                Panel("movement tuning", 350, [] { memory_tab::MovementTuning(); });
                Panel("weapon", 420, [] { memory_tab::WeaponControl(); memory_tab::WeaponHandling(); });
                Panel("world", 320, [] { memory_tab::Protection(); memory_tab::Camera(); });
                break;
            case Tab::Skins:
                Panel("blueprints", 150, [] { memory_tab::Blueprints(); });
                Panel("category", 150, [&] {
                    static const char* categories[]{"Character","Watch","Deadbox","Plane","Weapon","Camo","Snowboard"};
                    Combo("Skin category",state.skinCategory,categories,7);
                });
                break;
            case Tab::Settings:
                Panel("configs", 230, [&] {
                    Input("Configuration name",state.configName,sizeof(state.configName));
                    actions.saveConfig=lite_widgets::Button("Save configuration",{0,0});
                    ImGui::Spacing();
                    actions.loadConfig=lite_widgets::Button("Load configuration",{0,0});
                    if (view.error&&*view.error) ImGui::TextWrapped("%s",view.error);
                });
                Panel("menu", 230, [&] {
                    lite_widgets::SliderFloat("Menu scale",&state.zoom,.85f,1.35f,"%.2fx");
                    lite_widgets::Checkbox("Hide Record",&state.hideRecord);
                    lite_widgets::Checkbox("Clear Display",&Config.ExtraMenu.ClearDisplay);
                    lite_widgets::Checkbox("Reset Guest",&Config.ExtraMenu.ResetGuest);
                    if (lite_widgets::Button("Hide menu",{0,0})) state.minimized=true;
                });
                Panel("frame rate", 240, [] {
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
        ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        if (state.tab == Tab::Skins && view.skins) view.skins(state.skinCategory);
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
        if (view.startup.reveal < 1.f) for (auto* window : GImGui->Windows) {
            if (!window->Active || std::strcmp(window->RootWindow->Name,"ETHNIR LITE##menu")) continue;
            for (auto& vertex : window->DrawList->VtxBuffer) {
                const unsigned alpha = static_cast<unsigned>((vertex.col>>24)*view.startup.reveal);
                vertex.col = (vertex.col&0x00ffffffu)|(alpha<<24);
            }
        }
    }
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
