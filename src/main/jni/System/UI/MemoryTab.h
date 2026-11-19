#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/custom_widgets.hpp"
#include "../../ImGui/TouchScroll.h"
#include "../../BlueprintTierFilter.h"
#include "../Hooks/SlideFatigue.h"
#include "../Hooks/Hitbox.h"
#include "../Hooks/PickupTweaks.h"
#include "../Audio/UiSounds.h"
#include <algorithm>

extern float SlideRange,SnowBsize,speedHackMultiplier,jumpHeightMultiplier,MpBsize;

namespace memory_tab {
enum class Category { Movement,Weapons,Protection,Blueprints,Worlds };
inline Category selected=Category::Movement;
inline constexpr const char* labels[]{"Movement","Weapon","Protection & view","Blueprints","Worlds"};
inline bool CategoryButton(int index,ImVec2 size) {
    ImGui::PushID(index);
    const auto lo=ImGui::GetCursorScreenPos();
    const bool raw=ImGui::InvisibleButton("##category",size);
    const bool pressed=raw&&(!custom::touch::Applies()||custom::touch::Tap());
    const bool active=static_cast<int>(selected)==index;
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo,lo+size,ImGui::GetColorU32(active?ImVec4(c::accent.x,c::accent.y,c::accent.z,.25f):ImVec4(.14f,.13f,.19f,.80f)),10.f*c::scale);
    draw->AddRect(lo,lo+size,ImGui::GetColorU32(active?ImVec4(c::accent.x,c::accent.y,c::accent.z,.65f):ImVec4(.8f,.77f,.9f,.14f)),10.f*c::scale);
    auto* font=ImGui::GetFont();float fontSize=ImGui::GetFontSize();
    const auto measured=font->CalcTextSizeA(fontSize,FLT_MAX,0,labels[index]);
    fontSize*=ImMin(1.f,ImMax(1.f,size.x-16.f*c::scale)/ImMax(1.f,measured.x));
    const auto textSize=font->CalcTextSizeA(fontSize,FLT_MAX,0,labels[index]);
    draw->AddText(font,fontSize,lo+(size-textSize)*.5f,ImGui::GetColorU32(active?c::text::text_active:c::text::text_hov),labels[index]);
    if(pressed) {selected=static_cast<Category>(index);ui_sounds::Play(ui_sounds::Cue::Click);}
    ImGui::PopID();return pressed;
}
inline void Movement() {
    custom::Checkbox("No Crouch",&Config.ExtraMenu.NoCrouch);
    ImGui::BeginDisabled(!slide_fatigue::available.load());
    if(custom::Checkbox("No Slide Fatigue",&Config.ExtraMenu.NoSlideFatigue)) slide_fatigue::Publish(Config.ExtraMenu.NoSlideFatigue);
    ImGui::EndDisabled();
    if(!slide_fatigue::available.load()&&ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Unavailable for this game build");
    custom::Checkbox("No Sprint Fire Delay",&Config.ExtraMenu.NoSprintFire);
    custom::Checkbox("No Parachute",&Config.ExtraMenu.Parachute);
    custom::Checkbox("Fast Dive",&Config.ExtraMenu.Diving);
    custom::Checkbox("Unlimited Fly",&Config.ExtraMenu.UnlimitedFly);
}
inline void MovementTuning() {
    if(Config.ExtraMenu.UnlimitedFly) custom::SliderFloat("Fly Speed",&Config.ExtraMenu.FlySpeed,1.f,10.f,"%.0f");
    custom::SliderFloat("Snowboard Speed",&SnowBsize,0.f,100.f,"%.1f");
    custom::SliderFloat("Slide Distance",&SlideRange,0.f,30.f,"%.1f");
    custom::SliderFloat("Speed Multiplier",&speedHackMultiplier,.5f,2.f,"%.1fx");
    custom::SliderFloat("High Jump",&jumpHeightMultiplier,.5f,5.f,"%.2fx");
}
inline void WeaponControl() {
    custom::Checkbox("Hitbox",&Config.ExtraMenu.Hit);
    if(Config.ExtraMenu.Hit) {
        custom::SliderFloat("Hitbox Size",&Config.ExtraMenu.HitboxScale,hitbox::minimumScale,hitbox::maximumScale,"%.2fx");
        Config.ExtraMenu.HitboxScale=hitbox::NormalizeScale(Config.ExtraMenu.HitboxScale);
        if(!hitbox::available.load()) ImGui::TextDisabled("%s",hitbox::status.load()==hitbox::Status::Waiting?"Waiting for game hit checks":"Hitbox unavailable for this build");
        else if(!hitbox::sphereAvailable.load()) ImGui::TextDisabled("%s",hitbox::sphereStatus.load()==hitbox::Status::Waiting?"Sphere scaling pending":"Sphere scaling unavailable for this build");
    }
    hitbox::Publish(Config.ExtraMenu.Hit,Config.ExtraMenu.HitboxScale);
    custom::Checkbox("No Recoil",&Config.ExtraMenu.Recoil);
    custom::Checkbox("No Spread",&Config.ExtraMenu.Spread);
    custom::Checkbox("No Shake",&Config.ExtraMenu.Shake);
    custom::Checkbox("No Overheat",&Config.ExtraMenu.Rpd);
    custom::Checkbox("Fire Rate",&Config.ExtraMenu.Fire);
}
inline void WeaponHandling() {
    custom::Checkbox("Smart Reload",&Config.ExtraMenu.SmartReload);
    custom::Checkbox("Fast Reload",&Config.ExtraMenu.Reload);
    custom::Checkbox("Fast Scope",&Config.ExtraMenu.Scope);
    custom::Checkbox("Quick Switch",&Config.ExtraMenu.Switch);
    custom::Checkbox("Auto Jump Fire",&Config.ExtraMenu.AutoJumpFire);
    custom::Checkbox("Weapon Kinetic",&Config.ExtraMenu.Kinetic);
}
inline void Protection() {
    custom::Checkbox("Fast Armor Plate",&Config.ExtraMenu.FastArmorPlate);
    custom::Checkbox("Anti Flashbang",&Config.ExtraMenu.Flash);
    custom::Checkbox("No Smoke",&Config.ExtraMenu.NoSmoke);
    custom::Checkbox("Clear Terrain",&Config.ExtraMenu.ClearTerrain);
}
inline void World() {
    custom::Checkbox("Fast Pickup",&Config.ExtraMenu.FastPickup);
    custom::SliderFloat("Pickup Range",&Config.ExtraMenu.PickupRange,0.f,15.f,"%.1f");
    pickup_tweaks::Publish(Config.ExtraMenu.FastPickup,Config.ExtraMenu.PickupRange>0.f,Config.ExtraMenu.PickupRange);
}
inline void Camera() {
    custom::Checkbox("Camera POV",&Config.ExtraMenu.CameraPov);
    if(Config.ExtraMenu.CameraPov) custom::SliderFloat("Camera POV Size",&Config.ExtraMenu.CameraPovSize,0.f,150.f,"%.0f");
    custom::SliderFloat("MP POV",&MpBsize,0.f,150.f,"%.1f");
}
inline void Blueprints() {
    custom::Checkbox("Unlock Blueprints", &Config.ExtraMenu.Blueprints);
    ImGui::TextColored(c::accent, "BLUEPRINT UNLOCK FILTER");
    const std::string selectedTiers = BlueprintTierFilter::Preview();
    bool allBlueprints = BlueprintTierFilter::GetMask() == 0;
    bool mythicTier = BlueprintTierFilter::HasSelection(BlueprintTierFilter::Mythic);
    bool legendaryTier = BlueprintTierFilter::HasSelection(BlueprintTierFilter::Legendary);
    bool epicTier = BlueprintTierFilter::HasSelection(BlueprintTierFilter::Epic);
    bool commonTier = BlueprintTierFilter::HasSelection(BlueprintTierFilter::Common);
    const auto tierOptions = ImGuiSelectableFlags_DontClosePopups;
    if (custom::BeginCombo("Unlock tiers", selectedTiers.c_str(), 5)) {
        if (custom::Selectable("All blueprints", &allBlueprints, tierOptions, ImVec2(0, 0),
                custom::MenuItemDecoration::Ring(IM_COL32(185, 182, 198, 255))))
            BlueprintTierFilter::SetMask(0);
        if (custom::Selectable("Mythic", &mythicTier, tierOptions, ImVec2(0, 0),
                custom::MenuItemDecoration::Circle(IM_COL32(239, 77, 91, 255))))
            BlueprintTierFilter::Toggle(BlueprintTierFilter::Mythic);
        if (custom::Selectable("Legendary", &legendaryTier, tierOptions, ImVec2(0, 0),
                custom::MenuItemDecoration::Circle(IM_COL32(255, 208, 70, 255))))
            BlueprintTierFilter::Toggle(BlueprintTierFilter::Legendary);
        if (custom::Selectable("Epic", &epicTier, tierOptions, ImVec2(0, 0),
                custom::MenuItemDecoration::Circle(IM_COL32(181, 119, 245, 255))))
            BlueprintTierFilter::Toggle(BlueprintTierFilter::Epic);
        if (custom::Selectable("Common", &commonTier, tierOptions, ImVec2(0, 0),
                custom::MenuItemDecoration::Circle(IM_COL32(74, 222, 128, 255))))
            BlueprintTierFilter::Toggle(BlueprintTierFilter::Common);
        custom::EndCombo();
    }
}
inline void Section(const char* label) {
    ImGui::Dummy({0,8.f*c::scale});
    ImGui::TextColored(c::accent,"%s",label);
    ImGui::Separator();
}
inline void Draw(ImVec2 available) {
    if(available.x<=0||available.y<=0) return;
    ImGui::PushID("MemorySections");
    const float scale=ImClamp(c::scale,.8f,1.15f),gap=8.f*scale;
    constexpr int categoryCount=IM_ARRAYSIZE(labels);
    const int columns=available.x>=520.f*scale?4:2;
    const float buttonWidth=(available.x-(columns-1)*gap)/columns;
    const float buttonHeight=ImMax(34.f*scale,ImGui::GetTextLineHeight()+12.f*scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{gap,gap});
    for(int i=0;i<categoryCount;++i) {
        if(i%columns) ImGui::SameLine(0,gap);
        CategoryButton(i,{buttonWidth,buttonHeight});
    }
    ImGui::PopStyleVar();
    const int categoryRows=(categoryCount+columns-1)/columns;
    const float navHeight=categoryRows*buttonHeight+categoryRows*gap;
    const float bodyHeight=ImMax(32.f,available.y-navHeight);
    const float minimumPanel=ImGui::CalcTextSize("No Sprint Fire Delay").x+68.f*scale+ImGui::GetStyle().WindowPadding.x*2.f;
    const bool wide=available.x>=minimumPanel*2.f+gap&&selected!=Category::Blueprints&&selected!=Category::Worlds;
    const float panelWidth=wide?(available.x-gap)*.5f:available.x;
    const char* first=selected==Category::Movement?"MOVEMENT##memory_movement":selected==Category::Weapons?"WEAPON CONTROL##memory_weapons":selected==Category::Protection?"PROTECTION & VISIBILITY##memory_protection":selected==Category::Blueprints?"BLUEPRINTS##memory_blueprints":"WORLD##memory_world";
    ImGui::BeginGroup();
    custom::Child(first,{panelWidth,bodyHeight},true,ImGuiWindowFlags_NoScrollbar);
    switch(selected) {
        case Category::Movement: Movement();if(!wide) {Section("MOVEMENT TUNING");MovementTuning();}break;
        case Category::Weapons: WeaponControl();if(!wide) {Section("WEAPON");WeaponHandling();}break;
        case Category::Protection: Protection();if(!wide) {Section("CAMERA");Camera();}break;
        case Category::Blueprints: Blueprints();break;
        case Category::Worlds: World();break;
    }
    custom::EndChild();ImGui::EndGroup();
    if(wide) {
        ImGui::SameLine(0,gap);ImGui::BeginGroup();
        const char* second=selected==Category::Movement?"MOVEMENT TUNING##memory_tuning":selected==Category::Weapons?"WEAPON##memory_handling":"CAMERA##memory_camera";
        custom::Child(second,{panelWidth,bodyHeight},true,ImGuiWindowFlags_NoScrollbar);
        if(selected==Category::Movement) MovementTuning();else if(selected==Category::Weapons) WeaponHandling();else Camera();
        custom::EndChild();ImGui::EndGroup();
    }
    ImGui::PopID();
}
}
