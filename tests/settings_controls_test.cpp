#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
struct SettingsConfig {
    struct {bool ClearDisplay=false,EnhanceFPS=false,ResetGuest=false;} ExtraMenu;
    struct {bool FpsLevel=false,FpsLevelUltra=false;float FpsLevel1=60.f,FpsLevelUltra1=240.f;} Aim;
} Config;
#include "../src/main/jni/System/UI/SettingsControls.h"
#include "../src/main/jni/ImGui/TouchScroll.h"
#include "../src/main/jni/ImGui/imgui_settings.h"
#include <cassert>
#include <cstdio>
ImFont* F50=nullptr;ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font {ImFont* inter_semibold=nullptr;ImFont* inter_title=nullptr;ImFont* icomoon_page=nullptr;}
static ImGuiWindow* child=nullptr;static ImVec2 origin;static float row=0;static ImRect reset;
static void Frame() {
    ImGui::NewFrame();custom::UpdateTouchScroll();
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({600,600});
    ImGui::Begin("Settings fixture",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove);
    ImGui::BeginChild("Enhancement",{540,500},false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
    child=ImGui::GetCurrentWindow();
    bool value=false;const auto before=ImGui::GetCursorScreenPos();custom::Checkbox("Measure",&value);
    row=ImGui::GetCursorScreenPos().y-before.y;origin=ImGui::GetCursorScreenPos();
    settings_controls::Draw();reset=GImGui->LastItemData.Rect;
    ImGui::EndChild();ImGui::End();ImGui::Render();
    assert(GImGui->StyleVarStack.Size==0&&GImGui->ColorStack.Size==0&&GImGui->DisabledStackSize==0);
}
static void Tap(ImVec2 point) {
    auto& io=ImGui::GetIO();io.AddMousePosEvent(point.x,point.y);Frame();
    io.AddMouseButtonEvent(0,true);Frame();io.AddMouseButtonEvent(0,false);Frame();Frame();
}
int main() {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={640,640};io.DeltaTime=1.f/60;
    io.Fonts->AddFontDefault();io.Fonts->Build();c::ApplyTheme();c::scale=1.f;
    frame_rate::available=false;Frame();Frame();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    for(int index=1;index<=3;++index) {
        Config={};Frame();Frame();
        Tap(origin+ImVec2(60,row*index+12));
        assert(index==1?Config.ExtraMenu.EnhanceFPS:index==2?Config.Aim.FpsLevel:Config.Aim.FpsLevelUltra);
    }
    Config={};Frame();Frame();Tap(reset.GetCenter());assert(Config.ExtraMenu.ResetGuest);
    ImGui::DestroyContext();std::puts("FPS preferences stay touch-editable while backends are unavailable; Reset Guest remains reachable and disabled state never leaks");
}
