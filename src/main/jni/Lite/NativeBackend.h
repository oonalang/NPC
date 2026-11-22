#pragma once
#include "Menu.h"
#include "KillCosmetics.h"
#include "../System/UI/MusicPlayer.h"
#include <cstring>

namespace lite_backend {
inline void Skins(int category) {
    if(category>=7 && category<10) {
        lite_kill_cosmetics::Draw(static_cast<kill_cosmetics::Kind>(category-7));return;
    }
    skinSubTab = category;
    RenderSkinCategoryContent(category, false);
}

inline void Music() {
    static bool initialized = false;
    static char folder[512]{}, cloud[512]{}, search[128]{};
    static int source = 0;
    if (!initialized) {
        if (!VM) { ImGui::TextDisabled("Waiting for Android audio"); return; }
        music_player::Init(VM);
        std::snprintf(folder,sizeof(folder),"%s",music_player::AppFolder);
        std::snprintf(cloud,sizeof(cloud),"%s",music_player::DefaultDrive);
        initialized = true;
    }
    const auto snapshot = music_player::GetSnapshot();
    const auto& player = *snapshot;
    static const char* sources[]{"Local","Cloud"};
    ImGui::TextUnformatted("Source");
    if (custom::Combo("##source",&source,sources,2)) music_player::SetDriveMode(source==1);
    lite_ui::Input(source ? "Cloud folder" : "Music folder", source ? cloud : folder, sizeof(folder));
    const float half = (ImGui::GetContentRegionAvail().x-4.f)*.5f;
    if (lite_widgets::Button("Paste",{half,28.f*lite_widgets::scale})) {
        const auto clipboard = getClipboard();
        std::snprintf(source ? cloud : folder,sizeof(folder),"%s",clipboard.c_str());
    }
    ImGui::SameLine(0,4);
    ImGui::BeginDisabled(player.scanning||player.loading);
    if (lite_widgets::Button(source ? "Connect" : "Scan folder",{half,28.f*lite_widgets::scale})) {
        if (source) music_player::ConnectDrive(cloud);
        else music_player::ScanFolders({folder});
    }
    ImGui::EndDisabled();
    if(player.loading||player.scanning) ImGui::TextDisabled("Loading music");
    const float third=(ImGui::GetContentRegionAvail().x-8.f)/3.f;
    if(lite_widgets::Button("Previous",{third,28.f*lite_widgets::scale})) music_player::Previous();
    ImGui::SameLine(0,4);
    if(lite_widgets::Button(player.playing?"Pause":"Play",{third,28.f*lite_widgets::scale})) music_player::Toggle();
    ImGui::SameLine(0,4);
    if(lite_widgets::Button("Next",{third,28.f*lite_widgets::scale})) music_player::Next();
    if(player.current>=0&&player.current<static_cast<int>(player.tracks.size()))
        ImGui::TextWrapped("%s",player.tracks[player.current].title.c_str());
    if(player.length>0) {
        float position=player.position;
        if(lite_widgets::SliderFloat("Position",&position,0,player.length,"%.0f s")) music_player::Seek(position);
    }
    lite_ui::Input("Search songs",search,sizeof(search));
    if(!player.error.empty()) ImGui::TextWrapped("%s",player.error.c_str());
    static std::vector<int> visible;
    visible.clear();
    const char* searchBegin = search;
    const auto* searchEnd = searchBegin + std::strlen(search);
    for(std::size_t index=0;index<player.tracks.size();++index) {
        const auto& title = player.tracks[index].title;
        const auto lower = [](unsigned char value) { return value >= 'A' && value <= 'Z' ? value + 32 : value; };
        if (!*search || std::search(title.begin(),title.end(),searchBegin,searchEnd,
            [&](unsigned char a,unsigned char b) { return lower(a) == lower(b); }) != title.end())
            visible.push_back(static_cast<int>(index));
    }
    ImGui::BeginChild("##songs",{0,120.f*lite_widgets::scale},false,ImGuiWindowFlags_NoScrollbar);
    lite_widgets::SetFontScale();
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(visible.size()));
    while(clipper.Step()) for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;++row) {
        const int index=visible[row];
        ImGui::PushID(index);
        if(custom::Selectable(player.tracks[index].title.c_str(),player.current==index)) music_player::Play(index);
        ImGui::PopID();
    }
    ImGui::EndChild();
}
}
