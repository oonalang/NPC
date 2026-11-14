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
    assert(transition.At(100.).morph==0.f && transition.At(100.).label==1.f);
    assert(transition.At(100.30).label==0.f && transition.At(100.30).morph==0.f);
    assert(transition.At(100.60).morph>.49f && transition.At(100.60).morph<.51f);
    assert(transition.At(100.75).morph==1.f);
    assert(transition.At(100.9).rotation>0.f && transition.At(100.9).rotation<6.284f);
    const auto waiting=transition.At(110.);
    assert(waiting.droplet==0.f && waiting.check==0.f && waiting.opacity==1.f && !waiting.ready);
    transition.Resolve(invalid,true);
    assert(!transition.succeeded && transition.active);
    transition.Resolve(100.1,true);
    const double accepted=transition.accepted;
    assert(std::fabs(accepted-102.45)<.000001);
    transition.Resolve(101.,true);
    transition.Resolve(101.,false);
    assert(transition.active && transition.succeeded && transition.accepted==accepted);
    const auto at=[&](double offset) { return transition.At(accepted+offset); };
    assert(at(-.01).droplet==0.f && at(-.01).rotation>0.f);
    assert(at(0.).droplet==0.f && at(0.).rotation==0.f && at(0.).granted);
    assert(at(.125).droplet>.49f && at(.125).droplet<.51f);
    assert(at(.25).droplet>=.99f && at(.25).fall<.001f);
    assert(at(.5).fall>.42f && at(.5).fall<.44f);
    assert(at(.70).fall>=.99f && at(.70).splash<.001f);
    assert(at(1.).splash>.66f && at(1.).splash<.68f);
    assert(at(1.15).splash>=.99f && at(1.15).wave<.001f);
    assert(at(1.55).wave>.49f && at(1.55).wave<.51f);
    assert(at(1.95).wave>=.99f && at(1.95).check<.001f);
    assert(at(2.1).check>.42f && at(2.1).check<.44f);
    assert(at(2.3).check>=.99f && at(2.3).suck==0.f && at(2.3).ready==false);
    assert(at(2.6).suck>.14f && at(2.6).suck<.15f);
    assert(at(3.2).suck>=.99f && at(3.2).opacity<=.01f && at(3.2).hole<.001f);
    assert(at(3.28).hole>.52f && at(3.28).hole<.55f);
    assert(at(3.35).hole>=.99f && at(3.35).flash<.001f);
    assert(at(3.5).flash>=.99f && at(3.5).shock<.001f);
    assert(at(4.4).shock>.49f && at(4.4).shock<.51f);
    assert(at(5.3).shock>=.99f && at(5.3).ready==false);
    assert(at(5.64).ready==false && at(5.65).ready==true);
    assert(!transition.At(invalid).ready);
    transition.Reset();
    assert(transition.Start(200.));
    transition.Resolve(200.1,false);
    assert(!transition.active && !transition.succeeded && !transition.At(210.).ready);
    transition.Resolve(210.,true);
    assert(!transition.active && !transition.succeeded);
    assert(transition.Start(300.));
    assert(!transition.At(310.).ready && transition.At(310.).wave==0.f);
    transition.Resolve(310.,true);
    assert(transition.At(310.).wave==0.f && !transition.At(310.).ready);
    assert(transition.At(311.55).wave>.49f && transition.At(311.55).wave<.51f);
    assert(!transition.At(315.64).ready && transition.At(315.66).ready);
    for(int rate : {30,60,120}) {
        transition.Reset();
        assert(transition.Start(0.));
        transition.Resolve(.01,true);
        float droplet=0.f,fall=0.f,splash=0.f,wave=0.f,check=0.f,suck=0.f,hole=0.f,flash=0.f,shock=0.f,opacity=1.f;
        double firstWave=-1.,firstReady=-1.;
        for(int i=0;i<=rate*10;++i) {
            const double now=double(i)/rate;
            const auto frame=transition.At(now);
            assert(frame.droplet>=droplet && frame.fall>=fall && frame.splash>=splash && frame.wave>=wave);
            assert(frame.check>=check && frame.suck>=suck && frame.hole>=hole && frame.flash>=flash && frame.shock>=shock);
            assert(frame.opacity<=opacity && frame.opacity>=0.f);
            assert(frame.label>=0.f && frame.label<=1.f && frame.morph<=1.f && frame.rotation>=0.f && frame.rotation<6.284f);
            if(frame.wave>0.f && firstWave<0.) firstWave=now;
            if(frame.ready && firstReady<0.) firstReady=now;
            if(frame.ready) assert(frame.opacity<=.01f && frame.check>=.99f && frame.shock>=.99f);
            droplet=frame.droplet;fall=frame.fall;splash=frame.splash;wave=frame.wave;check=frame.check;
            suck=frame.suck;hole=frame.hole;flash=frame.flash;shock=frame.shock;opacity=frame.opacity;
        }
        assert(firstWave>=3.60 && firstWave<=3.60+1./rate);
        assert(firstReady>=8.10 && firstReady<=8.10+1./rate);
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
