#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/System/UI/StartupUI.h"
#include "../src/main/jni/System/UI/StartupState.h"
#include "../src/main/jni/ImGui/GlassTiming.h"
#include <cassert>
#include <limits>
#include <cstdio>
#include <vector>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr; ImFont* inter_title=nullptr; }

void CheckLoginTransition() {
    startup_ui::LoginTransition transition;
    const double invalid=std::numeric_limits<double>::quiet_NaN();
    assert(!transition.Start(invalid) && !transition.active);
    assert(!transition.Start(std::numeric_limits<double>::infinity()));
    assert(!transition.At(100.).ready && transition.At(100.).opacity==1.f);
    assert(transition.Start(100.) && !transition.Start(101.));
    assert(transition.started==100. && transition.active && !transition.succeeded);
    assert(transition.At(100.).morph==0.f);
    assert(transition.At(100.12).morph>.49f && transition.At(100.12).morph<.51f);
    assert(transition.At(100.25).morph==1.f && transition.At(100.49).rotation>0.f);
    const auto waiting=transition.At(110.);
    assert(waiting.fill==0.f && waiting.check==0.f && waiting.opacity==1.f && !waiting.ready);
    transition.Resolve(invalid,true);
    assert(!transition.succeeded && transition.active);
    transition.Resolve(100.1,true);
    const double accepted=transition.accepted;
    transition.Resolve(101.,true);
    transition.Resolve(101.,false);
    assert(transition.active && transition.succeeded && transition.accepted==accepted);
    assert(transition.At(102.23).fill==0.f && !transition.At(102.23).ready);
    const auto filling=transition.At(102.40);
    assert(filling.fill>0.f && filling.fill<1.f && filling.check==0.f && filling.opacity==1.f);
    const auto checking=transition.At(102.66);
    assert(checking.fill==1.f && checking.check>0.f && checking.check<1.f && checking.opacity==1.f);
    const auto holding=transition.At(103.0);
    assert(holding.fill==1.f && holding.check==1.f && holding.opacity==1.f && !holding.ready);
    const auto fading=transition.At(103.32);
    assert(fading.check==1.f && fading.opacity>0.f && fading.opacity<1.f && !fading.ready);
    assert(transition.At(103.44).ready && transition.At(103.44).opacity==0.f);
    assert(!transition.At(invalid).ready);
    transition.Reset();
    assert(transition.Start(200.));
    transition.Resolve(200.1,false);
    assert(!transition.active && !transition.succeeded && !transition.At(210.).ready);
    transition.Resolve(210.,true);
    assert(!transition.active && !transition.succeeded);
    assert(transition.Start(300.));
    assert(!transition.At(310.).ready && transition.At(310.).fill==0.f);
    transition.Resolve(310.,true);
    assert(transition.At(310.).fill==0.f && !transition.At(310.).ready);
    assert(transition.At(310.16).fill>.49f && transition.At(310.16).fill<.51f);
    assert(transition.At(311.20).ready);
    for(int rate : {30,60,120}) {
        transition.Reset();
        assert(transition.Start(0.));
        transition.Resolve(.01,true);
        float previousMorph=0.f,previousFill=0.f,previousCheck=0.f,previousOpacity=1.f;
        double firstFill=-1.,firstReady=-1.;
        for(int i=0;i<=rate*5;++i) {
            const double now=double(i)/rate;
            const auto frame=transition.At(now);
            assert(frame.morph>=previousMorph && frame.fill>=previousFill && frame.check>=previousCheck);
            assert(frame.opacity<=previousOpacity && frame.opacity>=0.f);
            assert(frame.morph<=1.f && frame.fill<=1.f && frame.check<=1.f);
            assert(frame.rotation>=0.f && frame.rotation<6.284f);
            if(frame.fill>0.f && firstFill<0.) firstFill=now;
            if(frame.ready && firstReady<0.) firstReady=now;
            if(frame.ready) assert(frame.opacity==0.f && frame.check==1.f);
            previousMorph=frame.morph;previousFill=frame.fill;previousCheck=frame.check;previousOpacity=frame.opacity;
        }
        assert(firstFill>=2.24 && firstFill<=2.24+1./rate);
        assert(firstReady>=3.43 && firstReady<=3.43+1./rate);
    }
}

int main() {
    CheckLoginTransition();
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
    io.IniFilename=nullptr;io.DisplaySize={1280,576};io.DeltaTime=1.f/60;
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
    auto assertCentered=[&] {
        const auto* login=ImGui::FindWindowByName("Login Menu");
        assert(login && ImLengthSqr(login->Pos+login->Size*.5f-ImGui::GetMainViewport()->GetCenter())<=2.f);
    };
    frame();assertCentered();
    for(const auto size : {ImVec2(2400,1080),ImVec2(1600,720),ImVec2(1280,576),ImVec2(1280,720)}) {
        io.DisplaySize=size;frame();assertCentered();
    }
    frame();
    const ImVec2 original=ImGui::FindWindowByName("Login Menu")->Pos;
    const auto pos=original+ImVec2(10,10);
    click(pos+ImVec2(200,308));assert(actions.paste && !actions.submit && !actions.load);
    click(pos+ImVec2(512,308));assert(actions.load && !actions.paste && !actions.submit);
    click(pos+ImVec2(360,391));assert(actions.submit);
    pending=true;click(pos+ImVec2(360,391));assert(!actions.submit);
    pending=false;
    assert(startup_ui::loginTransition.Start(startup_ui::Now()));
    for(const auto target : {pos+ImVec2(200,308),pos+ImVec2(512,308),pos+ImVec2(360,391),field.GetCenter()}) {
        click(target);
        assert(!actions.paste && !actions.load && !actions.submit && !keyboard);
    }
    startup_ui::loginTransition.Resolve(startup_ui::Now(),false);
    click(field.GetCenter());frame();
    assert(keyboard && field.Max.y<=io.DisplaySize.y*.44f);
    for(const auto size : {ImVec2(1600,720),ImVec2(1280,576),ImVec2(1280,720)}) {
        io.DisplaySize=size;frame();
        assert(keyboard && field.Max.y<=io.DisplaySize.y*.44f);
    }
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
    ImGui::NewFrame();
    ImGui::Begin("Login fade regression");
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled({50,50},{70,70},IM_COL32(10,20,30,200));
    const int first=draw->VtxBuffer.Size;
    draw->AddRectFilled({80,50},{100,70},IM_COL32(40,50,60,240));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,.7f);
    draw->AddRectFilled({110,50},{130,70},ImGui::GetColorU32(ImVec4(.3f,.4f,.5f,.8f)));
    ImGui::PopStyleVar();
    std::vector<ImU32> colors;
    for(const auto& vertex : draw->VtxBuffer) colors.push_back(vertex.col);
    startup_ui::FadeLogin(draw,first,.5f);
    for(int i=0;i<draw->VtxBuffer.Size;++i) {
        const auto before=colors[i],after=draw->VtxBuffer[i].col;
        assert((before&~IM_COL32_A_MASK)==(after&~IM_COL32_A_MASK));
        assert((after>>IM_COL32_A_SHIFT)==(i<first?(before>>IM_COL32_A_SHIFT):(before>>IM_COL32_A_SHIFT)/2));
    }
    startup_ui::FadeLogin(draw,first,0.f);
    for(int i=first;i<draw->VtxBuffer.Size;++i) assert((draw->VtxBuffer[i].col&IM_COL32_A_MASK)==0);
    ImGui::End();ImGui::Render();
    ImGui::DestroyContext();
    std::puts("Startup actions, authentication transitions, default and resized centering, keyboard layout, direct dragging, loading timing and capture budgets passed");
}
