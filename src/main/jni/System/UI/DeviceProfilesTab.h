#pragma once
#include "../Core/DeviceProfiles.h"
#include "../../ImGui/Call_ImGui.h"
#include <ctime>
#include <cstring>
#include <string>
#include <vector>

namespace device_profiles_tab {

inline void Draw() {
    static bool initialized=false;
    static std::vector<device_profiles::Entry> profiles;
    static char name[64]="My_Config";
    static std::string status;
    static std::string deleteTarget;
    if (!initialized) {
        profiles=device_profiles::List();
        initialized=true;
    }
    const auto refresh=[&]() { profiles=device_profiles::List(); };
    ImGui::TextUnformatted("CONFIG MANAGER");
    ImGui::TextDisabled("Profiles are stored on this device.");
    ImGui::Separator();
    ImGui::SetNextItemWidth(ImMin(360.0f,ImGui::GetContentRegionAvail().x));
    ImGui::InputText("Profile name",name,sizeof(name));
    if (ImGui::Button("Save current",ImVec2(150.0f,40.0f))) {
        if (device_profiles::SaveLocal(name)) { status="Configuration saved on this device.";refresh(); }
        else status="Unable to save this profile.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Quick save",ImVec2(130.0f,40.0f))) {
        const std::string autoName="Profile_"+std::to_string(static_cast<long long>(std::time(nullptr)));
        if (device_profiles::SaveLocal(autoName)) { status="Saved as "+autoName;refresh(); }
        else status="Unable to save this profile.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Import copied JSON",ImVec2(200.0f,40.0f))) {
        std::string imported;
        if (device_profiles::ImportClipboard(imported)) {
            status="Imported "+imported+". Select Load to apply it.";
            refresh();
        } else status="Import failed. Open the shared JSON file and copy its complete text first.";
    }
    if (!status.empty()) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s",status.c_str());
    }
    ImGui::Spacing();
    ImGui::TextDisabled("Export saves a portable .json file to Downloads/SHAI-ETHNIR.");
    ImGui::TextDisabled("To import another user's file, copy its JSON text and tap Import copied JSON.");
    ImGui::Separator();
    ImGui::Text("Saved profiles (%d)",static_cast<int>(profiles.size()));
    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh")) refresh();
    ImGui::BeginChild("##device_profile_list",ImVec2(0,0),false,ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (const auto& profile:profiles) {
        ImGui::PushID(profile.name.c_str());
        ImGui::TextUnformatted(profile.name.c_str());
        if (ImGui::Button("Load",ImVec2(100.0f,35.0f))) {
            status=device_profiles::LoadLocal(profile.name) ? "Loaded "+profile.name : "Unable to load this profile.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Export .json",ImVec2(145.0f,35.0f))) {
            status=device_profiles::ExportLocal(profile.name)
                ? "Exported to Downloads/SHAI-ETHNIR."
                : "Unable to export. Check storage access.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete",ImVec2(95.0f,35.0f))) {
            deleteTarget=profile.name;
            ImGui::OpenPopup("Delete profile?");
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    ImGui::EndChild();
    if (ImGui::BeginPopupModal("Delete profile?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete %s?",deleteTarget.c_str());
        if (ImGui::Button("Confirm",ImVec2(120,36))) {
            status=device_profiles::DeleteLocal(deleteTarget) ? "Profile deleted." : "Unable to delete profile.";
            refresh();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel",ImVec2(120,36))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

}
