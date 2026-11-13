#define IMGUI_DEFINE_MATH_OPERATORS
#include "../src/main/jni/ImGui/custom_widgets.hpp"
#include "../src/main/jni/ImGui/imgui_settings.h"
#include "../src/main/jni/ImGui/TouchScroll.h"
#include "../src/main/jni/ImGui/SliderGeometry.h"
#include <cassert>
#include <cmath>
#include <cstdio>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr;ImFont* inter_title=nullptr;ImFont* icomoon_page=nullptr; }

bool enabled=false,opened=false;
float slider=25.f;
int presses=0,selection=0;
ImRect toggleRect,sliderRect,comboRect,buttonRect,panelRect;
ImGuiWindow* panel=nullptr;
ImRect rows[18];
void Frame() {
    ImGui::NewFrame();custom::UpdateTouchScroll();
    ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({480,540});
    ImGui::Begin("Touch fixture",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetCursorScreenPos({40,40});
    custom::Child("Content",{400,310},true);
    panel=ImGui::GetCurrentWindow();
    custom::Checkbox("Toggle",&enabled);toggleRect=GImGui->LastItemData.Rect;
    custom::SliderFloat("Slider",&slider,0.f,100.f,"%.0f");sliderRect=GImGui->LastItemData.Rect;
    opened=custom::BeginCombo("Choice","Current",18);
    if(opened) {
        for(int i=0;i<18;++i) {
            char label[32];std::snprintf(label,sizeof(label),"Option %02d",i);
            if(custom::Selectable(label,selection==i)) selection=i;
            rows[i]=GImGui->LastItemData.Rect;
        }
        custom::EndCombo();
    }
    else comboRect=GImGui->LastItemData.NavRect;
    if(custom::Button("Action",{0,35})) ++presses;
    buttonRect=GImGui->LastItemData.Rect;
    ImGui::Dummy({0,700});
    custom::EndChild();panelRect=GImGui->LastItemData.Rect;
    ImGui::End();ImGui::Render();
}
void Move(ImVec2 point) { ImGui::GetIO().AddMousePosEvent(point.x,point.y);Frame(); }
void Down(ImVec2 point) { Move(point);ImGui::GetIO().AddMouseButtonEvent(0,true);Frame(); }
void Up() { ImGui::GetIO().AddMouseButtonEvent(0,false);Frame();Frame(); }
void ResetScroll() { panel->Scroll.y=0;panel->ScrollTarget.y=FLT_MAX;Frame();Frame(); }
void Pan(ImVec2 start) {
    const auto original=ImGui::FindWindowByName("Touch fixture")->Pos;
    Down(start);Move(start+ImVec2(2,-40));Move(start+ImVec2(3,-75));Up();
    assert(ImLengthSqr(ImGui::FindWindowByName("Touch fixture")->Pos-original)<.01f);
}
int main() {
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={600,600};io.DeltaTime=1.f/60;
    unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    c::ApplyTheme();c::scale=1.f;c::widget_scale=1.f;
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);Frame();Frame();
    assert(panel->ScrollMax.y>0 && !panel->ScrollbarY);
    assert(std::fabs(panelRect.GetHeight()-310.f)<.01f && panelRect.Min.y==40.f);
    Pan({410,325});assert(panel->Scroll.y>50.f);ResetScroll();
    for(float half : {7.f,17.f}) {
        const auto low=custom::MakeSliderGeometry(10,390,half,0.f),high=custom::MakeSliderGeometry(10,390,half,1.f);
        assert(low.center-low.halfWidth==low.start && low.fill==low.start);
        assert(high.center+high.halfWidth==high.end && high.fill==high.end);
    }
    Down(toggleRect.GetCenter());assert(!enabled);Up();assert(enabled);
    ResetScroll();Pan(toggleRect.GetCenter());assert(enabled && panel->Scroll.y>50.f);
    ResetScroll();const auto sliderPoint=ImVec2(sliderRect.GetCenter().x,sliderRect.Max.y-9.f);
    Down(sliderPoint);assert(slider==25.f);Move(sliderPoint+ImVec2(2,-45));Up();assert(slider==25.f && panel->Scroll.y>30.f);
    ResetScroll();Down({sliderRect.Max.x-3,sliderRect.Max.y-9});assert(slider==25.f);Up();assert(slider==100.f);
    ResetScroll();Down({sliderRect.Min.x+3,sliderRect.Max.y-9});Up();assert(slider==0.f);
    ResetScroll();Down({sliderRect.Min.x+10,sliderRect.Max.y-9});Move({sliderRect.Max.x-3,sliderRect.Max.y-9});assert(slider==100.f);Up();
    ResetScroll();slider=25.f;Down({sliderRect.Min.x+10,sliderRect.Max.y-9});
    io.ConfigInputTrickleEventQueue=false;
    io.AddMousePosEvent(sliderRect.Max.x-3,sliderRect.Max.y-9);io.AddMouseButtonEvent(0,false);Frame();Frame();assert(slider==100.f);
    io.ConfigInputTrickleEventQueue=true;
    ResetScroll();Pan(comboRect.GetCenter());assert(!opened && panel->Scroll.y>50.f);
    ResetScroll();Down(comboRect.GetCenter());assert(!opened);Up();assert(opened);
    const auto first=rows[0].GetCenter();Pan(first);assert(opened && selection==0);
    ImGui::GetIO().AddMousePosEvent(560,560);Frame();ImGui::GetIO().AddMouseButtonEvent(0,true);Frame();Up();assert(!opened);
    ResetScroll();Down(toggleRect.GetCenter());custom::CancelTouchScroll();Up();assert(enabled);
    ResetScroll();Pan(buttonRect.GetCenter());assert(presses==0 && panel->Scroll.y>50.f);
    ImGui::DestroyContext();std::puts("Touch scrolling, gesture cancellation, slider endpoints/taps, dropdowns and child geometry passed");
}
