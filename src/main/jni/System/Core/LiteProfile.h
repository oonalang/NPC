#pragma once

namespace shai_lite {

inline void Apply() {
    const bool line = Config.ESPMenu.isPlayerLine;
    const bool box = Config.ESPMenu.Box;
    const bool name = Config.ESPMenu.Name;
    const bool health = Config.ESPMenu.Health;
    const bool armor = Config.ESPMenu.Armor;
    const bool distance = Config.ESPMenu.Distance;
    const bool count = Config.ESPMenu.Count;
    const auto boxType = Config.ESPMenu.BoxType;
    const auto lineTarget = Config.ESPMenu.Target;
    const auto healthPosition = Config.ESPMenu.HealthPosition;

    Config.ESPMenu = decltype(Config.ESPMenu){};
    Config.ESPMenu.isPlayerLine = line;
    Config.ESPMenu.Box = box;
    Config.ESPMenu.Name = name;
    Config.ESPMenu.Health = health;
    Config.ESPMenu.Armor = armor;
    Config.ESPMenu.Distance = distance;
    Config.ESPMenu.Count = count;
    Config.ESPMenu.BoxType = boxType == EspBoxType::ThreeD ? EspBoxType::Outline : boxType;
    Config.ESPMenu.Target = lineTarget;
    Config.ESPMenu.HealthPosition = healthPosition;

    Config.Aim = decltype(Config.Aim){};
    Config.ExtraMenu = decltype(Config.ExtraMenu){};
}

}
