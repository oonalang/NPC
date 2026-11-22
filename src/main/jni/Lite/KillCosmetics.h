#pragma once
#include "Menu.h"
#include "../KillCosmetics/State.h"

namespace lite_kill_cosmetics {
inline void Draw(kill_cosmetics::Kind kind) {
    using namespace kill_cosmetics;
    const auto index=Index(kind);if(index>=categoryCount)return;
    ImGui::PushID(static_cast<int>(index));
    ImGui::TextUnformatted(labels[index]);
    if(lite_widgets::Checkbox("Automatic cosmetics",&configured.automatic))Publish(configured);
    ImGui::TextWrapped("Follows your equipped mythic skin. Other weapon skins use their default cosmetics.");
    ImGui::TextWrapped("Uses matching game assets on this device.");
    if(!available.load(std::memory_order_acquire))
        ImGui::TextWrapped("%s",attempted.load(std::memory_order_acquire)?"Cosmetics unavailable for this native build":"Waiting for cosmetic setup");
    ImGui::Text("Catalog entries: %u",static_cast<unsigned>(catalog.Count(kind)));
    ImGui::Text("Known mythic blueprints: %u",static_cast<unsigned>(weapons.Count()));
    ImGui::TextWrapped("Switching weapons updates the cosmetic automatically. Missing or ambiguous matches keep the default.");
    ImGui::PopID();
}
}
