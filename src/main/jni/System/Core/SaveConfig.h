#pragma once

#include <iomanip>
#include "../Hooks/FrameRate.h"
#include "../Hooks/Pickup.h"
#include <sstream>
#include <ctime>

#include "../foxcheats/include/json.hpp"
#include "../../Hacks/Feature/Init_Hacks.h"
#include "../../BlueprintTierFilter.h"

// Externals defined in Main.cpp
extern float menu[4];
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;

std::string androidFilesDir = "/storage/emulated/0/Android/data/com.garena.game.codm/files/";

bool SaveConfiguration(const std::string& filename) {
    nlohmann::json config;
    const auto frames=frame_rate::Configured(Config.Aim.FpsLevel,Config.Aim.FpsLevel1,Config.Aim.FpsLevelUltra,Config.Aim.FpsLevelUltra1);
    config["FrameRate"]["Custom"]=frames.custom;
    config["FrameRate"]["Regular"]=frames.regular;
    config["FrameRate"]["Ultra"]=frames.ultra;
    config["FrameRate"]["High"]=frames.high;

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

    config["ExtraMenu"]["Flash"] = Config.ExtraMenu.Flash;
    config["ExtraMenu"]["Diving"] = Config.ExtraMenu.Diving;
    config["ExtraMenu"]["Fire"] = Config.ExtraMenu.Fire;
    config["ExtraMenu"]["Rpd"] = Config.ExtraMenu.Rpd;
    config["ExtraMenu"]["Parachute"] = Config.ExtraMenu.Parachute;
    config["ExtraMenu"]["NoCrouch"] = Config.ExtraMenu.NoCrouch;
    config["ExtraMenu"]["FastPickup"] = Config.ExtraMenu.FastPickup;
    config["ExtraMenu"]["PickupRange"] = pickup::NormalizeRange(Config.ExtraMenu.PickupRange);
    config["ExtraMenu"]["Recoil"] = Config.ExtraMenu.Recoil;
    config["ExtraMenu"]["Shake"] = Config.ExtraMenu.Shake;
    config["ExtraMenu"]["Spread"] = Config.ExtraMenu.Spread;
    config["ExtraMenu"]["Hit"] = Config.ExtraMenu.Hit;
    config["ExtraMenu"]["Reload"] = Config.ExtraMenu.Reload;
    config["ExtraMenu"]["Scope"] = Config.ExtraMenu.Scope;
    config["ExtraMenu"]["Switch"] = Config.ExtraMenu.Switch;
    config["ExtraMenu"]["KineticArmor"] = Config.ExtraMenu.Kinetic;
    config["ExtraMenu"]["Blueprints"] = Config.ExtraMenu.Blueprints;
    config["ExtraMenu"]["BlueprintTiers"] = BlueprintTierFilter::GetMask();

    config["isJumpAdjustmentEnabled"] = isJumpAdjustmentEnabled;
    config["jumpHeightMultiplier"] = jumpHeightMultiplier;
    config["SnowB"] = SnowB;
    config["SnowBsize"] = SnowBsize;
    config["isSpeedHackEnabled"] = isSpeedHackEnabled;
    config["speedHackMultiplier"] = speedHackMultiplier;

    config["menu"][0] = menu[0];
    config["menu"][1] = menu[1];
    config["menu"][2] = menu[2];

    std::string configDir = androidFilesDir + "configs";
    mkdir(configDir.c_str(), 0777);

    std::string filePath = configDir + "/" + filename + ".json";
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    file << std::setw(4) << config;
    file.close();
    return !file.fail();
}

bool LoadConfiguration(const std::string& filename) {
    try {
        std::string filePath = androidFilesDir + "configs/" + filename + ".json";
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json config;
        file >> config;

        if (config.contains("ESPMenu")) {
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
            Config.Aim.Aimbot360 = config["Aim"]["Aimbot360"].get<bool>();
            Config.Aim.AimSilent = config["Aim"]["AimSilent"].get<bool>();
            Config.Aim.AimAssistSize = config["Aim"]["AimAssistSize"].get<float>();
            Config.Aim.Target = static_cast<EAimTarget>(config["Aim"]["Target"].get<int>());
            Config.Aim.Trigger = static_cast<EAimTrigger>(config["Aim"]["Trigger"].get<int>());
            Config.Aim.By = static_cast<EAim>(config["Aim"]["By"].get<int>());
            Config.Aim.size = config["Aim"]["size"].get<float>();
        }

        if (config.contains("ExtraMenu")) {
            Config.ExtraMenu.Flash = config["ExtraMenu"]["Flash"].get<bool>();
            Config.ExtraMenu.Diving = config["ExtraMenu"]["Diving"].get<bool>();
            Config.ExtraMenu.Fire = config["ExtraMenu"]["Fire"].get<bool>();
            Config.ExtraMenu.Rpd = config["ExtraMenu"]["Rpd"].get<bool>();
            Config.ExtraMenu.Parachute = config["ExtraMenu"]["Parachute"].get<bool>();
            Config.ExtraMenu.NoCrouch = config["ExtraMenu"].value("NoCrouch", false);
            Config.ExtraMenu.Recoil = config["ExtraMenu"]["Recoil"].get<bool>();
            Config.ExtraMenu.Shake = config["ExtraMenu"]["Shake"].get<bool>();
            Config.ExtraMenu.Spread = config["ExtraMenu"]["Spread"].get<bool>();
            Config.ExtraMenu.Hit = config["ExtraMenu"].value("Hit", false);
            Config.ExtraMenu.Reload = config["ExtraMenu"]["Reload"].get<bool>();
            Config.ExtraMenu.Scope = config["ExtraMenu"]["Scope"].get<bool>();
            Config.ExtraMenu.Switch = config["ExtraMenu"]["Switch"].get<bool>();
            Config.ExtraMenu.Kinetic = config["ExtraMenu"].value("Kinetic",config["ExtraMenu"].value("KineticArmor",false));
            if (config["ExtraMenu"].contains("Blueprints"))
                Config.ExtraMenu.Blueprints = config["ExtraMenu"]["Blueprints"].get<bool>();
            if (config["ExtraMenu"].contains("BlueprintTiers"))
                BlueprintTierFilter::SetMask(config["ExtraMenu"]["BlueprintTiers"].get<uint32_t>());
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

        if (config.contains("menu")) {
            menu[0] = config["menu"][0].get<float>();
            menu[1] = config["menu"][1].get<float>();
            menu[2] = config["menu"][2].get<float>();
        }

        bool fastPickup = false;
        float pickupRange = pickup::defaultRange;
        const auto extra = config.find("ExtraMenu");
        if (extra != config.end() && extra->is_object()) {
            const auto fast = extra->find("FastPickup");
            const auto range = extra->find("PickupRange");
            if (fast != extra->end() && fast->is_boolean()) fastPickup = fast->get<bool>();
            if (range != extra->end() && range->is_number()) pickupRange = pickup::NormalizeRange(range->get<float>());
        }
        Config.ExtraMenu.FastPickup = fastPickup;
        Config.ExtraMenu.PickupRange = pickupRange;
        pickup::Publish(fastPickup, pickupRange);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}
