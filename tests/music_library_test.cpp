#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/System/UI/MusicPlayer.h"
#include <cassert>
#include <fstream>
#include <unistd.h>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = {.69f, .43f, .96f, 1.f};
namespace font {
ImFont* inter_semibold = nullptr;
ImFont* inter_title = nullptr;
}

static void File(const std::string& path) { std::ofstream(path).put('x'); }
static void TextEditing() {
    ImGui::CreateContext();auto& io=ImGui::GetIO();
    io.IniFilename=nullptr;io.DisplaySize={960,540};io.DeltaTime=1.f/60.f;
    io.Fonts->AddFontDefault();io.Fonts->Build();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({600,300});
    ImGui::Begin("input geometry",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
    const auto inputAt=ImGui::GetCursorScreenPos();char example[32]="folder";
    ImGui::SetNextItemWidth(420.f);ImGui::InputText("##full_width",example,sizeof(example));
    assert(std::fabs(GImGui->LastItemData.Rect.Min.x-inputAt.x)<.1f);
    assert(std::fabs(GImGui->LastItemData.Rect.GetWidth()-420.f)<.1f);
    ImGui::End();ImGui::Render();
    music_ui::TextDialog editor;editor.Open("/storage/emulated/0/Music");
    bool open=true;
    const auto frame=[&] {
        ImGui::NewFrame();custom::touch::Update();
        ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize,ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_PopupBg,{0,0,0,0});
        ImGui::Begin("host",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
        if(open) {ImGui::OpenPopup("Music source");open=false;}
        music_ui::Dialog("Music source",editor,"Choose your music folder","Use folder");
        ImGui::End();ImGui::PopStyleColor();ImGui::Render();
        assert(GImGui->StyleVarStack.Size==0 && GImGui->ColorStack.Size==0);
    };
    frame();frame();frame();
    auto* modal=ImGui::FindWindowByName("Music source");assert(modal);
    assert(modal->Flags&ImGuiWindowFlags_Modal);
    assert(ImLengthSqr(modal->Pos+modal->Size*.5f-ImGui::GetMainViewport()->GetCenter())<3.f);
    bool surface=false;
    for(const auto& vertex:modal->DrawList->VtxBuffer) if((vertex.col>>IM_COL32_A_SHIFT)>245) surface=true;
    assert(surface && modal->WindowPadding.x>=14.f);
    editor.Replace("/storage/emulated/0/My songs");frame();frame();
    assert(std::strcmp(editor.text,"/storage/emulated/0/My songs")==0);
    io.AddInputCharactersUTF8("/a");frame();
    assert(std::strcmp(editor.text,"/storage/emulated/0/My songs/a")==0);
    editor.insert=".mp3";editor.focus=true;frame();frame();
    assert(std::strcmp(editor.text,"/storage/emulated/0/My songs/a.mp3")==0);
    editor.erase=true;editor.focus=true;frame();frame();
    assert(std::strcmp(editor.text,"/storage/emulated/0/My songs/a.mp")==0);
    editor.Replace("");frame();frame();assert(editor.text[0]==0);
    editor.Replace("café");frame();frame();editor.erase=true;editor.focus=true;frame();frame();
    assert(std::strcmp(editor.text,"caf")==0);
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    const auto field=modal->Pos+modal->WindowPadding+ImVec2(12.f,ImGui::GetFontSize()+24.f);
    io.AddMousePosEvent(field.x,field.y);frame();
    io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();frame();frame();
    assert(editor.keyboard);
    const float keyGap=4.f*ImClamp(c::scale,.8f,1.15f);
    const float keyWidth=(modal->InnerRect.GetWidth()-modal->WindowPadding.x*2.f-keyGap*9.f)/10.f;
    const auto row=modal->Pos+modal->WindowPadding+ImVec2(0,ImGui::GetFontSize()+9.f+ImGui::GetFontSize()+18.f+9.f+30.f+9.f);
    const ImVec2 key=row+ImVec2(keyWidth*.5f,21.f);
    io.AddMousePosEvent(key.x,key.y);frame();io.AddMouseButtonEvent(0,true);frame();
    io.AddMouseButtonEvent(0,false);frame();frame();frame();
    assert(std::strlen(editor.text)>3);
    ImGui::DestroyContext();
}
int main() {
    TextEditing();
    char root[]="/tmp/npc-music-XXXXXX";
    assert(mkdtemp(root));
    const std::string a=std::string(root)+"/a",b=std::string(root)+"/b";
    assert(mkdir(a.c_str(),0700)==0 && mkdir(b.c_str(),0700)==0);
    File(a+"/Zulu.MP3");File(a+"/alpha.flac");File(a+"/.hidden.mp3");File(a+"/readme.txt");
    assert(mkdir((a+"/folder.wav").c_str(),0700)==0);
    File(b+"/Beta.ogg");
    assert(symlink((a+"/alpha.flac").c_str(),(b+"/duplicate.flac").c_str())==0);
    const auto library=music_player::ReadLibrary({a,b,a});
    assert(library.readable && !library.truncated && library.tracks.size()==3);
    assert(library.tracks[0].title=="alpha" && library.tracks[1].title=="Beta" && library.tracks[2].title=="Zulu");
    const auto repeat=music_player::ReadLibrary({a,b});
    assert(repeat.tracks.size()==library.tracks.size());
    for(std::size_t i=0;i<library.tracks.size();++i) assert(repeat.tracks[i].path==library.tracks[i].path);
    const auto limited=music_player::ReadLibrary({a,b},2);
    assert(limited.tracks.size()==2 && limited.truncated);
    const auto missing=music_player::ReadLibrary({std::string(root)+"/missing"});
    assert(!missing.readable && missing.tracks.empty());
    assert(music_player::AudioFile("recording.AAC") && !music_player::AudioFile("video.mp4"));
    assert(music_player::Utf16("Music \xf0\x9f\x8e\xb5")==u"Music \U0001f3b5");
    assert(music_player::Utf16("\xed\xa0\x80")==u"\ufffd");
    assert(music_player::Clock(65.9f)=="1:05" && music_player::Clock(-5)=="0:00");
    {
        music_player::Service service;
        service.Start();
        service.Queue({music_player::Action::Scan,-1,0.f,{a,b}});
        auto wait=[&](auto predicate) {
            for(int i=0;i<400;++i) {
                auto snapshot=service.Read();
                if(predicate(*snapshot)) return snapshot;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            assert(false);return service.Read();
        };
        auto first=wait([&](const auto& s){return s.scanned && !s.scanning && s.folders.size()==2 && s.folders[0]==a;});
        assert(first->tracks.size()==3);
        service.Queue({music_player::Action::Scan,-1,0.f,{b}});
        auto second=wait([&](const auto& s){return s.scanned && !s.scanning && s.folders.size()==1 && s.folders[0]==b;});
        assert(second->tracks.size()==2 && first->tracks.size()==3);
        service.Queue({music_player::Action::Mode,1});
        wait([](const auto& s){return s.driveMode;});
        service.Queue({music_player::Action::Connect,-1,0.f,{},"https://example.com/music"});
        auto drive=wait([](const auto& s){return s.driveMode&&!s.error.empty();});
        assert(!drive->available&&drive->tracks.empty());
        service.Queue({music_player::Action::Mode,0});
        wait([](const auto& s){return !s.driveMode;});
    }
    unlink((a+"/Zulu.MP3").c_str());unlink((a+"/alpha.flac").c_str());unlink((a+"/.hidden.mp3").c_str());unlink((a+"/readme.txt").c_str());
    rmdir((a+"/folder.wav").c_str());unlink((b+"/Beta.ogg").c_str());unlink((b+"/duplicate.flac").c_str());
    rmdir(a.c_str());rmdir(b.c_str());rmdir(root);
    std::puts("Music scan filtering, ordering, deduplication, limits, immutable snapshots, rescans and Drive-mode state passed");
}
