#pragma once

#include <iomanip>
#include "../Hooks/FrameRate.h"
#include "../Hooks/SlideFatigue.h"
#include <sstream>
#include <ctime>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <cctype>
#include <cerrno>
#include "FileUtils.h"

#include "../../foxcheats/include/json.hpp"
#include "../../Trigger/TriggerRuntime.h"
#include <array>
#include <algorithm>
#include "../../Hacks/Feature/Init_Hacks.h"
#include "../../BlueprintTierFilter.h"
#include "../../Trigger/TriggerDotStyle.h"
#include "../Hooks/Hitbox.h"

// Externals defined in Main.cpp
extern float menu[4];
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;




std::string androidFilesDir = "/storage/emulated/0/Android/data/com.garena.game.codm/files/";

inline nlohmann::json CaptureConfiguration() {
    nlohmann::json config;
    const auto frames=frame_rate::Configured(Config.Aim.FpsLevel,Config.Aim.FpsLevel1,Config.Aim.FpsLevelUltra,Config.Aim.FpsLevelUltra1);
    config["FrameRate"]["Custom"]=frames.custom;
    config["FrameRate"]["Regular"]=frames.regular;
    config["FrameRate"]["Ultra"]=frames.ultra;
    config["FrameRate"]["High"]=frames.high;

    config["ESPMenu"]["isPlayerLine"] = Config.ESPMenu.isPlayerLine;
    config["ESPMenu"]["Skeleton"] = Config.ESPMenu.Skeleton;
    config["ESPMenu"]["Count"] = Config.ESPMenu.Count;
    config["ESPMenu"]["Aimline"] = Config.ESPMenu.Aimline;
    config["ESPMenu"]["Alert"] = Config.ESPMenu.Alert;
    config["ESPMenu"]["BRClass"] = Config.ESPMenu.BRClass;
    config["ESPMenu"]["ShowFov"] = Config.ESPMenu.ShowFov;
    config["ESPMenu"]["Box"] = Config.ESPMenu.Box;
    config["ESPMenu"]["Name"] = Config.ESPMenu.Name;
    config["ESPMenu"]["Health"] = Config.ESPMenu.Health;
    config["ESPMenu"]["Distance"] = Config.ESPMenu.Distance;
    config["ESPMenu"]["Target"] = static_cast<int>(Config.ESPMenu.Target);
    config["ESPMenu"]["BoxType"] = static_cast<int>(Config.ESPMenu.BoxType);
    config["ESPMenu"]["HealthPosition"] = static_cast<int>(Config.ESPMenu.HealthPosition);
    config["ESPMenu"]["CrosshairType"] = static_cast<int>(Config.ESPMenu.CrosshairType);
    config["ESPMenu"]["EspStyle"] = static_cast<int>(Config.ESPMenu.EspStyle);
    
    config["Aim"]["Aimbot360"] = Config.Aim.Aimbot360;
    config["Aim"]["AimSilent"] = Config.Aim.AimSilent;
    config["Aim"]["AimAssistSize"] = Config.Aim.AimAssistSize;
    config["Aim"]["Target"] = static_cast<int>(Config.Aim.Target);
    config["Aim"]["Trigger"] = static_cast<int>(Config.Aim.Trigger);
    config["Aim"]["By"] = static_cast<int>(Config.Aim.By);
    config["Aim"]["size"] = Config.Aim.size;
    config["Aim"]["Cross"] = Config.Aim.Cross;

    config["ExtraMenu"]["SmartReload"] = Config.ExtraMenu.SmartReload;
    config["ExtraMenu"]["ClearTerrain"] = Config.ExtraMenu.ClearTerrain;
    config["ExtraMenu"]["NoSmoke"] = Config.ExtraMenu.NoSmoke;
    config["ExtraMenu"]["RedWallhack"] = Config.ExtraMenu.RedWallhack;
    config["ExtraMenu"]["WallHack"] = Config.ExtraMenu.WallHack;
    config["ExtraMenu"]["AutoJumpFire"] = Config.ExtraMenu.AutoJumpFire;
    config["ExtraMenu"]["CameraPov"] = Config.ExtraMenu.CameraPov;
    config["ExtraMenu"]["CameraPovSize"] = Config.ExtraMenu.CameraPovSize;
    config["ExtraMenu"]["UnlimitedFly"] = Config.ExtraMenu.UnlimitedFly;
    config["ExtraMenu"]["FlySpeed"] = Config.ExtraMenu.FlySpeed;
    config["ExtraMenu"]["ClearDisplay"] = Config.ExtraMenu.ClearDisplay;
    config["ExtraMenu"]["Flash"] = Config.ExtraMenu.Flash;
    config["ExtraMenu"]["Diving"] = Config.ExtraMenu.Diving;
    config["ExtraMenu"]["Fire"] = Config.ExtraMenu.Fire;
    config["ExtraMenu"]["NoSprintFire"] = Config.ExtraMenu.NoSprintFire;
    config["ExtraMenu"]["Hit"] = Config.ExtraMenu.Hit;
    config["ExtraMenu"]["HitboxScale"] = hitbox::NormalizeScale(Config.ExtraMenu.HitboxScale);
    config["ExtraMenu"]["Parachute"] = Config.ExtraMenu.Parachute;
    config["ExtraMenu"]["NoCrouch"] = Config.ExtraMenu.NoCrouch;
    config["ExtraMenu"]["NoSlideFatigue"] = Config.ExtraMenu.NoSlideFatigue;
    config["ExtraMenu"]["Recoil"] = Config.ExtraMenu.Recoil;
    config["ExtraMenu"]["Spread"] = Config.ExtraMenu.Spread;
    config["ExtraMenu"]["Reload"] = Config.ExtraMenu.Reload;
    config["ExtraMenu"]["Scope"] = Config.ExtraMenu.Scope;
    config["ExtraMenu"]["Switch"] = Config.ExtraMenu.Switch;
    config["ExtraMenu"]["KineticArmor"] = Config.ExtraMenu.Kinetic;
    config["ExtraMenu"]["Blueprints"] = Config.ExtraMenu.Blueprints;
    config["ExtraMenu"]["BlueprintTiers"] = BlueprintTierFilter::GetMask();
    auto& dotStyle = TriggerDotStyle::appearance;
    TriggerDotStyle::Normalize(dotStyle);
    config["TriggerDot"]["Radius"] = dotStyle.radius;
    config["TriggerDot"]["Outline"] = dotStyle.outline;
    config["TriggerDot"]["Opacity"] = dotStyle.opacity;
    config["TriggerDot"]["Red"] = dotStyle.color[0];
    config["TriggerDot"]["Green"] = dotStyle.color[1];
    config["TriggerDot"]["Blue"] = dotStyle.color[2];

    config["isJumpAdjustmentEnabled"] = isJumpAdjustmentEnabled;
    config["jumpHeightMultiplier"] = jumpHeightMultiplier;
    config["SnowB"] = SnowB;
    config["SnowBsize"] = SnowBsize;
    config["isSpeedHackEnabled"] = isSpeedHackEnabled;
    config["speedHackMultiplier"] = speedHackMultiplier;

    const auto trigger = PhantomTrigger::GetSettings();
    config["Trigger"]["enabled"] = trigger.enabled;
    config["Trigger"]["showDot"] = trigger.showDot;
    config["Trigger"]["mode"] = trigger.mode == PhantomTrigger::Mode::ADSOnly ? 1 : 0;
    const auto colors = [](const float* rgba) { return nlohmann::json::array({rgba[0],rgba[1],rgba[2],rgba[3]}); };
    if (Config.sColorsESPPLAYER.LinePLAYER) config["ESPColors"]["Player"] = colors(Config.sColorsESPPLAYER.LinePLAYER);
    if (Config.sColorsESPBOT.LineBOT) config["ESPColors"]["Bot"] = colors(Config.sColorsESPBOT.LineBOT);
    config["menu"][0] = menu[0];
    config["menu"][1] = menu[1];
    config["menu"][2] = menu[2];

    return config;
}

inline bool ApplyConfiguration(const nlohmann::json& config) {
    try {
        if (config.contains("ESPMenu")) {
            const auto& esp = config["ESPMenu"];
            Config.ESPMenu.isPlayerLine = esp.value("isPlayerLine",Config.ESPMenu.isPlayerLine);
            Config.ESPMenu.Skeleton = esp.value("Skeleton",Config.ESPMenu.Skeleton);
            Config.ESPMenu.Count = esp.value("Count",Config.ESPMenu.Count);
            Config.ESPMenu.Aimline = esp.value("Aimline",Config.ESPMenu.Aimline);
            Config.ESPMenu.Alert = esp.value("Alert",Config.ESPMenu.Alert);
            Config.ESPMenu.BRClass = esp.value("BRClass",Config.ESPMenu.BRClass);
            Config.ESPMenu.ShowFov = esp.value("ShowFov",Config.ESPMenu.ShowFov);
            Config.ESPMenu.Box = config["ESPMenu"]["Box"].get<bool>();
            Config.ESPMenu.Name = config["ESPMenu"]["Name"].get<bool>();
            Config.ESPMenu.Health = config["ESPMenu"]["Health"].get<bool>();
            Config.ESPMenu.Distance = config["ESPMenu"]["Distance"].get<bool>();
            if (config["ESPMenu"].contains("Target")) Config.ESPMenu.Target = static_cast<LineTarget>(config["ESPMenu"]["Target"].get<int>());
            if (config["ESPMenu"].contains("BoxType")) Config.ESPMenu.BoxType = static_cast<EspBoxType>(config["ESPMenu"]["BoxType"].get<int>());
            if (config["ESPMenu"].contains("HealthPosition")) Config.ESPMenu.HealthPosition = static_cast<EspHealthPosition>(config["ESPMenu"]["HealthPosition"].get<int>());
            if (config["ESPMenu"].contains("CrosshairType")) Config.ESPMenu.CrosshairType = static_cast<CrosshairTarget>(config["ESPMenu"]["CrosshairType"].get<int>());
            if (config["ESPMenu"].contains("EspStyle")) Config.ESPMenu.EspStyle = static_cast<EspStyleTarget>(config["ESPMenu"]["EspStyle"].get<int>());
        }

        if (config.contains("Aim")) {
            Config.Aim.Cross = std::clamp(config["Aim"].value("Cross",Config.Aim.Cross),0.f,100.f);
            Config.Aim.Aimbot360 = config["Aim"]["Aimbot360"].get<bool>();
            Config.Aim.AimSilent = config["Aim"]["AimSilent"].get<bool>();
            Config.Aim.AimAssistSize = config["Aim"]["AimAssistSize"].get<float>();
            Config.Aim.Target = static_cast<EAimTarget>(config["Aim"]["Target"].get<int>());
            Config.Aim.Trigger = static_cast<EAimTrigger>(config["Aim"]["Trigger"].get<int>());
            Config.Aim.By = static_cast<EAim>(config["Aim"]["By"].get<int>());
            Config.Aim.size = config["Aim"]["size"].get<float>();
        }

        if (config.contains("ExtraMenu")) {
            const auto& extra = config["ExtraMenu"];
            Config.ExtraMenu.SmartReload = extra.value("SmartReload",Config.ExtraMenu.SmartReload);
            Config.ExtraMenu.ClearTerrain = extra.value("ClearTerrain",Config.ExtraMenu.ClearTerrain);
            Config.ExtraMenu.NoSmoke = extra.value("NoSmoke",Config.ExtraMenu.NoSmoke);
            Config.ExtraMenu.RedWallhack = extra.value("RedWallhack",Config.ExtraMenu.RedWallhack);
            Config.ExtraMenu.WallHack = extra.value("WallHack",Config.ExtraMenu.WallHack);
            Config.ExtraMenu.AutoJumpFire = extra.value("AutoJumpFire",Config.ExtraMenu.AutoJumpFire);
            Config.ExtraMenu.CameraPov = extra.value("CameraPov",Config.ExtraMenu.CameraPov);
            Config.ExtraMenu.CameraPovSize = std::clamp(extra.value("CameraPovSize",Config.ExtraMenu.CameraPovSize),0.f,150.f);
            Config.ExtraMenu.UnlimitedFly = extra.value("UnlimitedFly",Config.ExtraMenu.UnlimitedFly);
            Config.ExtraMenu.FlySpeed = std::clamp(extra.value("FlySpeed",Config.ExtraMenu.FlySpeed),1.f,10.f);
            Config.ExtraMenu.ClearDisplay = extra.value("ClearDisplay",Config.ExtraMenu.ClearDisplay);
            Config.ExtraMenu.Flash = config["ExtraMenu"]["Flash"].get<bool>();
            Config.ExtraMenu.Diving = config["ExtraMenu"]["Diving"].get<bool>();
            Config.ExtraMenu.Fire = config["ExtraMenu"]["Fire"].get<bool>();
            Config.ExtraMenu.NoSprintFire = config["ExtraMenu"].value("NoSprintFire", false);
            Config.ExtraMenu.Hit = config["ExtraMenu"]["Hit"].get<bool>();
            Config.ExtraMenu.HitboxScale = hitbox::NormalizeScale(config["ExtraMenu"].value("HitboxScale",hitbox::defaultScale));
            hitbox::Publish(Config.ExtraMenu.Hit,Config.ExtraMenu.HitboxScale);
            Config.ExtraMenu.Rpd = false;
            Config.ExtraMenu.Parachute = config["ExtraMenu"]["Parachute"].get<bool>();
            Config.ExtraMenu.NoCrouch = config["ExtraMenu"].value("NoCrouch", false);
            Config.ExtraMenu.NoSlideFatigue = config["ExtraMenu"].value("NoSlideFatigue", false);
            slide_fatigue::Publish(Config.ExtraMenu.NoSlideFatigue);
            Config.ExtraMenu.Recoil = config["ExtraMenu"]["Recoil"].get<bool>();
            Config.ExtraMenu.Shake = false;
            Config.ExtraMenu.Spread = config["ExtraMenu"]["Spread"].get<bool>();
            Config.ExtraMenu.Reload = config["ExtraMenu"]["Reload"].get<bool>();
            Config.ExtraMenu.Scope = config["ExtraMenu"]["Scope"].get<bool>();
            Config.ExtraMenu.Switch = config["ExtraMenu"]["Switch"].get<bool>();
            Config.ExtraMenu.Kinetic = config["ExtraMenu"].value("Kinetic",config["ExtraMenu"].value("KineticArmor",false));
            if (config["ExtraMenu"].contains("Blueprints"))
                Config.ExtraMenu.Blueprints = config["ExtraMenu"]["Blueprints"].get<bool>();
            if (config["ExtraMenu"].contains("BlueprintTiers"))
                BlueprintTierFilter::SetMask(config["ExtraMenu"]["BlueprintTiers"].get<uint32_t>());
        }

        if (config.contains("TriggerDot") && config["TriggerDot"].is_object()) {
            auto& dotStyle = TriggerDotStyle::appearance;
            const auto& stored = config["TriggerDot"];
            if (stored.contains("Radius") && stored["Radius"].is_number())
                dotStyle.radius = stored["Radius"].get<float>();
            if (stored.contains("Outline") && stored["Outline"].is_number())
                dotStyle.outline = stored["Outline"].get<float>();
            if (stored.contains("Opacity") && stored["Opacity"].is_number())
                dotStyle.opacity = stored["Opacity"].get<float>();
            if (stored.contains("Red") && stored["Red"].is_number())
                dotStyle.color[0] = stored["Red"].get<float>();
            if (stored.contains("Green") && stored["Green"].is_number())
                dotStyle.color[1] = stored["Green"].get<float>();
            if (stored.contains("Blue") && stored["Blue"].is_number())
                dotStyle.color[2] = stored["Blue"].get<float>();
            TriggerDotStyle::Normalize(dotStyle);
        }

        if(config.contains("FrameRate")&&config["FrameRate"].is_object()) {
            const auto& stored=config["FrameRate"];
            auto frames=frame_rate::Configured(stored.value("Custom",false),stored.value("Regular",60.f),stored.value("Ultra",false),stored.value("High",240.f));
            Config.Aim.FpsLevel=frames.custom;Config.Aim.FpsLevel1=frames.regular;
            Config.Aim.FpsLevelUltra=frames.ultra;Config.Aim.FpsLevelUltra1=frames.high;
            frame_rate::Publish(frames);
        }
        if (config.contains("isJumpAdjustmentEnabled"))
            isJumpAdjustmentEnabled = config["isJumpAdjustmentEnabled"].get<bool>();
        if (config.contains("jumpHeightMultiplier"))
            jumpHeightMultiplier = config["jumpHeightMultiplier"].get<float>();
        if (config.contains("SnowB"))
            SnowB = config["SnowB"].get<bool>();
        if (config.contains("SnowBsize"))
            SnowBsize = config["SnowBsize"].get<float>();
        if (config.contains("isSpeedHackEnabled"))
            isSpeedHackEnabled = config["isSpeedHackEnabled"].get<bool>();
        if (config.contains("speedHackMultiplier"))
            speedHackMultiplier = config["speedHackMultiplier"].get<float>();

        if (config.contains("Trigger") && config["Trigger"].is_object()) {
            auto trigger = PhantomTrigger::GetSettings();
            const auto& saved = config["Trigger"];
            trigger.enabled = saved.value("enabled",trigger.enabled);
            trigger.showDot = saved.value("showDot",trigger.showDot);
            trigger.mode = saved.value("mode",0) == 1 ? PhantomTrigger::Mode::ADSOnly : PhantomTrigger::Mode::Both;
            PhantomTrigger::SetSettings(trigger);
        }
        if (config.contains("ESPColors") && config["ESPColors"].is_object()) {
            const auto restore = [](const nlohmann::json& value, std::array<float*,6> linked) {
                if (!value.is_array() || value.size()!=4) return;
                for (size_t i=0;i<4;++i) if (!value[i].is_number()) return;
                for (float* rgba : linked) if (rgba) for (size_t i=0;i<4;++i)
                    rgba[i] = std::clamp(value[i].get<float>(),0.f,1.f);
            };
            const auto& saved = config["ESPColors"];
            if (saved.contains("Player")) restore(saved["Player"],{Config.sColorsESPPLAYER.LinePLAYER,Config.sColorsESPPLAYER.BoxPLAYER,Config.sColorsESPPLAYER.NamePLAYER,Config.sColorsESPPLAYER.HealthPLAYER,Config.sColorsESPPLAYER.DistancePLAYER,Config.sColorsESPPLAYER.SkeletonPLAYER});
            if (saved.contains("Bot")) restore(saved["Bot"],{Config.sColorsESPBOT.LineBOT,Config.sColorsESPBOT.BoxBOT,Config.sColorsESPBOT.NameBOT,Config.sColorsESPBOT.HealthBOT,Config.sColorsESPBOT.DistanceBOT,Config.sColorsESPBOT.SkeletonBOT});
        }
        if (config.contains("menu")) {
            menu[0] = config["menu"][0].get<float>();
            menu[1] = config["menu"][1].get<float>();
            menu[2] = config["menu"][2].get<float>();
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

inline std::string ConfigStorageDirectory() {
    return androidFilesDir + "configs";
}

inline bool ConfigFileNameValid(const std::string& name) {
    if (name.empty() || name.size() > 48) return false;
    for (unsigned char ch : name) {
        if (!std::isalnum(ch) && ch != '-' && ch != '_') return false;
    }
    return true;
}

inline bool SaveConfigurationFile(const std::string& name, const nlohmann::json& value) {
    if (!ConfigFileNameValid(name)) return false;
    const std::string dir = ConfigStorageDirectory();
    if (dir.empty() || (mkdir(dir.c_str(), 0700) != 0 && errno != EEXIST)) return false;
    const std::string path = dir + "/" + name + ".json";
    const std::string temp = path + ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) return false;
        out << value.dump(2);
        out.flush();
        if (!out.good()) { out.close(); std::remove(temp.c_str()); return false; }
    }
    if (std::rename(temp.c_str(), path.c_str()) != 0) {
        std::remove(temp.c_str());
        return false;
    }
    return true;
}

inline void SaveConfiguration(const std::string& filename) {
    (void)SaveConfigurationFile(filename, CaptureConfiguration());
}

inline bool LoadConfiguration(const std::string& filename) {
    if (!ConfigFileNameValid(filename)) return false;
    const std::string dir = ConfigStorageDirectory();
    if (dir.empty()) return false;
    std::ifstream input(dir + "/" + filename + ".json", std::ios::binary);
    if (!input.is_open()) {
        const std::string legacy = "/storage/emulated/0/Android/data/com.garena.game.codm/files/configs/" + filename + ".json";
        input.open(legacy, std::ios::binary);
    }
    if (!input.is_open()) return false;
    try {
        nlohmann::json config;
        input >> config;
        return ApplyConfiguration(config);
    } catch (const std::exception&) {
        return false;
    }
}
