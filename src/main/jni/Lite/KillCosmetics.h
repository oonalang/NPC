#pragma once
#include "Menu.h"
#include "../KillCosmetics/State.h"

namespace lite_kill_cosmetics {
inline const char* Rarity(int value) {
    constexpr const char* names[]{"Common","Common","Rare","Epic","Legendary","Mythic"};
    return value>=0 && value<6?names[value]:"Unknown";
}
inline bool Matches(const char* text,const char* query) {
    auto lower=[](unsigned char c){return c>='A' && c<='Z'?c+32:c;};
    return std::search(text,text+std::strlen(text),query,query+std::strlen(query),
        [&](unsigned char a,unsigned char b){return lower(a)==lower(b);})!=text+std::strlen(text) || !*query;
}
struct Cache {
    char search[64]{},previous[64]{};
    int rarity=0,previousRarity=-1;
    std::uint64_t revision=UINT64_MAX;
    std::vector<kill_cosmetics::Row> rows;
    std::vector<int> visible;
};
inline std::array<Cache,kill_cosmetics::categoryCount> caches;
inline void Draw(kill_cosmetics::Kind kind) {
    using namespace kill_cosmetics;
    const auto index=Index(kind);if(index>=categoryCount) return;
    auto& cache=caches[index];auto& selected=configured.ids[index];
    ImGui::PushID(static_cast<int>(index));
    ImGui::TextUnformatted(labels[index]);
    ImGui::TextWrapped("Uses downloaded game cosmetics. Visible on this device.");
    if(!available.load(std::memory_order_acquire))
        ImGui::TextWrapped("%s",attempted.load(std::memory_order_acquire)?"Cosmetics unavailable for this native build":"Waiting for game cosmetic setup");
    lite_ui::Input("Search cosmetics",cache.search,sizeof(cache.search));
    static const char* rarities[]{"All","Common","Rare","Epic","Legendary","Mythic"};
    lite_ui::Combo("Rarity",cache.rarity,rarities,6);
    const auto revision=catalog.Revision();
    if(cache.revision!=revision || cache.previousRarity!=cache.rarity || std::strcmp(cache.search,cache.previous)) {
        if(cache.revision!=revision) cache.rows=catalog.Rows(kind);
        cache.visible.clear();
        for(std::size_t i=0;i<cache.rows.size();++i) {
            const auto& row=cache.rows[i];char text[96];
            std::snprintf(text,sizeof(text),"%s %s %u",Rarity(row.rarity),labels[index],row.id);
            if((!cache.rarity || (cache.rarity==1?row.rarity<=1:row.rarity==cache.rarity)) && Matches(text,cache.search))
                cache.visible.push_back(static_cast<int>(i));
        }
        cache.revision=revision;cache.previousRarity=cache.rarity;
        std::snprintf(cache.previous,sizeof(cache.previous),"%s",cache.search);
    }
    bool stock=selected==0;
    if(lite_widgets::Checkbox("Default",&stock)) {selected=0;Publish(configured);}
    if(selected) ImGui::Text("Selected: %u",selected);
    else ImGui::TextUnformatted("Selected: Default");
    if(cache.rows.empty()) ImGui::TextWrapped("Waiting for the game's cosmetic catalog. If injected after loading, restart with the module loaded earlier.");
    else ImGui::Text("%u of %u entries",static_cast<unsigned>(cache.visible.size()),static_cast<unsigned>(cache.rows.size()));
    const float height=ImClamp(ImGui::GetContentRegionAvail().y,90.f*lite_widgets::scale,320.f*lite_widgets::scale);
    ImGui::BeginChild("##cosmetics",{0,height},false,ImGuiWindowFlags_NoScrollbar);
    lite_widgets::SetFontScale();
    ImGui::BeginDisabled(!available.load(std::memory_order_acquire));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,4.f*lite_widgets::scale));
    ImGuiListClipper clipper;clipper.Begin(static_cast<int>(cache.visible.size()));
    while(clipper.Step()) for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;++i) {
        const auto& row=cache.rows[cache.visible[i]];bool active=selected==row.id;char text[80];
        std::snprintf(text,sizeof(text),"%s / %u",Rarity(row.rarity),row.id);
        ImGui::PushID(static_cast<int>(row.id));
        if(lite_widgets::Checkbox(text,&active)) {selected=active?row.id:0;Publish(configured);}
        ImGui::PopID();
    }
    ImGui::PopStyleVar();ImGui::EndDisabled();ImGui::EndChild();ImGui::PopID();
}
}
