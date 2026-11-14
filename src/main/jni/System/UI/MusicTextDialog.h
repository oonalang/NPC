#pragma once
#include "../Audio/UiSounds.h"
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/TouchScroll.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace music_ui {
struct TextDialog {
    char text[512]{};
    std::string insert;
    bool replace=false,erase=false,focus=false,keyboard=false,shift=false;
    void Open(const std::string& value) {
        std::snprintf(text,sizeof(text),"%s",value.c_str());
        insert.clear();replace=erase=keyboard=shift=false;focus=true;
    }
    void Replace(std::string value) { insert=std::move(value);replace=true;focus=true; }
    static int Edit(ImGuiInputTextCallbackData* data) {
        auto& state=*static_cast<TextDialog*>(data->UserData);
        if(state.replace) {data->DeleteChars(0,data->BufTextLen);state.replace=false;}
        if(state.erase) {
            const int end=data->SelectionStart!=data->SelectionEnd?std::max(data->SelectionStart,data->SelectionEnd):data->CursorPos;
            int start=data->SelectionStart!=data->SelectionEnd?std::min(data->SelectionStart,data->SelectionEnd):end;
            if(start==end && start>0) {
                --start;
                while(start>0 && (static_cast<unsigned char>(data->Buf[start])&0xc0)==0x80) --start;
            }
            data->DeleteChars(start,end-start);data->CursorPos=data->SelectionStart=data->SelectionEnd=start;
            state.erase=false;
        }
        if(!state.insert.empty()) {
            const int start=std::min(data->SelectionStart,data->SelectionEnd);
            if(data->SelectionStart!=data->SelectionEnd) data->DeleteChars(start,std::abs(data->SelectionEnd-data->SelectionStart));
            data->InsertChars(data->CursorPos,state.insert.c_str());state.insert.clear();
        }
        return 0;
    }
};
inline bool Button(const char* label,ImVec2 size,bool primary=false) {
    ImGui::PushStyleColor(ImGuiCol_Button,primary?ImVec4(c::accent.x,c::accent.y,c::accent.z,.5f):ImVec4(.22f,.20f,.29f,1.f));
    const bool pressed=ImGui::Button(label,size);
    const bool accepted=pressed && (!custom::touch::Applies() || custom::touch::Tap());
    if(accepted) ui_sounds::Play(ui_sounds::Cue::Click);
    ImGui::PopStyleColor();return accepted;
}
enum class DialogResult { None,Apply,Shortcut };
inline DialogResult Dialog(const char* name,TextDialog& state,const char* title,const char* apply,
                   const char* preset=nullptr,const char* presetValue=nullptr,const char* shortcut=nullptr) {
    const auto* viewport=ImGui::GetMainViewport();
    const float scale=ImClamp(c::scale,.8f,1.15f);
    const float width=ImMin(state.keyboard?760.f*scale:560.f*scale,viewport->Size.x-24.f);
    ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Always,{.5f,.5f});
    ImGui::SetNextWindowSizeConstraints({width,0},{width,viewport->Size.y-24.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(18,16)*scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,20.f*scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,10.f*scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(10,9)*scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8,9)*scale);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,ImVec4(.075f,.065f,.11f,.99f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.075f,.065f,.11f,.99f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg,ImVec4(.015f,.01f,.025f,.65f));
    ImGui::PushStyleColor(ImGuiCol_Text,c::text::text_active);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(.15f,.13f,.20f,1.f));
    auto result=DialogResult::None;
    if(ImGui::BeginPopupModal(name,nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar|
        ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextUnformatted(title);
        if(preset && presetValue && !state.keyboard) {
            const float presetWidth=shortcut?(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*.5f:ImGui::GetContentRegionAvail().x;
            if(Button(preset,{presetWidth,30.f*scale})) state.Replace(presetValue);
            if(shortcut) {
                ImGui::SameLine();
                if(Button(shortcut,{presetWidth,30.f*scale})) {result=DialogResult::Shortcut;state.keyboard=false;ImGui::CloseCurrentPopup();}
            }
        }
        if(state.focus) {ImGui::SetKeyboardFocusHere();state.focus=false;}
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputText("##entry",state.text,sizeof(state.text),ImGuiInputTextFlags_CallbackAlways,TextDialog::Edit,&state);
        if(ImGui::GetIO().MouseSource==ImGuiMouseSource_TouchScreen && ImGui::IsItemActive()) state.keyboard=true;
        const float half=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*.5f;
        if(Button("Paste",{half,30.f*scale})) {
            if(const char* value=ImGui::GetClipboardText()) state.Replace(value);
        }
        ImGui::SameLine();
        if(Button("Clear",{half,30.f*scale})) state.Replace("");
        if(state.keyboard) {
            const float available=ImGui::GetContentRegionAvail().x;
            const float keyGap=4.f*scale,keyWidth=(available-keyGap*9.f)/10.f;
            const float keyHeight=ImClamp((viewport->Size.y-235.f*scale)/5.f,19.f*scale,42.f*scale);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{keyGap,keyGap});
            const char* rows[]{"1234567890",state.shift?"QWERTYUIOP":"qwertyuiop",state.shift?"ASDFGHJKL_":"asdfghjkl_",state.shift?"ZXCVBNM-./":"zxcvbnm-./"};
            ImGui::PushID("keys");
            for(int row=0;row<4;++row) for(int i=0;i<10;++i) {
                char label[]{rows[row][i],0};
                ImGui::PushID(row*10+i);
                if(Button(label,{keyWidth,keyHeight})) {state.insert+=label;state.focus=true;}
                ImGui::PopID();if(i<9) ImGui::SameLine();
            }
            const float utility=(available-keyGap*3.f)*.25f;
            if(Button(state.shift?"abc":"ABC",{utility,keyHeight})) state.shift=!state.shift;
            ImGui::SameLine();
            if(Button("Space",{utility,keyHeight})) {state.insert+=" ";state.focus=true;}
            ImGui::SameLine();
            if(Button("Delete",{utility,keyHeight})) {state.erase=true;state.focus=true;}
            ImGui::SameLine();
            if(Button("Done",{utility,keyHeight})) state.keyboard=false;
            ImGui::PopID();ImGui::PopStyleVar();
        }
        if(Button(apply,{half,34.f*scale},true) && state.text[0]) {result=DialogResult::Apply;state.keyboard=false;ImGui::CloseCurrentPopup();}
        ImGui::SameLine();
        if(Button("Cancel",{half,34.f*scale})) {state.keyboard=false;ImGui::CloseCurrentPopup();}
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(5);ImGui::PopStyleVar(5);
    return result;
}
}
