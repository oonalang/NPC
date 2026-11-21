#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "ImGui/Call_ImGui.h"
#include "IL2CppSDKGenerator/BasicStructs/Call_BasicStructs.h"
#include "IL2CppSDKGenerator/IL2Cpp/Call_IL2Cpp.h"
#include "Hacks/Hacks.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <functional>
#include <cstring>
#include <cfloat>
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <cmath>
#include <thread>
#include <chrono>

#include "oxorany/source/oxorany.h"
#include "oxorany/source/oxorany.cpp"
#include "oxorany/source/oxorany_include.h"

#include "MainFeatureIncludes.h"
#include "BlueprintEquipment.h"
#include "Trigger/NativeProfile.h"
#include "Trigger/TriggerRuntime.h"
#include "Trigger/TriggerOverlay.h"
#include "CaptureOverlay/HideRecord.h"
#include "Lite/Fonts.h"
#include "Lite/Menu.h"
#include "Lite/NativeBackend.h"
#include "Lite/FrostedSidebar.h"
#include "Lite/TouchInput.h"
#include "Lite/InputRouting.h"
#include "System/UI/ReducedMotion.h"

static void RequestTriggerInitialization();
static void RequestLicensedFeatures();
void Init_Thread2();


#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <sys/system_properties.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "FeedbackService.h"

class _BYTE;
class _BOOL4;
class _BOOL8;
class _WORD;
class _DWORD;
class _QWORD;

#define CREATE_COLOR(r, g, b, a) new float[4] {(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f}
bool ClearDisplay = false;
bool ShowFPS;
bool SnowB = false;
float SnowBsize = 0.0f;
bool MpB = false;
float MpBsize = 0.0f;
bool isSpeedHackEnabled = false;
float speedHackMultiplier = 1.0f;
bool RedWallhackShow = false;
bool isJumpAdjustmentEnabled = false;
float jumpHeightMultiplier = 1.0f;
char logintext[4096];
float menu[4] = {188.0f / 255.0f, 110.0f / 255.0f, 1.0f, 1.0f};

#define _BYTE uint8_t
#define _WORD  uint8_t
#define _DWORD uint64_t
#define _QWORD uint64_t
#define _BOOL4 uint8_t

#include <fstream>
using namespace std;

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
ImFont* SOCIAL = nullptr;
ImFont* Bold = nullptr;
JavaVM* jvm = nullptr;
JavaVM* VM = nullptr;

namespace font {
    ImFont* icomoon_logo = nullptr;
    ImFont* inter_semibold = nullptr;
    ImFont* inter_title = nullptr;
    ImFont* icomoon_page = nullptr;
}


static int g_GlWidth, g_GlHeight;
static bool g_App = false;

struct My_Patches
{
    MemoryPatch A1, grap, NoCrouch, SpeedhackX, SpeedhackX1, NoWingsuit, fpss, frame;
} Patches;

float AVIWA = 119.167f;

bool wallh;
bool active = false;
float AimSmooth = 1.0f;

bool showKeyboard = false;

struct sRegion
{
    uintptr_t start, end;
};

std::chrono::steady_clock::time_point appStartTime = std::chrono::steady_clock::now();

static std::atomic<unsigned> g_LiteSetupProgress{0};
static lite_routing::Router g_LiteRouting;
static std::atomic<std::uint64_t> g_LiteTouchResolution{0};
static lite_ui::State g_LiteState;
static lite_startup::Motion g_LiteStartup;
static bool g_LiteTriedKey = false;
static std::string err;
static char s[256];

static std::atomic<bool> g_LoginFinished{false};
static bool g_LoginPending = false;
static std::string g_LoginResult;
static std::string g_SubmittedKey;
static int (*g_AuthOriginalTouchCount)()=nullptr;
static int AuthTouchCount() {
    const int count=g_AuthOriginalTouchCount?g_AuthOriginalTouchCount():0;
    if(count<=0) return g_LiteRouting.Filter(0,0,{},lite_touch::Phase::Ended);
    const auto packed=g_LiteTouchResolution.load(std::memory_order_acquire);
    const auto width=static_cast<unsigned>(packed>>32),height=static_cast<unsigned>(packed);
    if(!m_unity || !api2 || !width || !height) return count;
    const auto getTouch=reinterpret_cast<Touch(*)(uintptr_t,int)>(Class_Input_GetTouch);
    const auto touch=getTouch(Config.ImGuiMenu.thiz,0);
    const auto phase=touch.m_Phase>=TouchPhase::Began && touch.m_Phase<=TouchPhase::Canceled ?
        static_cast<lite_touch::Phase>(touch.m_Phase):lite_touch::Phase::Canceled;
    return g_LiteRouting.Filter(count,touch.m_FingerId,{touch.m_Position.x/width,1.f-touch.m_Position.y/height},phase);
}

std::vector<sRegion> trapRegions;
uintptr_t address = 0;
std::string md5(std::string s);
uintptr_t g_il2cpp;
EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
static void PollLiteTouch(ImGuiIO& io) {
    static lite_touch::Input input;
    if(!m_unity || !api1 || !api2 || !api4 || !api5) { input.Update(io,nullptr,{}); return; }
    const int width=get_width(),height=get_height();
    if(width<=0 || height<=0) { input.Update(io,nullptr,{}); return; }
    g_LiteTouchResolution.store((std::uint64_t(static_cast<unsigned>(width))<<32)|static_cast<unsigned>(height),std::memory_order_release);
    const auto count = g_AuthOriginalTouchCount ? g_AuthOriginalTouchCount : reinterpret_cast<int(*)()>(Class_Input_get_touchCount);
    const int touches=count?count():0;
    if (touches > 0) {
        const auto getTouch = reinterpret_cast<Touch(*)(uintptr_t,int)>(Class_Input_GetTouch);
        if(!getTouch) { input.Update(io,nullptr,{}); return; }
        const auto touch = getTouch(Config.ImGuiMenu.thiz,0);
        const auto phase=touch.m_Phase>=TouchPhase::Began && touch.m_Phase<=TouchPhase::Canceled ?
            static_cast<lite_touch::Phase>(touch.m_Phase) : lite_touch::Phase::Canceled;
        const bool captured=g_LiteRouting.Filter(touches,touch.m_FingerId,
            {touch.m_Position.x/width,1.f-touch.m_Position.y/height},phase)==0;
        const lite_touch::Sample sample{touch.m_FingerId,{touch.m_Position.x,touch.m_Position.y},phase};
        if(!captured && phase==lite_touch::Phase::Began && GImGui->OpenPopupStack.Size)
            ImGui::ClosePopupToLevel(0,true);
        if(captured) input.Update(io,&sample,{static_cast<float>(width),static_cast<float>(height)});
        else input.Ignore(io);
    } else {
        g_LiteRouting.Filter(0,0,{},lite_touch::Phase::Ended);
        input.Update(io,nullptr,{});
    }
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy,surface,EGL_WIDTH,&g_GlWidth);
    eglQuerySurface(dpy,surface,EGL_HEIGHT,&g_GlHeight);
    if(g_GlWidth<=0 || g_GlHeight<=0) return old_eglSwapBuffers(dpy,surface);
    if(!g_App) {
        ImGui::CreateContext();
        auto& io=ImGui::GetIO();
        io.IniFilename=io.LogFilename=nullptr;
        io.ConfigFlags|=ImGuiConfigFlags_NoMouseCursorChange;
        io.MouseDragThreshold=7.f;
        lite_touch::Input::Configure(io);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        const auto fonts=lite_fonts::Load(*io.Fonts,lite_body_start,lite_body_end-lite_body_start,
            lite_title_start,lite_title_end-lite_title_start,lite_icons_start,lite_icons_end-lite_icons_start);
        font::inter_semibold=fonts.body?fonts.body:io.Fonts->AddFontDefault();
        font::inter_title=fonts.title?fonts.title:font::inter_semibold;
        font::icomoon_page=font::inter_semibold;
        F50=F107=font::inter_semibold;
        io.FontDefault=font::inter_semibold;
        io.Fonts->Build();
        Config={};
        Config.ExtraMenu.ClearDisplay = true;
        Config.sColorsESPPLAYER.LinePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.BoxPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.NamePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.DistancePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.HealthPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.SkeletonPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPBOT.LineBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.BoxBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.NameBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.HealthBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.DistanceBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.SkeletonBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPOTHERS.PovOTHERS = CREATE_COLOR(225, 0, 0, 180);

        Config.ExtraMenu.NoGravityScale = 1.0f;
        Config.Aim.AimAssistSize = 0.0f;
        Config.Aim.Cross = 45.0f;
        Config.Aim.Target = EAimTarget::Heads;
        Config.Aim.Trigger = EAimTrigger::None;
        Config.Aim.By = EAim::Distance;

        Config.Bline = 2.0f;
        Config.Pline = 2.0f;


        g_App=true;
    }
    auto& io=ImGui::GetIO();
    screenWidth=static_cast<float>(g_GlWidth);
    screenHeight=static_cast<float>(g_GlHeight);
    io.DisplaySize={screenWidth,screenHeight};
    const auto now=std::chrono::steady_clock::now();
    static auto previous=now;
    io.DeltaTime=ImClamp(std::chrono::duration<float>(now-previous).count(),.0001f,.1f);
    previous=now;
    PollLiteTouch(io);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    custom::UpdateTouchScroll();
    slide_fatigue::Publish(Config.ExtraMenu.NoSlideFatigue);
    pickup::Publish(Config.ExtraMenu.FastPickup,Config.ExtraMenu.PickupRange);
    hitbox::Publish(Config.ExtraMenu.Hit,Config.ExtraMenu.HitboxScale);
    frame_rate::Publish(frame_rate::Configured(Config.Aim.FpsLevel,Config.Aim.FpsLevel1,
        Config.Aim.FpsLevelUltra,Config.Aim.FpsLevelUltra1));
    static auto lastProbe=now;
    if(Config.ExtraMenu.Hit&&hitbox::NeedsProbe()&&now-lastProbe>=std::chrono::seconds(1)) {
        lastProbe=now;Init_Thread2();
    }
    if(!g_LiteTriedKey&&VM) {
        g_LiteTriedKey=true;
        if(LoadTextFromFile()&&logintext[0]) {
            std::snprintf(s,sizeof(s),"%s",logintext);
        }
    }
    if(g_LoginPending&&g_LoginFinished.load(std::memory_order_acquire)) {
        g_LoginPending=false;
        g_LoginFinished.store(false,std::memory_order_relaxed);
        err=g_LoginResult;
        if(err=="OK"&&license::Allows(license::Capability::Menu)) {
            usedKey=g_SubmittedKey;
            std::snprintf(logintext,sizeof(logintext),"%s",g_SubmittedKey.c_str());
            SaveLoginTextToFile(logintext);
            err.clear();
            RequestLicensedFeatures();
        } else if(err=="OK"||err.empty()) err="Authentication failed.";
    }
    const bool authorized=license::Allows(license::Capability::Menu);
    const bool features=license::Allows(license::Capability::Features);
    if(authorized&&features&&g_LiteSetupProgress.load(std::memory_order_acquire)==0) RequestLicensedFeatures();
    const auto stage=g_LiteSetupProgress.load(std::memory_order_acquire);
    static bool reducedLoaded = false, reducedMotion = false;
    if (!reducedLoaded && VM) { reducedMotion=startup_ui::ReducedMotionEnabled(VM); reducedLoaded=true; }
    const auto presentation=g_LiteStartup.Update(std::chrono::duration<double>(now-appStartTime).count(),
        authorized,authorized&&(!features||stage>=3),reducedMotion);
    const bool ready=presentation.showMenu;
    static bool wasAuthorized = false;
    if (wasAuthorized && !authorized) {
        g_LiteState.minimized = false;
        lite_ui::keyboard = showKeyboard = false;
        ImGui::ClearActiveID();
        err = license::LastError();
        if (err.empty()) err = "Session expired. Authenticate again.";
    }
    wasAuthorized = authorized;
    if (ready && features && lite_ui::EspEnabled())
        DrawESP(ImGui::GetBackgroundDrawList(),g_GlWidth,g_GlHeight,get_dpi());
    else { ApplySpeedhack(); ApplyNoGravityRuntime(); }
    if (ready) PhantomTrigger::DrawTriggerDot(ImGui::GetBackgroundDrawList(),screenWidth,screenHeight);
    if(features&&Config.ExtraMenu.WallHack) Patches.A1.Modify();else Patches.A1.Restore();
    if(features&&Config.ExtraMenu.NoCrouch) Patches.NoCrouch.Modify();else Patches.NoCrouch.Restore();
    static auto sampledAt=now;
    static unsigned frames=0;
    static float fps=0;
    ++frames;
    const float seconds=std::chrono::duration<float>(now-sampledAt).count();
    if(seconds>=1.f) {fps=frames/seconds;frames=0;sampledAt=now;}
    char expiry[48]="No active license";
    const auto until=static_cast<time_t>(license::LicenseUntil());
    if(until>0) {std::tm date{};if(gmtime_r(&until,&date)) std::strftime(expiry,sizeof(expiry),"%Y-%m-%d %H:%M UTC",&date);}
    lite_ui::View view;
    view.authorized=authorized;view.pending=g_LoginPending;view.ready=ready;
    view.interactive=presentation.interactive;view.startup=presentation;
    lite_ui::PreparePlacement(g_LiteState,view);
    if (ready && !g_LiteState.minimized) {
        const auto layout=lite_ui::Fit(io.DisplaySize);
        const auto position=g_LiteState.placement.Position(io.DisplaySize,layout.size);
        const auto texture=lite_blur::Capture({position.x,position.y,layout.sidebar,layout.size.y},
            g_GlWidth,g_GlHeight,std::chrono::duration<double>(now-appStartTime).count());
        view.sidebarTexture=reinterpret_cast<ImTextureID>(static_cast<std::uintptr_t>(texture.id));
        view.sidebarUv0={texture.u0,texture.v1};view.sidebarUv1={texture.u1,texture.v0};
    }
    view.preparation=stage/3.f;view.fps=fps;
    view.uptime=static_cast<std::uint64_t>(std::chrono::duration<double>(now-appStartTime).count());
    view.error=err.c_str();view.expiry=expiry;
    view.plan=license::Allows(license::Capability::Premium)?"Premium":"Free";
    view.titleFont=font::inter_title;view.skins=lite_backend::Skins;view.music=lite_backend::Music;
    const auto actions=lite_ui::Draw(g_LiteState,view,s,sizeof(s));
    if(actions.paste) {const auto value=getClipboard();std::snprintf(s,sizeof(s),"%s",value.c_str());err.clear();}
    if(actions.loadKey) {
        if(LoadTextFromFile()&&logintext[0]) {std::snprintf(s,sizeof(s),"%s",logintext);err.clear();}
        else err="No saved key found.";
    }
    if(actions.submit&&!g_LoginPending) {
        if(!s[0]) err="Enter your access key.";
        else {
            g_SubmittedKey=s;g_LoginPending=true;err.clear();
            lite_ui::keyboard=false;showKeyboard=false;
            ImGui::ClearActiveID();
            std::thread([](std::string key) {
                g_LoginResult=Login(key.c_str());
                g_LoginFinished.store(true,std::memory_order_release);
            },g_SubmittedKey).detach();
        }
    }
    if(actions.retryTrigger) RequestTriggerInitialization();
    if(actions.saveConfig||actions.loadConfig) {
        const std::string name=g_LiteState.configName;
        const bool valid=!name.empty()&&name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_- ")==std::string::npos;
        if(!valid) err="Use a configuration name containing letters, numbers, spaces, - or _.";
        else if(actions.loadConfig) err=LoadConfiguration(name)?"Configuration loaded.":"Unable to load configuration.";
        else err=SaveConfiguration(name)?"Configuration saved.":"Unable to save configuration.";
    }
    g_LiteRouting.Publish(lite_routing::VisibleRegions());
    static bool musicWasEnabled = false;
    if (musicWasEnabled && !g_LiteState.music) music_player::Stop();
    musicWasEnabled = g_LiteState.music;
    BlueprintEquipment::ApplyPending();
    UpdateCamoOverride();
    ApplyWorldVisualsRuntime();
    InstallBRClassEspConfigHook();
    ImGui::Render();
    if (g_LiteState.hideRecord && !ZEL_Render(ImGui::GetDrawData(),dpy,surface,eglGetCurrentContext(),old_eglSwapBuffers)) {
        g_LiteState.hideRecord=false;
        err="Hide Record is unavailable on this device.";
    }
    if (!g_LiteState.hideRecord) {
        ZEL_Shutdown();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    if(feedback::IsSending()) feedback::PumpAfterRender();
    return old_eglSwapBuffers(dpy,surface);
}
size_t hook_strlen(const char *thread)
{
    if (strstr(thread, "eglSwapBuffers"))
    {
    }
    return strlen(thread);
}

void Init_Thread2()
{
    static std::atomic<bool> running{false};
    if(!license::Allows(license::Capability::Features)||!m_unity||running.exchange(true)) return;
    std::thread([base=static_cast<std::uintptr_t>(m_unity)] {
        const auto install=[](void* address,void* replacement,void** original){hook(address,replacement,original);};
        for(unsigned attempt=0;attempt<80;++attempt) {
            frame_rate::Install(base,install);
            slide_fatigue::Install(base,install);
            pickup::Install(base,install);
            hitbox::Install(base,install);
            if(frame_rate::available.load()&&slide_fatigue::available.load()&&pickup::available.load()&&hitbox::available.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        running=false;
    }).detach();
}

static void RequestTriggerInitialization()
{
    static std::atomic<bool> running{false};
    if (!license::Allows(license::Capability::Features) || !m_unity || running.exchange(true, std::memory_order_acq_rel)) return;
    std::thread([base = static_cast<std::uintptr_t>(m_unity)] {
        for (unsigned attempt = 0; attempt < 40; ++attempt) {
            if (PhantomTrigger::InitializeNativeTrigger(base)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        running.store(false, std::memory_order_release);
    }).detach();
}

void Init_SwapHook()
{
}

static void RequestLicensedFeatures() {
    static std::atomic<bool> started{false};
    if(!license::Allows(license::Capability::Features)||started.exchange(true)) return;
    g_LiteSetupProgress.store(1,std::memory_order_release);
    std::thread([] {
        DobbyHook((void*)getAbsoluteAddress("libunity.so",0xD5319D4),
            reinterpret_cast<void*>(&license::Guard<WeaponFireComponent_Instant_CreateBulletLine,&oWeaponFireComponent_Instant_CreateBulletLine>::Invoke),
            reinterpret_cast<void**>(&oWeaponFireComponent_Instant_CreateBulletLine));
        DobbyHook((void*)getAbsoluteAddress("libunity.so",0xD53DA64),
            reinterpret_cast<void*>(&license::Guard<WeaponFireComponent_Instant_CreateBulletProjectile,&oWeaponFireComponent_Instant_CreateBulletProjectile>::Invoke),
            reinterpret_cast<void**>(&oWeaponFireComponent_Instant_CreateBulletProjectile));
        g_LiteSetupProgress.store(2,std::memory_order_release);
        InstallBRClassEspConfigHook();InitializeAllHooks();BlueprintEquipment::Install();
        Init_Thread2();RequestTriggerInitialization();
        g_LiteSetupProgress.store(3,std::memory_order_release);
    }).detach();
}

void Init_Thread()
{
    while (!m_unity)
    {
        m_unity = Tools::GetBaseAddress("libunity.so");
        sleep(1);
    }
    LOGI("libunity.so: %p", m_unity);
    UpdateAllOffset();    

	
    Patches.A1 = MemoryPatch::createWithHex("libunity.so", 0x55655CC, "1F 20 03 D5 E0 03 13 AA");
    Patches.NoCrouch = MemoryPatch::createWithHex("libunity.so",0x54A83A4,"00 00 80 D2 C0 03 5F D6");
    Patches.NoWingsuit = MemoryPatch::createWithHex("libunity.so",0x557C6E0,"00 00 80 D2 C0 03 5F D6");
    
    if(!g_AuthOriginalTouchCount && DobbyHook(reinterpret_cast<void*>(Class_Input_get_touchCount),
        reinterpret_cast<void*>(&AuthTouchCount),reinterpret_cast<void**>(&g_AuthOriginalTouchCount))!=0)
        LOGE("Authentication touch guard could not be installed");
MemoryPatch::createWithHex("libanogs.so", 0x202680, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x204218, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x35140C, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x37B5A8, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x3893D8, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x39AE94, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x44A714, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x44BC90, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x455A80, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x48CF20, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x497244, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x501210, "00 00 80 D2 C0 03 5F D6").Modify();
    auto swapBuffers = ((uintptr_t)DobbySymbolResolver(OBFUSCATE("libunity.so"), OBFUSCATE("eglSwapBuffers")));
    KittyMemory::ProtectAddr((void *)swapBuffers, sizeof(swapBuffers), PROT_READ | PROT_WRITE | PROT_EXEC);
    xhook_enable_debug(0);
    xhook_register(OBFUSCATE(".*libunity\\.so$"), OBFUSCATE("eglSwapBuffers"), (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    if (xhook_refresh(0) == 0) {
        xhook_clear();
                                
    }
}

__attribute__((constructor))
void native_Init(JNIEnv *env, jclass clazz, jobject mContext) {
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK) return JNI_ERR;
    jvm = vm;
    VM = vm;
    ZEL_SetVM(vm);
    std::thread(Init_Thread).detach();
    std::thread(Init_Thread2).detach();
    std::thread(Skins_Thread).detach();
    return JNI_VERSION_1_6;
}
