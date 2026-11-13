#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/System/UI/StartupUI.h"
#include "../src/main/jni/System/UI/StartupState.h"
#include "../src/main/jni/ImGui/GlassTiming.h"
#include <cassert>
#include <limits>
#include <cstdio>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr; ImFont* inter_title=nullptr; }

int main() {
    startup_ui::LoadingTimeline timeline;
    timeline.Start(100.,7.);
    assert(timeline.active && timeline.Progress(100.)==0.f);
    assert(std::fabs(timeline.Progress(103.5)-.5f)<.0001f);
    assert(!timeline.Finish(106.99) && timeline.active);
    assert(timeline.Finish(107.) && !timeline.active && !timeline.Finish(108.));
    timeline.Start(200.,-5.);
    assert(timeline.Progress(199.)==0.f && timeline.Progress(205.)==1.f);
    timeline.Start(300.,7.);
    assert(timeline.Progress(304.2)>.599f && timeline.Progress(304.2)<.601f);
    assert(timeline.Progress(std::numeric_limits<double>::quiet_NaN())==0.f);
    for(int rate : {30,60,120}) {
        clay::CaptureSchedule normal,moving;
        int normalCount=0,movingCount=0;
        for(int i=0;i<=rate;++i) {
            normalCount+=normal.Due(double(i)/rate,false);
            movingCount+=moving.Due(double(i)/rate,true);
        }
        assert(normalCount<=31 && movingCount<=21);
        assert(normalCount>=30 && movingCount>=16);
        moving.Reset(); assert(moving.Due(1.,true));
    }
    assert(startup_ui::TipAt(0.).index==0 && startup_ui::TipAt(3.).index==1 && startup_ui::TipAt(5.5).index==2);
    assert(startup_ui::TipAt(7.6).index==0 && startup_ui::TipAt(-1.).index==0);
    assert(startup_ui::media::FrameAt(0.)==0 && startup_ui::media::FrameAt(-1.)==0);
    assert(startup_ui::media::FrameAt(startup_ui::media::bannerDurationMs/1000.0)==0);
    assert(startup_ui::media::FrameAt(.2)>0);
    for(float aspect : {1.f,2.7f,3.2f,4.f}) for(int i=0;i<100;++i) {
        const auto crop=startup_ui::media::BannerCrop(aspect,i*.1);
        assert(startup_ui::media::FrameAt(i*.1)>=0 && startup_ui::media::FrameAt(i*.1)<startup_ui::media::bannerFrameCount);
        assert(crop.lo.x>=0.f && crop.lo.y>=0.f && crop.hi.x<=1.f && crop.hi.y<=1.f);
        assert(crop.hi.x>crop.lo.x && crop.hi.y>crop.lo.y);
        const auto tip=startup_ui::TipAt(i*.1);
        assert(tip.index>=0 && tip.index<3 && tip.opacity>=0.f && tip.opacity<=1.f);
    }
    ImGui::CreateContext(); auto& io=ImGui::GetIO();
    io.IniFilename=nullptr;io.DisplaySize={1280,720};io.DeltaTime=1.f/60;
    unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    c::ApplyTheme();
    char key[256]="TEST";bool keyboard=false,pending=false;
    startup_ui::LoginActions actions;ImRect field;
    auto frame=[&] {
        ImGui::NewFrame();
        actions=startup_ui::RenderLogin(key,sizeof(key),pending,"",true,keyboard,
            [&](const char* id,char*,size_t,ImVec2 size) {
                ImGui::InvisibleButton(id,size);
                field=GImGui->LastItemData.Rect;
            });
        ImGui::Render();
        assert(GImGui->StyleVarStack.Size==0 && GImGui->ColorStack.Size==0);
    };
    auto click=[&](ImVec2 where) {
        io.AddMousePosEvent(where.x,where.y);frame();
        io.AddMouseButtonEvent(0,true);frame();
        io.AddMouseButtonEvent(0,false);frame();
    };
    frame();frame();
    const ImVec2 original=ImGui::FindWindowByName("Login Menu")->Pos;
    const auto pos=original+ImVec2(10,10);
    click(pos+ImVec2(200,308));assert(actions.paste && !actions.submit && !actions.load);
    click(pos+ImVec2(512,308));assert(actions.load && !actions.paste && !actions.submit);
    click(pos+ImVec2(360,391));assert(actions.submit);
    pending=true;click(pos+ImVec2(360,391));assert(!actions.submit);
    pending=false;click(field.GetCenter());frame();
    assert(keyboard && field.Max.y<=io.DisplaySize.y*.44f);
    io.AddMousePosEvent(20,20);frame();io.AddMouseButtonEvent(0,true);frame();
    io.AddMouseButtonEvent(0,false);frame();frame();
    assert(!keyboard);
    auto* window=ImGui::FindWindowByName("Login Menu");
    assert(ImLengthSqr(window->Pos-original)<1.f);
    const auto start=window->Pos;
    const auto grab=start+ImVec2(80,85);
    io.AddMousePosEvent(grab.x,grab.y);frame();
    io.AddMouseButtonEvent(0,true);frame();frame();
    for(int i=1;i<=12;++i) {
        io.AddMousePosEvent(grab.x+i*8,grab.y+i*3);frame();
        assert(ImLengthSqr(window->Pos-(start+ImVec2(i*8,i*3)))<=2.f);
    }
    io.AddMouseButtonEvent(0,false);frame();
    assert(GImGui->MovingWindow==nullptr);
    ImGui::NewFrame();ImGui::Render();
    frame();
    window=ImGui::FindWindowByName("Login Menu");
    const auto centered=ImGui::GetMainViewport()->GetCenter()-window->Size*.5f;
    assert(ImLengthSqr(window->Pos-centered)<=2.f);
    ImGui::DestroyContext();
    std::puts("Startup actions, pending state, keyboard layout, direct dragging, loading timing and capture budgets passed");
}
