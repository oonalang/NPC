#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/System/UI/HomeTab.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <thread>
#include <unistd.h>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr; ImFont* inter_title=nullptr; }

namespace {
struct Layout {
    bool present=false,scrollable=false,musicScrollable=false;
    float musicContent=0.f,musicHeight=0.f,musicBottom=0.f,viewport=0.f,overflow=0.f;
    float scrollMax=0.f,musicScrollMax=0.f;
};

ImGuiWindow* Window(const char* token) {
    if(!GImGui) return nullptr;
    for(ImGuiWindow* window : GImGui->Windows) if(std::strstr(window->Name,token)) return window;
    return nullptr;
}

Layout Frame(ImVec2 size) {
    auto& io=ImGui::GetIO();
    io.DisplaySize=size;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);
    ImGui::SetNextWindowSize(size,ImGuiCond_Always);
    ImGui::Begin("##home_host",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove);
    home_tab::HomeStats stats;
    stats.fps=60.f;stats.frameMs=16.6f;stats.uptime=3725.;
    stats.width=static_cast<int>(size.x);stats.height=static_cast<int>(size.y);
    stats.activeFeatures=9;stats.keySaved=true;
    home_tab::Draw(stats,ImGui::GetContentRegionAvail());
    Layout out;
    if(auto* dashboard=Window("##home_dashboard")) {
        out.present=true;
        out.scrollable=(dashboard->Flags & ImGuiWindowFlags_NoScrollWithMouse)==0;
        assert((dashboard->Flags & ImGuiWindowFlags_NoScrollbar)!=0);
        out.scrollMax=dashboard->ScrollMax.y;
        out.viewport=dashboard->InnerRect.GetHeight();
        const float content=(dashboard->DC.CursorMaxPos.y-dashboard->Pos.y)+dashboard->WindowPadding.y;
        out.overflow=std::max(0.f,content-out.viewport);
        if(auto* music=Window("##home_music")) {
            out.musicContent=(music->DC.CursorMaxPos.y-music->Pos.y)+music->WindowPadding.y;
            out.musicHeight=music->Size.y;
            out.musicScrollable=(music->Flags & ImGuiWindowFlags_NoScrollbar)==0;
            out.musicScrollMax=music->ScrollMax.y;
            out.musicBottom=(music->Pos.y-dashboard->Pos.y)+music->Size.y;
        }
    }
    ImGui::End();
    ImGui::Render();
    return out;
}

Layout Pump(ImVec2 size,int frames=2) {
    Layout out;
    for(int i=0;i<frames;++i) out=Frame(size);
    return out;
}

bool WaitFor(ImVec2 size,bool (*predicate)(const music_player::Snapshot&)) {
    for(int i=0;i<400;++i) {
        if(predicate(*music_player::GetSnapshot())) return true;
        Frame(size);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return predicate(*music_player::GetSnapshot());
}

void Check(ImVec2 size,const char* mode,const char* scaleLabel) {
    const auto out=Pump(size);
    assert(out.present);
    if(!(out.musicHeight>0.f)) {
        std::fprintf(stderr,"%s %s %.0fx%.0f: no music card\n",scaleLabel,mode,size.x,size.y);
        assert(false);
    }
    if(!(out.musicContent<=out.musicHeight+.5f)) {
        if(!out.musicScrollable) {
            std::fprintf(stderr,"%s %s %.0fx%.0f: music content %.1f clipped by card %.1f and cannot scroll\n",
                scaleLabel,mode,size.x,size.y,out.musicContent,out.musicHeight);
            assert(false);
        }
    }
    if(!(out.musicBottom<=out.viewport+.5f) && !out.scrollable) {
        std::fprintf(stderr,"%s %s %.0fx%.0f: music card ends at %.1f past a %.1f viewport without scrolling\n",
            scaleLabel,mode,size.x,size.y,out.musicBottom,out.viewport);
        assert(false);
    }
    if(out.overflow>1.f) {
        if(!out.scrollable) {
            std::fprintf(stderr,"%s %s %.0fx%.0f: %.1f of overflow is unreachable (scrollable %d, scrollMax %.1f, viewport %.1f)\n",
                scaleLabel,mode,size.x,size.y,out.overflow,(int)out.scrollable,out.scrollMax,out.viewport);
            assert(false);
        }
    }
    std::printf("%s %s %.0fx%.0f: card %.0f, content %.0f, viewport %.0f, scroll %.0f\n",
        scaleLabel,mode,size.x,size.y,out.musicHeight,out.musicContent,out.viewport,out.scrollMax);
}
}

int main() {
    ImGui::CreateContext();
    auto& io=ImGui::GetIO();
    io.IniFilename=nullptr;
    io.DisplaySize=ImVec2(900,520);
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    char root[]="/tmp/npc-home-layout-XXXXXX";
    assert(mkdtemp(root));
    const std::string folder=root;
    std::ofstream(folder+"/Alpha.mp3").put('x');
    std::ofstream(folder+"/Beta.flac").put('x');
    const std::string missing=folder+"/missing";

    music_player::ScanFolders({folder});
    assert(WaitFor(io.DisplaySize,[](const music_player::Snapshot& state) {
        return state.scanned && !state.scanning && state.tracks.size()==2 && state.error.empty();
    }));

    const ImVec2 sizes[]={ImVec2(900,520),ImVec2(700,420),ImVec2(620,300),ImVec2(1000,300),
        ImVec2(420,760),ImVec2(330,600),ImVec2(280,520)};
    const float scales[]={1.15f,.8f};

    for(const float scale : scales) {
        c::scale=scale;
        const std::string label=scale>1.f?"scale 1.15":"scale 0.80";
        for(const auto size : sizes) Check(size,"local",label.c_str());
        music_player::ScanFolders({missing});
        assert(WaitFor(io.DisplaySize,[](const music_player::Snapshot& state) {
            return !state.scanning && state.tracks.empty() && !state.error.empty();
        }));
        for(const auto size : sizes) Check(size,"local error",label.c_str());

        music_player::SetDriveMode(true);
        assert(WaitFor(io.DisplaySize,[](const music_player::Snapshot& state) { return state.driveMode; }));
        for(const auto size : sizes) Check(size,"Drive empty",label.c_str());
        music_player::ConnectDrive("https://invalid.example/music");
        assert(WaitFor(io.DisplaySize,[](const music_player::Snapshot& state) {return !state.scanning&&!state.error.empty();}));
        for(const auto size:sizes) Check(size,"Drive error",label.c_str());
        music_player::SetDriveMode(false);
        assert(WaitFor(io.DisplaySize,[](const music_player::Snapshot& state) { return !state.driveMode; }));
    }

    music_player::ScanFolders({folder});
    WaitFor(io.DisplaySize,[](const music_player::Snapshot& state) { return state.error.empty(); });
    ImGui::DestroyContext();
    unlink((folder+"/Alpha.mp3").c_str());
    unlink((folder+"/Beta.flac").c_str());
    rmdir(folder.c_str());
    std::puts("Home dashboard keeps the music card and its Local and Drive controls fully visible at wide, landscape, portrait and narrow sizes, and overflowing panels stay scrollable");
}
