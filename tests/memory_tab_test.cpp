#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "memory_test_fixture.h"
#include "../src/main/jni/System/UI/MemoryTab.h"
#include <cassert>
#include <cstdio>
#include <cstring>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font {ImFont* inter_semibold=nullptr;ImFont* inter_title=nullptr;ImFont* icomoon_page=nullptr;}
static ImGuiWindow* host=nullptr;
static int clicks=0;
static ImGuiWindow* Find(const char* token) {
    for(auto* window:GImGui->Windows) if(window->LastFrameActive==GImGui->FrameCount&&std::strstr(window->Name,token)) return window;
    return nullptr;
}
static void Frame(ImVec2 size) {
    auto& io=ImGui::GetIO();io.DisplaySize=size;
    ImGui::NewFrame();custom::UpdateTouchScroll();
    ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);ImGui::SetNextWindowSize(size,ImGuiCond_Always);
    ImGui::Begin("Memory fixture",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::BeginChild("##memory_host",{0,0},false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
    host=ImGui::GetCurrentWindow();memory_tab::Draw(ImGui::GetContentRegionAvail());
    ImGui::EndChild();ImGui::End();ImGui::Render();
    assert(GImGui->StyleVarStack.Size==0&&GImGui->ColorStack.Size==0);
    for(auto* window:GImGui->Windows) if(window->LastFrameActive==GImGui->FrameCount&&std::strstr(window->Name,"memory_")&&window!=host) {
        assert((window->Flags&ImGuiWindowFlags_NoScrollbar)&&!window->ScrollbarY);
        assert(window->Pos.x>=host->Pos.x-1.f&&window->Pos.x+window->Size.x<=host->Pos.x+host->Size.x+1.f);
        assert(window->Pos.y+window->Size.y<=host->Pos.y+host->Size.y+1.f);
    }
    const auto* data=ImGui::GetDrawData();
    for(int i=0;i<data->CmdListsCount;++i) for(const auto& vertex:data->CmdLists[i]->VtxBuffer) assert(std::isfinite(vertex.pos.x)&&std::isfinite(vertex.pos.y));
}
static void Pump(ImVec2 size,int frames=3) {for(int i=0;i<frames;++i) Frame(size);}
static void Touch(ImVec2 point,ImVec2 size) {
    auto& io=ImGui::GetIO();io.AddMousePosEvent(point.x,point.y);Frame(size);
    io.AddMouseButtonEvent(0,true);Frame(size);io.AddMouseButtonEvent(0,false);Frame(size);Frame(size);
}
int main() {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DeltaTime=1.f/60.f;
    ImFontConfig config;config.SizePixels=20.f;io.Fonts->AddFontDefault(&config);io.Fonts->Build();
    c::ApplyTheme();slide_fatigue::available=true;
    ui_sounds::observer=[](ui_sounds::Cue){++clicks;};
    for(float scale:{.8f,1.f,1.15f}) {
        c::scale=scale;
        for(auto size:{ImVec2(900,360),ImVec2(700,280),ImVec2(400,600),ImVec2(280,360)}) {
            for(int category=0;category<4;++category) {
                memory_tab::selected=static_cast<memory_tab::Category>(category);Pump(size);
                if(category==0) assert(Find("memory_movement"));
                if(category==1) assert(Find("memory_weapons"));
                if(category==2) assert(Find("memory_protection"));
                if(category==3) assert(Find("memory_blueprints"));
            }
        }
    }
    c::scale=1.f;const ImVec2 size(400,360);memory_tab::selected=memory_tab::Category::Movement;Pump(size);
    auto* movement=Find("memory_movement");assert(movement&&movement->ScrollMax.y>100.f);
    const auto start=movement->InnerRect.GetCenter();const int before=clicks;
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);io.AddMousePosEvent(start.x,start.y+30);Frame(size);
    io.AddMouseButtonEvent(0,true);Frame(size);io.AddMousePosEvent(start.x+2,start.y-90);Frame(size);
    io.AddMouseButtonEvent(0,false);Frame(size);Frame(size);
    assert(movement->Scroll.y>60.f&&clicks==before&&!Config.ExtraMenu.NoCrouch&&!Config.ExtraMenu.NoSlideFatigue);
    const float width=host->ContentRegionRect.GetWidth(),gap=8.f;
    const float buttonWidth=(width-gap)*.5f,buttonHeight=ImMax(34.f,ImGui::GetFontSize()+12.f);
    const auto category=host->DC.CursorStartPos+ImVec2(buttonWidth+gap+buttonWidth*.5f,buttonHeight*.5f);
    Touch(category,size);assert(memory_tab::selected==memory_tab::Category::Weapons);
    memory_tab::selected=memory_tab::Category::Movement;movement->Scroll.y=0;movement->ScrollTarget.y=FLT_MAX;Pump(size);
    const auto first=movement->DC.CursorStartPos+ImVec2(30,12);Touch(first,size);assert(Config.ExtraMenu.NoCrouch);
    ImGui::DestroyContext();
    std::puts("Four Memory categories fit landscape/narrow layouts; movement scrolls silently, category taps switch and checkbox taps still work");
}
