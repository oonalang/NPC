#define IMGUI_DEFINE_MATH_OPERATORS
#include <cstdint>
struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
#include "../src/main/jni/Hacks/Feature/Init_Hacks.h"
#include "../src/main/jni/Lite/Menu.h"
#include "../src/main/jni/Lite/Fonts.h"
#include "../src/main/jni/Lite/FrostedSidebar.h"
#include "../src/main/jni/Lite/TouchInput.h"
#include "../src/main/jni/Lite/InputRouting.h"
#include <thread>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#ifdef LITE_GPU_TEST
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include "../src/main/jni/ImGui/imgui_impl_opengl3.h"
#endif

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4]{.8f,.8f,.8f,1.f};
float SnowBsize = 0, speedHackMultiplier = 1, jumpHeightMultiplier = 1, MpBsize = 0;
namespace font { ImFont* inter_semibold = nullptr; ImFont* inter_title = nullptr; }

namespace {
std::size_t allocations = 0;
void* Allocate(std::size_t size, void*) { ++allocations; return std::malloc(size); }
void Free(void* pointer, void*) { std::free(pointer); }
int skinCalls = 0, musicCalls = 0;
ImTextureID allowedTexture = nullptr;
void Skins(int category) { ++skinCalls; ImGui::Text("Category %d",category); }
void Music() { ++musicCalls; ImGui::TextUnformatted("Music service fixture"); }
std::vector<unsigned char> Read(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    assert(input.good());
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
void CheckFrame() {
    assert(GImGui->StyleVarStack.Size == 0 && GImGui->ColorStack.Size == 0 && GImGui->DisabledStackSize == 0);
    const auto* data = ImGui::GetDrawData();
    assert(data->TotalVtxCount < 25000 && data->CmdListsCount < 40);
    for (const auto* list : data->CmdLists) {
        for (const auto& vertex : list->VtxBuffer) assert(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y));
        for (const auto& command : list->CmdBuffer) {
            assert(command.UserCallback == nullptr);
            assert(command.TextureId == ImGui::GetIO().Fonts->TexID || (allowedTexture && command.TextureId == allowedTexture));
            assert(command.IdxOffset + command.ElemCount <= static_cast<unsigned>(list->IdxBuffer.Size));
            for (unsigned at = command.IdxOffset; at < command.IdxOffset + command.ElemCount; ++at)
                assert(list->IdxBuffer[at] + command.VtxOffset < static_cast<unsigned>(list->VtxBuffer.Size));
        }
    }
}
lite_ui::State state;
lite_ui::View view;
char key[256] = "FREETRIAL";
lite_ui::Actions Frame() {
    ImGui::NewFrame();
    custom::UpdateTouchScroll();
    const auto result = lite_ui::Draw(state,view,key,sizeof(key));
    ImGui::Render();
    CheckFrame();
    return result;
}
ImGuiWindow* Find(const char* suffix) {
    for (auto* window : GImGui->Windows) {
        const std::string name = window->Name;
        if (name.find(suffix) != std::string::npos && window->Active) return window;
    }
    return nullptr;
}
void Tap(ImVec2 point) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(point.x,point.y); Frame();
    io.AddMouseButtonEvent(0,true); Frame();
    io.AddMouseButtonEvent(0,false); Frame(); Frame();
}
void ValidateLayouts() {
    auto& io = ImGui::GetIO();
    view.ready = view.authorized = true;
    const ImVec2 sizes[]{{1280,720},{960,540},{640,360},{360,780},{320,568},{1920,1080}};
    for (const auto size : sizes) {
        io.DisplaySize = size;
        for (float zoom : {.85f,1.f,1.35f}) {
            state.zoom = zoom;
            for (int tab = 0; tab < 7; ++tab) {
                state.tab = static_cast<lite_ui::Tab>(tab);
                Frame(); Frame();
                auto* root = Find("ETHNIR LITE##menu");
                assert(root && root->Pos.x >= 0 && root->Pos.y >= 0);
                assert(root->Pos.x + root->Size.x <= size.x && root->Pos.y + root->Size.y <= size.y);
                assert(root->Flags & ImGuiWindowFlags_NoMove);
                assert(!Find("ESP PREVIEW"));
                for (const auto* child : GImGui->Windows) if (child->Active && child->ParentWindow &&
                    std::strstr(child->Name,"ETHNIR LITE##menu/"))
                    assert(std::fabs(child->CalcFontSize()-17.f*lite_widgets::scale)<.1f);
            }
        }
    }
    assert(skinCalls > 0 && musicCalls == 0);
    state = {}; io.DisplaySize = {1280,720}; Frame(); Frame();
    state.music = true; Frame(); assert(musicCalls == 1);
    state.tab = lite_ui::Tab::Esp; Frame(); assert(musicCalls == 1);
    state.minimized = true; Frame(); assert(musicCalls == 1 && Find("##lite_restore"));
    state = {};
    std::puts("All seven native pages fit six viewports at three scales; optional music is lazy and the preview is absent");
}
std::uint64_t DrawHash() {
    std::uint64_t result=1469598103934665603ull;
    for (const auto* list : ImGui::GetDrawData()->CmdLists) for (const auto& vertex : list->VtxBuffer) {
        const auto* bytes=reinterpret_cast<const unsigned char*>(&vertex);
        for (std::size_t index=0;index<sizeof(vertex);++index) { result^=bytes[index];result*=1099511628211ull; }
    }
    return result;
}
void ValidateMotion() {
    using namespace lite_startup;
    for (int fps : {15,30,60,90,120}) {
        Motion motion;
        bool sawMenu=false,sawInteractive=false;
        for (int step=0;step<fps*3;++step) {
            const double now=double(step)/fps;
            const auto value=motion.Update(now,true,true);
            if (now<minimumLoadingSeconds+finishSeconds) assert(!value.showMenu);
            if (value.interactive) assert(value.showMenu && value.reveal==1.f);
            if (value.showMenu && !sawMenu) {
                assert(now>=minimumLoadingSeconds+finishSeconds);
                assert(now<=minimumLoadingSeconds+finishSeconds+2./fps+.0001);
                sawMenu=true;
            }
            if (value.interactive && !sawInteractive) {
                assert(now<=minimumLoadingSeconds+finishSeconds+revealSeconds+3./fps+.0001);
                sawInteractive=true;
            }
        }
        assert(sawMenu && sawInteractive);
        assert(!motion.Update(5,false,false).showMenu);
        assert(!motion.Update(6,true,true).showMenu);
    }
    Motion waiting;
    assert(!waiting.Update(0,true,false).showMenu);
    assert(!waiting.Update(20,true,false).showMenu);
    assert(!waiting.Update(20.1,true,true).showMenu);
    assert(waiting.Update(20.6,true,true).showMenu);
    Motion reduced;
    assert(reduced.Update(0,false,false,true).reducedMotion);
    assert(!reduced.Update(1,true,false,true).showMenu);
    assert(reduced.Update(2,true,true,true).interactive);
    Motion rewind;
    rewind.Update(10,true,false);
    assert(std::fabs(rewind.Update(10.5,true,false).elapsed-.5)<.00001);
    assert(std::fabs(rewind.Update(5,true,false).elapsed-.5)<.00001);
    assert(std::fabs(rewind.Update(std::nan(""),true,false).elapsed-.5)<.00001);
    state={}; view.ready=false; view.authorized=true; view.pending=false;
    auto& io=ImGui::GetIO(); io.DisplaySize={960,540};
    view.startup={}; view.startup.elapsed=.2; Frame(); const auto first=DrawHash();
    view.startup.elapsed=.6; Frame(); assert(first!=DrawHash());
    view.startup.reducedMotion=true; Frame(); const auto frozen=DrawHash();
    view.startup.elapsed=5.; Frame(); assert(frozen==DrawHash());
    view.startup={};
    std::puts("Startup: bounded presentation at 15-120 FPS, backend readiness, full reveal input gate, revocation, rewind/NaN clocks, animated geometry and reduced-motion freeze passed");
}
void ValidateStartup() {
    state = {};
    const auto previousMusic = musicCalls, previousSkins = skinCalls;
    auto& io = ImGui::GetIO(); io.DisplaySize = {960,540};
    view.ready = false; view.authorized = false;
    for (bool pending : {false,true}) {
        view.pending = pending;
        Frame(); Frame();
        const auto* login = Find("ETHNIR LITE##login");
        assert(login);
        const auto center = login->Pos + login->Size*.5f;
        assert(ImLengthSqr(center-io.DisplaySize*.5f) < 1.f);
        if (pending) {
            const auto result = Frame();
            assert(!result.submit && !result.paste && !result.loadKey);
        }
    }
    view.authorized = true; view.pending = false; view.preparation = 2.f/3.f;
    Frame(); assert(!Find("ETHNIR LITE##menu"));
    assert(previousMusic == musicCalls && previousSkins == skinCalls);
    view.ready = true; Frame(); assert(Find("ETHNIR LITE##menu"));
    std::puts("Login stays centered, pending controls stay blocked, and menu callbacks wait for feature preparation");
}
ImRect checkRect, sliderRect, buttonRect, inputRect, comboRect, choiceRect;
int choice = 0;
bool comboOpen = false;
bool checked = false, buttonPressed = false;
bool longChoices=false;
ImRect lastChoiceRect;
float slider = .25f;
ImGuiWindow* scrollWindow = nullptr;
char text[32] = "";
void ControlsFrame() {
    ImGui::NewFrame(); custom::UpdateTouchScroll(); lite_ui::Theme(); lite_widgets::scale = 1;
    ImGui::SetNextWindowPos({20,20}); ImGui::SetNextWindowSize({380,280});
    ImGui::Begin("Control fixture",nullptr,ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
    ImGui::BeginChild("Pan controls",{340,200},false,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    scrollWindow = ImGui::GetCurrentWindow();
    lite_widgets::Checkbox("Checkbox",&checked); checkRect = GImGui->LastItemData.Rect;
    lite_widgets::SliderFloat("Slider",&slider,0,1,"%.2f"); sliderRect = GImGui->LastItemData.Rect;
    buttonPressed = lite_widgets::Button("Button",{0,28}); buttonRect = GImGui->LastItemData.Rect;
    lite_ui::Input("Input",text,sizeof(text)); inputRect = GImGui->LastItemData.Rect;
    const char* choices[]{"Choice zero","Choice one","Choice two"};
    comboOpen = custom::BeginCombo("Combo",longChoices?"Choose item":choices[choice],3);
    comboRect = GImGui->LastItemData.Rect;
    if (comboOpen) {
        for (int index=0;index<(longChoices?40:3);++index) {
            char item[32]; std::snprintf(item,sizeof(item),"Choice %02d",index);
            if (custom::Selectable(longChoices?item:choices[index],choice==index)) choice=index;
            if (index==1) choiceRect=GImGui->LastItemData.Rect;
            if (index==39) lastChoiceRect=GImGui->LastItemData.Rect;
        }
        custom::EndCombo();
    }
    ImGui::Dummy({1,900});
    ImGui::EndChild(); ImGui::End();
    if (lite_ui::keyboard) RenderVirtualKeyboard("##lite_keyboard",text,sizeof(text),&lite_ui::keyboard);
    ImGui::Render(); CheckFrame();
}
void ControlTap(ImVec2 point) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(point.x,point.y); ControlsFrame();
    io.AddMouseButtonEvent(0,true); ControlsFrame();
    io.AddMouseButtonEvent(0,false); ControlsFrame(); ControlsFrame();
}
void Drag(ImVec2 from, ImVec2 to) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(from.x,from.y); ControlsFrame();
    io.AddMouseButtonEvent(0,true); ControlsFrame();
    io.AddMousePosEvent(to.x,to.y); ControlsFrame();
    io.AddMouseButtonEvent(0,false); ControlsFrame(); ControlsFrame();
}
void ResetScroll() { scrollWindow->Scroll.y = 0; scrollWindow->ScrollTarget.y = FLT_MAX; ControlsFrame(); ControlsFrame(); }
void ValidateTouch() {
    auto& io = ImGui::GetIO(); io.DisplaySize = {640,640};
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    ControlsFrame(); ControlsFrame();
    ControlTap(checkRect.GetCenter()); assert(checked);
    checked = false;
    Drag(checkRect.GetCenter(),checkRect.GetCenter()-ImVec2(0,90));
    assert(!checked && scrollWindow->Scroll.y > 50);
    ResetScroll();
    const auto initial = slider;
    Drag(sliderRect.GetCenter(),sliderRect.GetCenter()-ImVec2(0,90));
    assert(slider == initial && scrollWindow->Scroll.y > 50);
    ResetScroll();
    Drag({sliderRect.Min.x+3,sliderRect.GetCenter().y},{sliderRect.Max.x,sliderRect.GetCenter().y});
    assert(slider == 1);
    Drag({sliderRect.Max.x-3,sliderRect.GetCenter().y},{sliderRect.Min.x,sliderRect.GetCenter().y});
    assert(slider == 0);
    Drag(buttonRect.GetCenter(),buttonRect.GetCenter()-ImVec2(0,90));
    assert(!buttonPressed && scrollWindow->Scroll.y > 50);
    ResetScroll();
    Drag(comboRect.GetCenter(),comboRect.GetCenter()-ImVec2(0,90));
    assert(!comboOpen && choice==0 && scrollWindow->Scroll.y>50);
    ResetScroll(); ControlTap(comboRect.GetCenter()); assert(comboOpen);
    ControlTap(choiceRect.GetCenter()); assert(choice==1 && !comboOpen);
    ControlTap(inputRect.GetCenter()); assert(lite_ui::keyboard);
    auto* keyboard = Find("##lite_keyboard"); assert(keyboard);
    for(int settle=0;settle<45;++settle) ControlsFrame();
    ControlTap(lite_keyboard::state.keys[lite_keyboard::firstDigit].bounds.GetCenter()); assert(std::strcmp(text,"1") == 0);
    ControlTap(lite_keyboard::state.keys[lite_keyboard::spaceKey].bounds.GetCenter());
    assert(std::strcmp(text,"1 ") == 0);
    ControlTap(lite_keyboard::state.keys[lite_keyboard::deleteKey].bounds.GetCenter());
    assert(std::strcmp(text,"1") == 0);
    lite_ui::keyboard = false;
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse); io.AddMousePosEvent(-FLT_MAX,-FLT_MAX); Frame();
    std::puts("Touch taps work; vertical drags pan without changing controls or opening dropdowns; dropdown selection and slider endpoints work; keyboard types, spaces and deletes");
}
#include "lite_touch_checks.h"
#include "lite_keyboard_size_checks.h"
#include "lite_restore_checks.h"
#include "lite_routing_checks.h"
void ValidateBindings() {
    Config = {}; assert(Config.ExtraMenu.PickupRange == 1 && Config.ExtraMenu.HitboxScale == 1);
    assert(!lite_ui::EspEnabled()); Config.ESPMenu.EspStyle = EspStyle3DSphere; assert(lite_ui::EspEnabled());
    Config = {}; state = {}; view.ready = view.authorized = true; state.tab = lite_ui::Tab::Esp;
    auto& io = ImGui::GetIO(); io.DisplaySize = {1280,720};
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen); Frame(); Frame();
    auto* panel = Find("/players_"); assert(panel);
    const auto row = panel->DC.CursorStartPos + ImVec2(50,50);
    view.interactive=false; view.startup.reveal=.2f; Frame(); Frame();
    Tap(row); assert(!Config.ESPMenu.isPlayerLine);
    for (const auto* list : ImGui::GetDrawData()->CmdLists) for (const auto& vertex : list->VtxBuffer)
        assert((vertex.col>>24)<=51);
    view.interactive=true; view.startup.reveal=1.f;
    Tap(row); assert(Config.ESPMenu.isPlayerLine);
    assert(!Config.Aim.AimSilent && !Config.ExtraMenu.Hit && !Config.ESPMenu.Box);
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse); io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
    state = {}; Frame(); Frame();
    std::puts("Source-derived controls bind the existing Config fields; the visual-work gate includes style-only ESP");
}
void ValidateBudget() {
    auto& io = ImGui::GetIO(); io.DisplaySize = {1280,720}; state = {}; view.ready = true;
    for (int i = 0; i < 40; ++i) Frame();
    const auto previous = allocations;
    const int vertices = ImGui::GetDrawData()->TotalVtxCount;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 600; ++i) { Frame(); assert(ImGui::GetDrawData()->TotalVtxCount == vertices); }
    const double elapsed = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    assert(allocations == previous);
    std::printf("Idle UI: %d vertices, %d command lists, zero ImGui allocations across 600 warm frames (host %.3f ms/frame)\n",
        vertices,ImGui::GetDrawData()->CmdListsCount,elapsed/600.);
}
PhantomTrigger::WorldDiagnostic DiagnosticSample() {
    PhantomTrigger::WorldDiagnostic sample;
    sample.result=PhantomTrigger::Occlusion::Blocked;
    sample.reason=PhantomTrigger::WorldProbeReason::CharacterUnavailable;
    sample.owner=PhantomTrigger::WorldProbeOwner::Target;
    sample.targetDistance=37.125f;sample.worldDistance=17.25f;sample.contactDistance=17.25f;
    sample.material=-1;sample.group=6;sample.contacts=32;sample.sequence=12345;sample.timeMillis=1000;
    sample.fastQueries=0;sample.contactQueries=1;sample.elapsedMicros=2200;
    return sample;
}
ImRect diagnosticTextRect;
ImGuiWindow* DiagnosticFrame(float width,float scale,std::uint64_t now,
                             PhantomTrigger::DecisionStatus status=PhantomTrigger::DecisionStatus::WorldBlocked) {
    ImGui::NewFrame();custom::UpdateTouchScroll();lite_ui::Theme();lite_widgets::scale=scale;
    ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({width+32.f,340.f*scale+32.f});
    ImGui::Begin("Trigger diagnostic fixture",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGuiWindow* panel=nullptr;
    if(lite_widgets::BeginPanel("triggerbot",{width,300.f*scale})) {
        panel=ImGui::GetCurrentWindow();
        bool enabled=true;lite_widgets::Checkbox("Triggerbot",&enabled);
        ImGui::TextUnformatted("Mode: ADS Only");ImGui::TextWrapped("%s",PhantomTrigger::DecisionStatusText(status));
        lite_widgets::Button("Retry setup",{0,0});
        RenderTriggerDiagnostics(status,DiagnosticSample(),now);
        diagnosticTextRect=GImGui->LastItemData.Rect;
        assert(diagnosticTextRect.Min.x>=panel->WorkRect.Min.x-1.f);
        assert(diagnosticTextRect.Max.x<=panel->WorkRect.Max.x+1.f);
    }
    lite_widgets::EndPanel();ImGui::End();ImGui::Render();CheckFrame();
    return panel;
}
void ValidateDiagnostics() {
    auto& io=ImGui::GetIO();io.DisplaySize={1280,720};
    const auto settings=PhantomTrigger::GetSettings();
    std::array<unsigned char,sizeof(Config)> previous;
    std::memcpy(previous.data(),&Config,sizeof(Config));
    for(float width:{180.f,230.f,330.f}) for(float scale:{.85f,1.f,1.35f}) {
        auto* panel=DiagnosticFrame(width,scale,1016);assert(panel);
        panel=DiagnosticFrame(width,scale,1016);
        assert(panel->Flags&ImGuiWindowFlags_NoScrollbar);
        ImGui::SetScrollY(panel,panel->ScrollMax.y+panel->Size.y);
        for(int settle=0;settle<90;++settle) panel=DiagnosticFrame(width,scale,1016);
        if(std::fabs(panel->Scroll.y-panel->ScrollMax.y)>=1.f)
            std::fprintf(stderr,"Diagnostic scroll width %.1f scale %.2f: %.3f / %.3f\n",width,scale,panel->Scroll.y,panel->ScrollMax.y);
        assert(std::fabs(panel->Scroll.y-panel->ScrollMax.y)<1.f);
        assert(diagnosticTextRect.Max.y<=panel->InnerRect.Max.y+1.f);
        DiagnosticFrame(width,scale,3000);DiagnosticFrame(width,scale,999);
        DiagnosticFrame(width,scale,1016,PhantomTrigger::DecisionStatus::Firing);
        DiagnosticFrame(width,scale,3000,PhantomTrigger::DecisionStatus::Firing);
    }
    assert(std::memcmp(previous.data(),&Config,sizeof(Config))==0);
    const auto after=PhantomTrigger::GetSettings();
    assert(after.enabled==settings.enabled && after.mode==settings.mode && after.showDot==settings.showDot);
    state={};Frame();Frame();
    std::puts("World diagnostic UI: bounded wrapping, scroll reachability, fresh/stale/clock states and unchanged feature settings passed");
}
#ifdef LITE_GPU_TEST
#include "lite_blur_gpu_checks.h"
void Screenshot(const std::filesystem::path& path, bool backdrop = false) {
    const auto size = ImGui::GetIO().DisplaySize;
    glBindFramebuffer(GL_FRAMEBUFFER,0); glViewport(0,0,int(size.x),int(size.y));
    if (backdrop) Backdrop(); else { glClearColor(.12f,.13f,.14f,1); glClear(GL_COLOR_BUFFER_BIT); }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    std::vector<unsigned char> pixels(static_cast<std::size_t>(size.x*size.y*4));
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,int(size.x),int(size.y),GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    assert(glGetError() == GL_NO_ERROR);
    std::ofstream output(path,std::ios::binary); output << "P6\n" << int(size.x) << ' ' << int(size.y) << "\n255\n";
    for (int y=int(size.y)-1; y>=0; --y) for (int x=0; x<int(size.x); ++x)
        output.write(reinterpret_cast<const char*>(pixels.data()+(y*int(size.x)+x)*4),3);
    assert(output.good());
}
void CaptureStartup(const std::filesystem::path& output) {
    std::filesystem::create_directories(output);
    auto& io=ImGui::GetIO(); io.DisplaySize={960,540}; io.DeltaTime=1.f/30.f;
    auto* title=view.titleFont;
    state={}; view={}; view.titleFont=title; view.plan="Premium"; view.expiry="2030-01-01 00:00 UTC"; view.fps=60;
    lite_startup::Motion motion;
    Frame(); Frame();
    for (int index=0;index<180;++index) {
        const double time=double(index)/30.;
        view.pending=time>=.9 && time<2.; view.authorized=time>=2.;
        view.startup=motion.Update(time,view.authorized,time>=3.8);
        view.ready=view.startup.showMenu; view.interactive=view.startup.interactive;
        view.preparation=time<2.5 ? 1.f/3.f : time<3.8 ? 2.f/3.f : 1.f;
        view.uptime=static_cast<std::uint64_t>(time);
        PrepareSidebar(); Frame();
        char name[48]; std::snprintf(name,sizeof(name),"frame-%04d.ppm",index);
        Screenshot(output/name,view.ready);
    }
    std::puts("Native login/loading/check/reveal frames captured; server and setup timing use fixtures");
}
void ValidateGL() {
    glActiveTexture(GL_TEXTURE2); glDisable(GL_BLEND); glEnable(GL_CULL_FACE); glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST); glViewport(4,5,510,250); glScissor(7,8,90,100);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    GLint actual[4]; glGetIntegerv(GL_VIEWPORT,actual);
    assert(actual[0] == 4 && actual[1] == 5 && actual[2] == 510 && actual[3] == 250);
    glGetIntegerv(GL_SCISSOR_BOX,actual);
    assert(actual[0] == 7 && actual[1] == 8 && actual[2] == 90 && actual[3] == 100);
    glGetIntegerv(GL_ACTIVE_TEXTURE,actual); assert(actual[0] == GL_TEXTURE2);
    assert(!glIsEnabled(GL_BLEND) && glIsEnabled(GL_CULL_FACE) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_SCISSOR_TEST));
    assert(glGetError() == GL_NO_ERROR);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_CULL_FACE); glDisable(GL_DEPTH_TEST); glActiveTexture(GL_TEXTURE0);
    std::puts("GLES3 direct rendering restores game viewport, scissor, texture unit and enable states");
}
#endif
}
int main(int argc, char** argv) {
    assert(argc >= 2);
    ImGui::SetAllocatorFunctions(Allocate,Free);
#ifdef LITE_GPU_TEST
    const auto getDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    assert(getDisplay);
    const auto display = getDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,nullptr);
    EGLint major,minor; assert(eglInitialize(display,&major,&minor) && eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint attributes[]{EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig config; EGLint count; assert(eglChooseConfig(display,attributes,&config,1,&count) && count);
    const EGLint contextAttributes[]{EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    const auto context = eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttributes);
    const EGLint surfaceAttributes[]{EGL_WIDTH,1920,EGL_HEIGHT,1280,EGL_NONE};
    const auto surface = eglCreatePbufferSurface(display,config,surfaceAttributes);
    assert(context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE && eglMakeCurrent(display,surface,surface,context));
    std::printf("GLES: %s\n",glGetString(GL_VERSION));
#endif
    ImGui::CreateContext();
    auto& io = ImGui::GetIO(); io.IniFilename = io.LogFilename = nullptr;
    io.DisplaySize = {1280,720}; io.DeltaTime = 1.f/60.f;
    const std::filesystem::path root = argv[1];
    const auto body = Read(root/"src/main/jni/Assets/lite-body.ttf"), title = Read(root/"src/main/jni/Assets/lite-title.otf"),
        icons = Read(root/"src/main/jni/Assets/lite-icons.ttf");
    const auto fonts = lite_fonts::Load(*io.Fonts,body.data(),body.size(),title.data(),title.size(),icons.data(),icons.size());
    assert(fonts.body && fonts.title); io.FontDefault = fonts.body; view.titleFont = fonts.title;
    io.Fonts->Build();
    for (ImWchar icon : {0xf013,0xf015,0xf05b,0xf07b,0xf0e7,0xf1fc,0xf500}) assert(fonts.body->FindGlyphNoFallback(icon));
    assert(io.Fonts->TexWidth*io.Fonts->TexHeight*4 <= 2*1024*1024);
    std::printf("Font atlas: %dx%d (%d bytes RGBA), source text fonts with merged sidebar icons\n",io.Fonts->TexWidth,io.Fonts->TexHeight,io.Fonts->TexWidth*io.Fonts->TexHeight*4);
#ifdef LITE_GPU_TEST
    assert(ImGui_ImplOpenGL3_Init("#version 300 es")); ImGui_ImplOpenGL3_NewFrame();
#endif
    view.skins = Skins; view.music = Music; view.expiry = "2030-01-01 00:00 UTC"; view.plan = "Premium";
#ifdef LITE_GPU_TEST
    const char* capture=std::getenv("LITE_CAPTURE_STARTUP");
    if (capture && std::strcmp(capture,"1")==0) {
        assert(argc>=3); CaptureStartup(argv[2]);
    } else
#endif
    {
    ValidateLayouts(); ValidateMotion(); ValidateStartup(); ValidateTouch(); ValidatePolledTouch(); ValidateNestedPan(); ValidateRestore(); ValidateRouting(); ValidateMenuDrag(); ValidateBindings(); ValidateDiagnostics(); ValidateKeyboard(); ValidateMenuSize(); ValidateBudget();
#ifdef LITE_GPU_TEST
    assert(argc >= 3); const std::filesystem::path output = argv[2]; std::filesystem::create_directories(output);
    ValidateBlur();ValidateSidebarReference();
    io.DisplaySize = {1280,720}; state = {}; view.ready = true;
    const char* names[]{"home","esp","aim","trigger","memory","skins","settings"};
    for (int tab=0;tab<7;++tab) { state.tab=static_cast<lite_ui::Tab>(tab); PrepareSidebar(); Frame(); Frame(); Screenshot(output/(std::string(names[tab])+".ppm"),true); }
    DiagnosticFrame(330,1,1016);DiagnosticFrame(330,1,1016);Screenshot(output/"trigger-diagnostic.ppm");
    DiagnosticFrame(180,1,1016);auto* diagnostic=DiagnosticFrame(180,1,1016);
    ImGui::SetScrollY(diagnostic,diagnostic->ScrollMax.y+diagnostic->Size.y);
    for(int settle=0;settle<90;++settle) DiagnosticFrame(180,1,1016);
    Screenshot(output/"trigger-diagnostic-narrow.ppm");
    DiagnosticFrame(330,1,3000);Screenshot(output/"trigger-diagnostic-stale.ppm");
    ValidateGL();
    io.DisplaySize={1280,720};keyboardOpen=true;keyboardText[0]=0;lite_keyboard::state={};
    for(int settle=0;settle<50;++settle) KeyboardFrame();Screenshot(output/"keyboard.ppm",true);
    io.DisplaySize={360,780};lite_keyboard::state={};
    for(int settle=0;settle<50;++settle) KeyboardFrame();Screenshot(output/"keyboard-portrait.ppm",true);
    keyboardOpen=false;KeyboardFrame();
    io.DisplaySize={1280,720};state={};state.tab=lite_ui::Tab::Settings;state.zoom=.85f;
    PrepareSidebar();Frame();Frame();Screenshot(output/"menu-size-small.ppm",true);
    state.zoom=1.f;PrepareSidebar();Frame();Frame();Screenshot(output/"menu-size-full.ppm",true);
    state.minimized=true;PrepareSidebar();Frame();Frame();Screenshot(output/"restore.ppm",true);state.minimized=false;
    state = {}; state.tab = lite_ui::Tab::Memory; io.DisplaySize = {360,780}; PrepareSidebar(); Frame(); Frame(); Screenshot(output/"portrait.ppm",true);
    state = {}; io.DisplaySize = {960,540}; view.ready = view.authorized = false; view.sidebarTexture=allowedTexture=nullptr; Frame(); Frame(); Screenshot(output/"login.ppm");
    view.pending = true; Frame(); Screenshot(output/"authenticating.ppm");
    view.authorized = true; view.pending = false; view.preparation = 2.f/3.f; Frame(); Screenshot(output/"loading.ppm");
#endif
    }
#ifdef LITE_GPU_TEST
    lite_blur::Release(); ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui::DestroyContext();
#ifdef LITE_GPU_TEST
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroySurface(display,surface); eglDestroyContext(display,context); eglTerminate(display);
#endif
    return 0;
}
