#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/System/UI/StartupUI.h"
#include "../src/main/jni/System/UI/StartupState.h"
#include "../src/main/jni/System/UI/AuthenticateInput.h"
#include "../src/main/jni/ImGui/GlassTiming.h"
#include <cassert>
#include <limits>
#include <cstdio>
#include <array>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr; ImFont* inter_title=nullptr; }

int touchPolls=0;
int FakeTouchCount() { ++touchPolls; return 5; }
void CheckTouchInputGate() {
    auth_input::SetBlocked(false);
    assert(!auth_input::Blocked());
    assert(auth_input::TouchCount(FakeTouchCount)==5 && touchPolls==1);
    auth_input::SetBlocked(true);
    assert(auth_input::Blocked());
    for(int i=0;i<3;++i) assert(auth_input::TouchCount(FakeTouchCount)==0);
    assert(touchPolls==1 && auth_input::TouchCount(nullptr)==0);
    auth_input::SetBlocked(false);
    assert(!auth_input::Blocked() && auth_input::TouchCount(nullptr)==0 && touchPolls==1);
    assert(auth_input::TouchCount(FakeTouchCount)==5 && touchPolls==2);
}

void CheckLoadingTipClipping() {
    auto& io=ImGui::GetIO();
    const float scale=startup_ui::Fit({680,600});
    const auto size=ImVec2(680,600)*scale;
    const auto pos=(io.DisplaySize-size)*.5f;
    const auto center=pos+size*.5f;
    for(float progress : {.3f,1.f}) for(float time : {2.49f,2.4999f,2.5f,2.5001f,2.51f,
             4.99f,4.9999f,5.f,5.0001f,5.01f,7.49f,7.4999f,7.5f,7.5001f,7.51f}) {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize,ImGuiCond_Always);
        ImGui::Begin("Loading clip regression",nullptr,ImGuiWindowFlags_NoDecoration|
            ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove);
        auto* draw=ImGui::GetWindowDrawList();
        const int clipDepth=draw->_ClipRectStack.Size;
        const int styleDepth=GImGui->StyleVarStack.Size,colorDepth=GImGui->ColorStack.Size;
        const startup_ui::LoadingReveal reveal{progress};
        startup_ui::LoadingCard(draw,pos,size,scale,.4f,time,&reveal);
        assert(draw->_ClipRectStack.Size==clipDepth);
        assert(GImGui->StyleVarStack.Size==styleDepth && GImGui->ColorStack.Size==colorDepth);
        const auto clipLo=center+(pos+ImVec2(30,134)*scale-center)*reveal.Scale();
        const auto clipHi=center+(pos+ImVec2(650,160)*scale-center)*reveal.Scale();
        bool visibleTipClip=false;
        for(const auto& command : draw->CmdBuffer) {
            const auto& clip=command.ClipRect;
            assert(std::isfinite(clip.x) && std::isfinite(clip.y) && std::isfinite(clip.z) && std::isfinite(clip.w));
            assert(clip.x<=clip.z && clip.y<=clip.w);
            assert(command.IdxOffset+command.ElemCount<=static_cast<unsigned>(draw->IdxBuffer.Size));
            for(unsigned i=command.IdxOffset;i<command.IdxOffset+command.ElemCount;++i)
                assert(draw->IdxBuffer[i]+command.VtxOffset<static_cast<unsigned>(draw->VtxBuffer.Size));
            if(command.ElemCount && std::fabs(clip.x-clipLo.x)<.001f && std::fabs(clip.y-clipLo.y)<.001f &&
                std::fabs(clip.z-clipHi.x)<.001f && std::fabs(clip.w-clipHi.y)<.001f) visibleTipClip=true;
        }
        if(startup_ui::TipAt(time).opacity>.01f) assert(visibleTipClip);
        ImGui::End();
        ImGui::Render();
        assert(GImGui->StyleVarStack.Size==0 && GImGui->ColorStack.Size==0);
    }
}

void CheckLoginTransition() {
    using namespace startup_ui;
    static_assert(AuthTiming::labelEndMs==150. && AuthTiming::collapseEndMs==450.);
    static_assert(AuthTiming::spinMs==2000. && AuthTiming::spinEndMs==2450.);
    static_assert(AuthTiming::morphEndMs==2700. && AuthTiming::hoverEndMs==2800.);
    static_assert(AuthTiming::fallEndMs==3150. && AuthTiming::splashEndMs==3600.);
    static_assert(AuthTiming::waveEndMs==4400. && AuthTiming::checkEndMs==4750.);
    static_assert(AuthTiming::checkHoldMs==500. && AuthTiming::suckStartMs==5250.);
    static_assert(AuthTiming::suckEndMs==5950. && AuthTiming::singularityEndMs==6100.);
    static_assert(AuthTiming::flashEndMs==6250. && AuthTiming::explosionEndMs==8050.);
    static_assert(AuthTiming::calmMs==350. && AuthTiming::revealStartMs==8400.);
    static_assert(AuthTiming::revealMs==450. && AuthTiming::revealEndMs==8850.);
    struct Phase { AuthStage stage; double first,last; };
    const std::array<Phase,17> phases{{
        {AuthStage::LabelFade,0.,150.}, {AuthStage::Collapse,150.,450.},
        {AuthStage::Spin,450.,2450.}, {AuthStage::Droplet,2450.,2700.},
        {AuthStage::Hover,2700.,2800.}, {AuthStage::Fall,2800.,3150.},
        {AuthStage::Splash,3150.,3600.}, {AuthStage::Wave,3600.,4400.},
        {AuthStage::Check,4400.,5250.}, {AuthStage::Suck,5250.,5950.},
        {AuthStage::Singularity,5950.,6100.}, {AuthStage::Flash,6100.,6250.},
        {AuthStage::Explosion,6250.,8050.}, {AuthStage::Calm,8050.,8400.},
        {AuthStage::Reveal,8400.,8850.}, {AuthStage::Loading,8850.,9600.},
        {AuthStage::Loading,9600.,10000.}
    }};
    LoginTransition transition;
    const double invalid=std::numeric_limits<double>::quiet_NaN();
    assert(!transition.Start(invalid) && !transition.active);
    assert(!transition.Start(std::numeric_limits<double>::infinity()));
    const auto idle=transition.At(100.);
    assert(idle.stage==AuthStage::Idle && idle.loginVisible && !idle.loadingVisible);
    assert(!idle.ready && !idle.inputBlocked && !idle.granted);
    assert(transition.Start(0.) && !transition.Start(1.));
    assert(transition.started==0. && transition.active && !transition.succeeded);
    transition.Resolve(invalid,true);
    assert(!transition.succeeded && transition.active);
    transition.Resolve(.01,true);
    const double accepted=transition.accepted;
    transition.Resolve(1.,true);
    transition.Resolve(1.,false);
    assert(transition.active && transition.succeeded && transition.accepted==accepted);
    const auto sample=[&](double ms) { return transition.At(ms/1000.); };
    for(const auto& phase : phases) {
        assert(sample(phase.first+1.).stage==phase.stage);
        assert(sample(phase.last-1.).stage==phase.stage);
    }
    const auto initial=sample(0.);
    assert(initial.button.labelOpacity==1.f && initial.button.collapse==0.f);
    assert(sample(75.).button.labelOpacity==.5f && sample(149.).button.collapse==0.f);
    assert(sample(150.).button.labelOpacity==0.f);
    const auto moving=sample(300.).button;
    assert(moving.collapse==.5f && moving.spinnerRotation==0.f);
    const ImVec2 origin(80,90),fullSize(600,66),center=origin+fullSize*.5f;
    const auto idleSurface=button_art::SurfaceAt(origin,fullSize,initial.button);
    const auto spinSurface=button_art::SurfaceAt(origin,fullSize,sample(450.).button);
    assert(idleSurface.lo.x==origin.x && idleSurface.size.x==fullSize.x);
    assert(spinSurface.size.x==spinSurface.size.y && spinSurface.size.x<fullSize.y);
    assert(ImLengthSqr(spinSurface.lo+spinSurface.size*.5f-center)<.0001f);
    assert(button_art::SurfaceAt(origin,fullSize,sample(2750.).button).opacity==0.f);
    const auto restored=button_art::SurfaceAt(origin,fullSize,sample(3150.).button);
    assert(restored.size.x==fullSize.x && restored.opacity==1.f);
    for(double time=0.;time<=4400.;time+=1000./60.) {
        const auto surface=button_art::SurfaceAt(origin,fullSize,sample(time).button);
        assert(surface.opacity>=0.f && surface.opacity<=1.f);
        assert(surface.lo.x>=origin.x && surface.lo.y>=origin.y);
        assert(surface.lo.x+surface.size.x<=origin.x+fullSize.x+.001f);
        assert(surface.lo.y+surface.size.y<=origin.y+fullSize.y+.001f);
        assert(ImLengthSqr(surface.lo+surface.size*.5f-center)<.0001f);
    }
    assert(sample(450.).button.collapse==1.f);
    assert(sample(450.).button.spinnerRotation==0.f);
    const float fullTurn=static_cast<float>(6.283185307179586*AuthTiming::spinTurns);
    assert(sample(500.).button.spinnerRotation<fullTurn*.01f);
    assert(std::fabs(sample(1450.).button.spinnerRotation-fullTurn*.5f)<.00001f);
    assert(sample(2400.).button.spinnerRotation>fullTurn*.99f);
    assert(std::fabs(std::sin(sample(2450.).button.spinnerRotation))<.00001f);
    assert(std::fabs(std::cos(sample(2450.).button.spinnerRotation)-1.f)<.00001f);
    assert(sample(2575.).button.dropletMorph==.5f);
    const auto hover=sample(2750.).button;
    assert(hover.stage==ButtonStage::Hover && hover.dropletMorph==1.f && hover.fall==0.f);
    assert(sample(2975.).button.fall==.5f && sample(2975.).button.splash==0.f);
    assert(sample(3375.).button.splash==.5f && sample(3375.).button.wave==0.f);
    assert(sample(4000.).button.wave==.5f && sample(4000.).button.check==0.f);
    assert(!sample(4399.).granted && sample(4399.).button.check==0.f);
    assert(sample(4400.).granted && sample(4400.).button.wave==1.f);
    assert(sample(4575.).button.check==.5f);
    assert(sample(4751.).button.check==1.f && sample(5249.).button.check==1.f);
    for(double ms : {8050.1,8100.,8250.,8399.9}) {
        const auto calm=sample(ms);
        assert(calm.stage==AuthStage::Calm && !calm.loginVisible && !calm.loadingVisible);
        assert(calm.sceneProgress==0.f && calm.loadingReveal==0.f && calm.inputBlocked && !calm.ready);
    }
    assert(!sample(8399.9).loadingVisible);
    const auto reveal=sample(8400.1);
    assert(reveal.loadingVisible && !reveal.loginVisible && reveal.inputBlocked && !reveal.ready);
    assert(sample(8849.9).inputBlocked && !sample(8849.9).ready);
    const auto complete=sample(8850.1);
    assert(complete.stage==AuthStage::Loading && complete.loadingVisible && !complete.loginVisible);
    assert(complete.loadingReveal==1.f && complete.ready && !complete.inputBlocked);
    assert(!transition.At(invalid).ready);
    transition.Presented(sample(4800.));
    assert(!transition.checkPresented);
    transition.Reset();
    assert(transition.Start(200.));
    transition.Resolve(200.1,false);
    assert(!transition.active && !transition.succeeded && !transition.At(210.).ready);
    assert(!transition.At(210.).inputBlocked && !transition.At(210.).loadingVisible);
    transition.Resolve(210.,true);
    assert(!transition.active && !transition.succeeded);
    assert(transition.Start(300.));
    for(double now : {302.45,310.,400.}) {
        const auto waiting=transition.At(now);
        assert(waiting.stage==AuthStage::Spin && waiting.inputBlocked && waiting.loginVisible);
        assert(!waiting.ready && !waiting.granted && !waiting.loadingVisible);
        assert(waiting.button.dropletMorph==0.f && waiting.button.wave==0.f && waiting.button.check==0.f);
        assert(std::fabs(std::sin(waiting.button.spinnerRotation))<.00001f);
        assert(std::fabs(std::cos(waiting.button.spinnerRotation)-1.f)<.00001f);
    }
    transition.Resolve(310.,true);
    assert(std::fabs(transition.delayMs-7550.)<.00001);
    const auto resumed=transition.At(310.0001);
    assert(resumed.stage==AuthStage::Droplet && !resumed.granted && !resumed.loadingVisible);
    assert(transition.At(311.9499).button.check==0.f && !transition.At(311.9499).granted);
    assert(transition.At(311.9501).stage==AuthStage::Check && transition.At(311.9501).granted);
    assert(!transition.At(316.3999).ready && transition.At(316.4001).ready);
    for(int rate : {30,60,120}) {
        transition.Reset();
        assert(transition.Start(0.));
        transition.Resolve(.01,true);
        std::array<double,static_cast<int>(AuthStage::Loading)+1> first;
        first.fill(-1.);
        float previousMove=0.f,previousWave=0.f,previousCheck=0.f;
        double firstLoading=-1.,firstReady=-1.;
        for(int i=0;i<=rate*10;++i) {
            const double now=double(i)/rate;
            const auto frame=transition.At(now);
            const auto stage=static_cast<int>(frame.stage);
            if(first[stage]<0.) first[stage]=now*1000.;
            assert(frame.button.collapse>=previousMove && frame.button.wave>=previousWave && frame.button.check>=previousCheck);
            assert(frame.button.collapse<=1.f && frame.button.wave<=1.f && frame.button.check<=1.f);
            assert(frame.button.spinnerRotation>=0.f && frame.button.spinnerRotation<=fullTurn+.00001f);
            if(now*1000.<4400.-.000001) assert(!frame.granted && frame.button.check==0.f);
            if(now*1000.<8400.-.000001) assert(!frame.loadingVisible);
            if(now*1000.<8850.-.000001) assert(frame.inputBlocked && !frame.ready);
            if(frame.loadingVisible && firstLoading<0.) firstLoading=now;
            if(frame.ready && firstReady<0.) firstReady=now;
            if(frame.ready) assert(!frame.inputBlocked && frame.loadingReveal==1.f);
            previousMove=frame.button.collapse;previousWave=frame.button.wave;previousCheck=frame.button.check;
        }
        for(size_t i=0;i<phases.size()-1;++i) {
            const auto& phase=phases[i];
            const double observed=first[static_cast<int>(phase.stage)];
            assert(observed>=phase.first-.000001 && observed<=phase.first+1000./rate+.000001);
        }
        assert(firstLoading>=8.4-.000001 && firstLoading<=8.4+1./rate+.000001);
        assert(firstReady>=8.85-.000001 && firstReady<=8.85+1./rate+.000001);
    }
    for(float progress : {0.f,.125f,.25f,.5f,.75f,1.f}) {
        const LoadingReveal reveal{progress};
        assert(reveal.Scale()>=.9f && reveal.Scale()<=1.f);
        float previous=1.f;
        for(int i=0;i<5;++i) {
            const float group=reveal.Group(i);
            assert(group>=0.f && group<=1.f && group<=previous);
            if(progress==0.f) assert(group==0.f);
            if(progress==1.f) assert(group==1.f);
            previous=group;
        }
    }
    assert(LoadingReveal{0.f}.Scale()==.9f && LoadingReveal{1.f}.Scale()==1.f);
    const LoadingReveal stagger{.25f};
    assert(stagger.Group(0)>stagger.Group(1) && stagger.Group(1)>stagger.Group(2));
    assert(stagger.Group(3)==0.f && stagger.Group(4)==0.f);
    transition.Reset();
    assert(transition.Start(500.,true));
    const auto reducedWaiting=transition.At(501.);
    assert(reducedWaiting.stage==AuthStage::Spin && reducedWaiting.inputBlocked);
    assert(!reducedWaiting.granted && !reducedWaiting.loadingVisible);
    transition.Presented(reducedWaiting);
    assert(!transition.checkPresented);
    transition.Resolve(501.,true);
    const auto reducedCheck=transition.At(501.);
    assert(reducedCheck.stage==AuthStage::Check && reducedCheck.button.check==1.f && reducedCheck.granted);
    assert(reducedCheck.loginVisible && !reducedCheck.loadingVisible && reducedCheck.inputBlocked && !reducedCheck.ready);
    assert(transition.At(1000.).stage==AuthStage::Check);
    transition.Presented(reducedCheck);
    const auto reducedLoading=transition.At(1000.);
    assert(reducedLoading.stage==AuthStage::Loading && reducedLoading.loadingVisible && !reducedLoading.loginVisible);
    assert(reducedLoading.loadingReveal==1.f && !reducedLoading.inputBlocked && reducedLoading.ready);
    transition.Reset();
    assert(transition.Start(600.,true));
    transition.Resolve(600.1,false);
    assert(!transition.active && !transition.At(1000.).granted && !transition.At(1000.).loadingVisible);
}

int main() {
    CheckTouchInputGate();
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
    CheckLoadingTipClipping();
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
    ImGui::DestroyContext();
    std::puts("Startup actions, storyboard phases and frame timing, success and reduced-motion gates, default and resized centering, keyboard layout, direct dragging, loading timing and capture budgets passed");
}
