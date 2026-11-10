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
#include <thread>

#include "oxorany/source/oxorany.h"
#include "oxorany/source/oxorany.cpp"
#include "oxorany/source/oxorany_include.h"

#include "MainFeatureIncludes.h"
#include "Trigger/NativeProfile.h"
#include "Trigger/TriggerRuntime.h"
#include "Trigger/TriggerOverlay.h"
#include "CaptureOverlay/HideRecord.h"


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

static auto lastScreenshotTime = std::chrono::steady_clock::now();
constexpr std::chrono::seconds screenshotInterval(180);

class _BYTE;
class _BOOL4;
class _BOOL8;
class _WORD;
class _DWORD;
class _QWORD;

#define CREATE_COLOR(r, g, b, a) new float[4] {(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f}
bool g_clearMousePos = false;
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

float g_LastLogoOpacity = 1.0f;
float g_LastLogoSize = 1.0f;
int g_LogoHideDelayFrames = 0;
int g_LogoHideDelay = 40;

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
static bool g_RuntimeClearDisplayInit = false;

struct ClearDisplayDefaultInit {
    ClearDisplayDefaultInit()
    {
        Config.ExtraMenu.ClearDisplay = true;       
    }
} g_ClearDisplayDefaultInit;

struct sRegion
{
    uintptr_t start, end;
};

std::chrono::steady_clock::time_point appStartTime = std::chrono::steady_clock::now();

static float veh_min = 0.1f;
static float veh_max = 5.0f;
static bool windowCollapsed = false;
static double collapseBarLastActiveTime = 0.0;
static float collapseBarOpacityAnim = 1.0f;
static float collapseBarPressAnim = 0.0f;
static float collapseBarEnterAnim = 1.0f;
static float collapseBarRestoreAnim = 1.0f;
static bool collapseBarRestoreActive = false;
static bool collapseBarWasCollapsed = false;
static float uncollapseOpenAnim = 1.0f;
static bool dark = true;
static float tabAlpha = 0.0f;
static float tabAdd = 0.0f;
static int page = 1;
static int activeTab = 1;
bool g_LogoPreviewMode = false;
static bool isLogin = false;
static std::string err;
static std::string storedKey = "";
static char s[256];
static bool g_LoginTextLoaded = false;
// The worker publishes its result before the render thread consumes it.
static std::atomic<bool> g_LoginFinished{false};
static bool g_LoginPending = false;
static std::string g_LoginResult;
static std::string g_SubmittedKey;

std::vector<sRegion> trapRegions;
uintptr_t address = 0;
std::string md5(std::string s);
uintptr_t g_il2cpp;
static bool isMenuVisible = true;
static bool hideRecordEnabled = false;
static bool hideRecordUnavailable = false;

int TABG = 1;

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    eglQuerySurface(dpy, surface, EGL_WIDTH, &g_GlWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_GlHeight);

    if (!g_App)
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        io.LogFilename = NULL;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.MouseDoubleClickTime = 0.3f;
        io.MouseDragThreshold = 2.f;
        ImGui_ImplOpenGL3_Init("#version 300 es");

        ImFontConfig icomoon_logo_config;
        icomoon_logo_config.MergeMode = false;
        icomoon_logo_config.PixelSnapH = true;
        icomoon_logo_config.FontDataOwnedByAtlas = false;
        font::icomoon_logo = io.Fonts->AddFontFromMemoryTTF(
            (void*)icomoon_page,
            sizeof(icomoon_page),
            20.f,
            &icomoon_logo_config
        );

        ImFontConfig inter_config;
        inter_config.MergeMode = false;
        inter_config.PixelSnapH = true;
        inter_config.FontDataOwnedByAtlas = false;
        font::inter_semibold = io.Fonts->AddFontFromMemoryTTF(
            (void*)inter_semibold,
            sizeof(inter_semibold),
            20.f,
            &inter_config
        );

        ImFontConfig page_config;
        page_config.MergeMode = false;
        page_config.PixelSnapH = true;
        page_config.FontDataOwnedByAtlas = false;
        font::icomoon_page = io.Fonts->AddFontFromMemoryTTF(
            (void*)icomoon_page,
            sizeof(icomoon_page),
            18.f,
            &page_config
        );

        static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.OversampleH = 2.5f;
        iconsConfig.OversampleV = 2.5f;
        iconsConfig.FontDataOwnedByAtlas = false;
        F107 = io.Fonts->AddFontFromMemoryCompressedTTF(
            (void*)font_awesome_data1,
            (int)font_awesome_size1,
            25.0f,
            &iconsConfig,
            icons_ranges
        );
        F50 = io.Fonts->AddFontFromMemoryTTF((void *)F50_data, F50_size, 30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        if (!F107) {
            F107 = font::inter_semibold;
        }
        if (font::inter_semibold) {
            io.FontDefault = font::inter_semibold;
        }
        io.Fonts->Build();
        
        ImGui_ImplOpenGL3_CreateFontsTexture();

        memset(&Config, 0, sizeof(sConfig));

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

        g_App = true;
    }

    ImGuiIO *io = &ImGui::GetIO();
    screenWidth = (float)g_GlWidth;
    screenHeight = (float)g_GlHeight;
    io->DisplaySize = ImVec2((float)g_GlWidth, (float)g_GlHeight);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImDrawList *draw = ImGui::GetBackgroundDrawList();

    if (isLogin && expiryTimestamp > 0 && time(nullptr) >= expiryTimestamp) {
        isLogin = false;
        err = "License expired";
        auto settings = PhantomTrigger::GetSettings();
        settings.enabled = false;
        PhantomTrigger::SetSettings(settings);
    }
    if (isLogin)
        DrawESP(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight, get_dpi());
    PhantomTrigger::DrawTriggerDot(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight);
    floating_info::Render(draw, screenWidth, screenHeight);
      


    if (windowCollapsed)
    {
        if (!collapseBarWasCollapsed) {
            collapseBarEnterAnim = 0.0f;
            collapseBarRestoreAnim = 1.0f;
            collapseBarRestoreActive = false;
            collapseBarOpacityAnim = 1.0f;
            collapseBarLastActiveTime = ImGui::GetTime();
        }
        collapseBarWasCollapsed = true;
        const float dt = ImGui::GetIO().DeltaTime;
        collapseBarEnterAnim = ImClamp(collapseBarEnterAnim + dt * 6.0f, 0.0f, 1.0f);
        if (collapseBarRestoreActive)
            collapseBarRestoreAnim = ImClamp(collapseBarRestoreAnim + dt * 8.0f, 0.0f, 1.0f);
        const float enterEase = collapseBarEnterAnim * collapseBarEnterAnim * (3.0f - 2.0f * collapseBarEnterAnim);
        const float restoreEase = collapseBarRestoreAnim * collapseBarRestoreAnim * (3.0f - 2.0f * collapseBarRestoreAnim);
        const float collapsedAlphaSetting = ImClamp(GetLogoOpacity(), 0.0f, 1.0f);
        const float collapsedScaleSetting = ImClamp(GetLogoSizeMultiplier(), 0.1f, 2.0f);
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float baseLineW = 120.0f * c::scale * collapsedScaleSetting;
        const float line_w = collapseBarRestoreActive ? ImLerp(baseLineW * 1.28f, baseLineW, restoreEase) : ImLerp(baseLineW * 1.28f, baseLineW, enterEase);
        const float line_h = collapseBarRestoreActive ? ImLerp(10.0f, 6.0f, restoreEase) : ImLerp(6.0f, 10.0f, enterEase);
        const float lineH = line_h * c::scale * collapsedScaleSetting;
        const float click_w = line_w + 72.0f * c::scale;
        const float click_h = ImMax(28.0f * c::scale, lineH + 20.0f * c::scale);
        const float startY = display.y - 10.0f * c::scale - lineH;
        const float endY = display.y - 10.0f * c::scale - lineH;
        const float handleY = collapseBarRestoreActive ? ImLerp(startY, endY, restoreEase) : ImLerp(endY, startY, enterEase);
        const ImVec2 handlePos((display.x - line_w) * 0.5f, handleY);
        const ImVec2 clickPos(handlePos.x - (click_w - line_w) * 0.5f, handlePos.y - (click_h - lineH) * 0.5f);

        ImGui::SetNextWindowPos(clickPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(click_w, click_h), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        if (ImGui::Begin("##indicator", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings))
        {
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##restoreclick", ImVec2(click_w, click_h));

            bool barHeld = ImGui::IsItemActive();
            bool barHovered = ImGui::IsItemHovered();
            if (barHovered || barHeld)
                collapseBarLastActiveTime = ImGui::GetTime();
            const bool idle = (ImGui::GetTime() - collapseBarLastActiveTime) > 5.0;
            const float targetOpacity = idle ? 0.20f : 1.0f;
            collapseBarOpacityAnim = ImLerp(collapseBarOpacityAnim, targetOpacity, ImGui::GetIO().DeltaTime * 9.0f);
            collapseBarPressAnim = ImLerp(collapseBarPressAnim, barHeld ? 1.0f : 0.0f, ImGui::GetIO().DeltaTime * 22.0f);

            if (ImGui::IsItemClicked())
            {
                collapseBarRestoreActive = true;
                collapseBarRestoreAnim = 0.0f;
                collapseBarLastActiveTime = ImGui::GetTime();
            }

            float drawAlpha = ImClamp(collapseBarOpacityAnim * collapsedAlphaSetting, 0.0f, 1.0f);
            ImDrawList* indicatorDraw = ImGui::GetWindowDrawList();
            const ImVec2 lineMin(windowPos.x + (click_w - line_w) * 0.5f, windowPos.y + (click_h - lineH) * 0.5f);
            const ImVec2 lineMax(lineMin.x + line_w, lineMin.y + lineH);
            const float press = collapseBarPressAnim;
            indicatorDraw->AddRectFilled(
                lineMin,
                lineMax,
                IM_COL32(255, 255, 255, (int)((225.0f + 30.0f * press) * drawAlpha)),
                lineH * 0.5f
            );

            if (collapseBarRestoreActive && collapseBarRestoreAnim >= 1.0f)
            {
                windowCollapsed = false;
                isMenuVisible = true;
                uncollapseOpenAnim = 0.0f;
                collapseBarRestoreActive = false;
                collapseBarEnterAnim = 1.0f;
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }
    else
    {
        collapseBarWasCollapsed = false;
        collapseBarEnterAnim = 1.0f;
    }

    if (isMenuVisible && !windowCollapsed)
    {
        if (!g_RuntimeClearDisplayInit) {
            Config.ExtraMenu.ClearDisplay = true;
            g_RuntimeClearDisplayInit = true;
        }

        if (!g_LoginTextLoaded && VM != nullptr)
        {
            if (LoadTextFromFile() && logintext[0] != '\0')
            {
                strncpy(s, logintext, sizeof(s) - 1);
                s[sizeof(s) - 1] = '\0';
                g_LoginTextLoaded = true;
            }
        }

        runtime_preview_menu::EnsureTexturesLoaded();
        main_runtime_theme::ApplyThemeState();

        ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (!isLogin && !ui_loading::IsActive())
        {
            if (g_LoginPending && g_LoginFinished.load(std::memory_order_acquire)) {
                g_LoginPending = false;
                g_LoginFinished.store(false, std::memory_order_relaxed);
                err = g_LoginResult;
                if (err == "OK" && bValid) {
                    showKeyboard = false;
                    strncpy(logintext, g_SubmittedKey.c_str(), sizeof(logintext) - 1);
                    logintext[sizeof(logintext) - 1] = '\0';
                    SaveLoginTextToFile(logintext);
                    g_LoginTextLoaded = true;
                    err.clear();
                    ui_loading::Start(7.0f);
                } else if (err == "OK") {
                    err = "License rejected";
                }
            }
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(630, 400), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);
            
            if (ImGui::Begin(OBFUSCATE("Login Menu"), nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                const ImVec2 pos = ImGui::GetWindowPos();
                ImDrawList* draw = ImGui::GetWindowDrawList();
                const ImVec2 login_size = ImVec2(540.0f, 330.0f);

                // Compact authentication panel inspired by the supplied reference:
                // dark translucent card, thin border, key icon/title, key field,
                // two utility buttons, primary authenticate button, and error line.
                draw->AddRectFilled(
                    pos,
                    pos + login_size,
                    IM_COL32(0, 0, 0, 255),
                    12.0f
                );
                draw->AddRect(
                    pos,
                    pos + login_size,
                    IM_COL32(220, 40, 55, 220),
                    12.0f,
                    0,
                    1.0f
                );

                // Subtle top highlight.
                draw->AddRectFilled(
                    pos + ImVec2(1.0f, 1.0f),
                    pos + ImVec2(login_size.x - 1.0f, 5.0f),
                    IM_COL32(240, 50, 65, 180),
                    10.0f
                );

                // Header icon and title.
                const ImVec2 iconCenter = pos + ImVec2(31.0f, 31.0f);
                draw->AddCircleFilled(iconCenter, 13.0f, IM_COL32(220, 40, 55, 50), 24);
                draw->AddCircle(iconCenter, 13.0f, IM_COL32(240, 55, 70, 210), 24, 1.0f);

                if (F50)
                    draw->AddText(F50, 15.0f,
                                  iconCenter + ImVec2(-5.0f, -8.0f),
                                  IM_COL32(235, 240, 242, 255), "N");

                const char* titleText = "NPC EXECUTIVE AUTH";
                draw->AddText(
                    F50 ? F50 : ImGui::GetFont(),
                    17.0f,
                    pos + ImVec2(53.0f, 20.0f),
                    IM_COL32(226, 236, 246, 255),
                    titleText
                );

                draw->AddLine(
                    pos + ImVec2(53.0f, 46.0f),
                    pos + ImVec2(login_size.x - 18.0f, 46.0f),
                    IM_COL32(92, 70, 24, 180),
                    1.0f
                );

                // Key input.
                const float inputX = 24.0f;
                const float inputY = 72.0f;
                const float inputW = login_size.x - 48.0f;
                const float inputH = 48.0f;

                ImGui::SetCursorPos(ImVec2(inputX, inputY));
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(40, 10, 12, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(60, 15, 18, 255));
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(220, 40, 55, 235));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(224, 234, 244, 255));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, IM_COL32(122, 132, 148, 220));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                if (F50) ImGui::PushFont(F50);
                ImGui::AstralInput("##key_login", s, sizeof(s), ImVec2(inputW, inputH));
                if (F50) ImGui::PopFont();

                const bool loginInputClicked = ImGui::IsItemClicked();
                const bool loginInputActive = ImGui::IsItemActive();
                const bool loginInputHovered = ImGui::IsItemHovered();

                if (loginInputClicked || loginInputActive)
                    showKeyboard = true;

                if (showKeyboard && !loginInputActive && !loginInputHovered && ImGui::IsMouseClicked(0))
                {
                    ImGuiIO& io = ImGui::GetIO();
                    const float screenHeight = io.DisplaySize.y;
                    const float keyboardHeight = 300.0f;
                    if (ImGui::GetMousePos().y < screenHeight - keyboardHeight)
                        showKeyboard = false;
                }

                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(6);

                // Utility buttons: PASTE + LOAD CACHED.
                const float utilityY = 132.0f;
                const float utilityGap = 12.0f;
                const float utilityW = (login_size.x - 48.0f - utilityGap) * 0.5f;
                const float utilityH = 42.0f;

                auto drawUtilityButton = [&](const char* label, float x) -> bool
                {
                    ImGui::SetCursorPos(ImVec2(x, utilityY));
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(40, 10, 12, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(60, 15, 18, 255));
                    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(220, 40, 55, 235));
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(207, 218, 231, 255));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                    if (F50) ImGui::PushFont(F50);
                    bool clicked = ImGui::Button(label, ImVec2(utilityW, utilityH));
                    if (F50) ImGui::PopFont();

                    ImGui::PopStyleVar(2);
                    ImGui::PopStyleColor(5);
                    return clicked;
                };


                if (drawUtilityButton("LOAD", 24.0f + utilityW + utilityGap))
                {
                    if (LoadTextFromFile() && logintext[0] != '\0')
                    {
                        strncpy(s, logintext, sizeof(s) - 1);
                        s[sizeof(s) - 1] = '\0';
                        g_LoginTextLoaded = true;
                        err.clear();
                    }
                    else
                    {
                        err = "No cached key found.";
                    }
                }

                // Primary authentication button.
                const float authX = 24.0f;
                const float authY = 190.0f;
                const float authW = login_size.x - 48.0f;
                const float authH = 46.0f;

                ImGui::SetCursorPos(ImVec2(authX, authY));
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(40, 10, 12, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(60, 15, 18, 255));
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(220, 40, 55, 235));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(225, 236, 246, 255));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                if (F50) ImGui::PushFont(F50);
                if (ImGui::Button(g_LoginPending ? "CHECKING..." : "AUTHENTICATE", ImVec2(authW, authH)) && !g_LoginPending)
                {
                    if (s[0] == '\0')
                    {
                        err = "Enter a key first.";
                    }
                    else
                    {
                        g_SubmittedKey = s;
                        g_LoginPending = true;
                        err.clear();
                        std::thread([](std::string key) {
                            g_LoginResult = Login(key.c_str());
                            g_LoginFinished.store(true, std::memory_order_release);
                        }, g_SubmittedKey).detach();
                    }
                }
                if (F50) ImGui::PopFont();
                                if (drawUtilityButton("PASTE", 24.0f))
                {
                    auto key = getClipboard();
                    strncpy(s, key.c_str(), sizeof(s) - 1);
                    s[sizeof(s) - 1] = '\0';
                    err.clear();
                }


                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(5);

                // Error/status line.
                if (!err.empty() && err != "OK")
                {
                    draw->AddText(
                        pos + ImVec2(24.0f, 255.0f),
                        IM_COL32(255, 105, 115, 255),
                        ("Error: " + err).c_str()
                    );
                }
                else
                {
                    draw->AddText(
                        pos + ImVec2(24.0f, 255.0f),
                        IM_COL32(118, 132, 150, 210),
                        "Use your authorized access key to continue."
                    );
                }

                draw->AddLine(
                    pos + ImVec2(24.0f, 283.0f),
                    pos + ImVec2(login_size.x - 24.0f, 283.0f),
                    IM_COL32(82, 38, 44, 180),
                    1.0f
                );

                draw->AddText(
                    pos + ImVec2(24.0f, 296.0f),
                    IM_COL32(102, 116, 134, 190),
                    "Secure access  •  Local cache"
                );

                if (showKeyboard)
                    RenderVirtualKeyboard("##VirtualKeyboardLogin", s, sizeof(s), &showKeyboard);
            }
            ImGui::End();
        }
        else if (!isLogin && ui_loading::IsActive())
        {
            if (ui_loading::RenderWindow((ImTextureID)(intptr_t)runtime_preview_menu::g_menuBackground.id))
            {
                isLogin = true;
            }
        }
        else
        {
            uncollapseOpenAnim = ImClamp(uncollapseOpenAnim + ImGui::GetIO().DeltaTime * 5.0f, 0.0f, 1.0f);
            float openEase = uncollapseOpenAnim * uncollapseOpenAnim * (3.0f - 2.0f * uncollapseOpenAnim);
            float openAlpha = 0.2f + 0.8f * openEase;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, openAlpha);

            ImVec2 mainWindowSize = ImVec2(1220.f, 540.f);
            mainWindowSize.x = ImMin(mainWindowSize.x, displaySize.x);
            mainWindowSize.y = ImMin(mainWindowSize.y, displaySize.y);
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGui::Begin(
                "@Npc00000000000000000000000000000",
                nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground
            );
            {
                runtime_preview_menu::StateRefs runtimeState{
                    dark,
                    tabAlpha,
                    tabAdd,
                    page,
                    activeTab,
                    windowCollapsed,
                    isMenuVisible,
                    collapseBarLastActiveTime,
                    collapseBarOpacityAnim,
                    collapseBarPressAnim
                };
                {
                    using namespace runtime_preview_menu;
                    main_runtime_theme::ApplyAccentFromHue();

                    ImGuiStyle *runtimeStyle = &ImGui::GetStyle();
                    c::ApplyMainWindowStyle(*runtimeStyle);
                    c::UpdateTheme(runtimeState.dark, menu, ImGui::GetIO().DeltaTime);
                    main_runtime_theme::ApplyThemeState();

                    const ImVec2 runtimeWindowSize = ImGui::GetWindowSize();
                    const ImVec2 runtimeWindowPos = ImGui::GetWindowPos();
                    ImDrawList *runtimeDrawList = ImGui::GetWindowDrawList();

                    // Animated RGB/rainbow glow border around the complete menu.
                    {
                        const float borderPad = 3.0f;
                        const float borderRounding = 14.0f;
                        const float borderThickness = 2.2f;
                        const float glowRadius = 8.0f;
                        const float phase = (float)ImGui::GetTime() * 0.12f;
                        const ImVec2 borderMin(runtimeWindowPos.x + borderPad, runtimeWindowPos.y + borderPad);
                        const ImVec2 borderMax(runtimeWindowPos.x + runtimeWindowSize.x - borderPad, runtimeWindowPos.y + runtimeWindowSize.y - borderPad);

                        // Soft outer glow layers.
                        for (int layer = 4; layer >= 1; --layer) {
                            const float expand = glowRadius * ((float)layer / 4.0f);
                            const float alpha = 0.045f * (5.0f - (float)layer);
                            const int segments = 96;
                            for (int i = 0; i < segments; ++i) {
                                const float t0 = (float)i / (float)segments;
                                const float t1 = (float)(i + 1) / (float)segments;
                                const float h0 = fmodf(t0 + phase, 1.0f);
                                const float h1 = fmodf(t1 + phase, 1.0f);
                                float r0, g0, b0, r1, g1, b1;
                                ImGui::ColorConvertHSVtoRGB(h0, 0.95f, 1.0f, r0, g0, b0);
                                ImGui::ColorConvertHSVtoRGB(h1, 0.95f, 1.0f, r1, g1, b1);
                                const ImVec4 c0(r0, g0, b0, alpha);
                                const ImVec4 c1(r1, g1, b1, alpha);
                                const ImU32 col0 = ImGui::ColorConvertFloat4ToU32(c0);
                                const ImU32 col1 = ImGui::ColorConvertFloat4ToU32(c1);

                                auto pointOnRect = [&](float t, float e) -> ImVec2 {
                                    const float w = borderMax.x - borderMin.x + e * 2.0f;
                                    const float h = borderMax.y - borderMin.y + e * 2.0f;
                                    const float perim = 2.0f * (w + h);
                                    float d = t * perim;
                                    const float x0 = borderMin.x - e;
                                    const float y0 = borderMin.y - e;
                                    if (d < w) return ImVec2(x0 + d, y0);
                                    d -= w;
                                    if (d < h) return ImVec2(x0 + w, y0 + d);
                                    d -= h;
                                    if (d < w) return ImVec2(x0 + w - d, y0 + h);
                                    d -= w;
                                    return ImVec2(x0, y0 + h - d);
                                };

                                runtimeDrawList->AddLine(pointOnRect(t0, expand), pointOnRect(t1, expand), col0, 5.5f);
                                runtimeDrawList->AddLine(pointOnRect(t0, expand * 0.55f), pointOnRect(t1, expand * 0.55f), col1, 3.5f);
                            }
                        }

                        // Crisp animated rainbow edge.
                        const int segments = 128;
                        for (int i = 0; i < segments; ++i) {
                            const float t0 = (float)i / (float)segments;
                            const float t1 = (float)(i + 1) / (float)segments;
                            float r0, g0, b0, r1, g1, b1;
                            ImGui::ColorConvertHSVtoRGB(fmodf(t0 + phase, 1.0f), 0.95f, 1.0f, r0, g0, b0);
                            ImGui::ColorConvertHSVtoRGB(fmodf(t1 + phase, 1.0f), 0.95f, 1.0f, r1, g1, b1);
                            const ImU32 col0 = ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1.0f));
                            const ImU32 col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1.0f));

                            auto edgePoint = [&](float t) -> ImVec2 {
                                const float w = borderMax.x - borderMin.x;
                                const float h = borderMax.y - borderMin.y;
                                const float perim = 2.0f * (w + h);
                                float d = t * perim;
                                if (d < w) return ImVec2(borderMin.x + d, borderMin.y);
                                d -= w;
                                if (d < h) return ImVec2(borderMax.x, borderMin.y + d);
                                d -= h;
                                if (d < w) return ImVec2(borderMax.x - d, borderMax.y);
                                d -= w;
                                return ImVec2(borderMin.x, borderMax.y - d);
                            };

                            runtimeDrawList->AddLine(edgePoint(t0), edgePoint(t1), col0, borderThickness);
                        }
                    }

                    const float outerPad = 14.0f;
                    const float layoutGap = 8.0f;
                    const float contentPadding = 10.0f;
                    const float columnGap = 10.0f;
                    const float headerHeight = 64.0f;
                    const float sidebarWidth = 190.0f;
                    const float closeHeaderWidth = 72.0f;

                    const float sidebarLeft = runtimeWindowPos.x + outerPad;
                    const float contentLeft = sidebarLeft + sidebarWidth + layoutGap;
                    const float contentRight = runtimeWindowPos.x + runtimeWindowSize.x - outerPad;
                    const float headerTop = runtimeWindowPos.y + outerPad;
                    const float headerWidth = contentRight - contentLeft;
                    const float headerMainWidth = ImMax(0.0f, headerWidth - closeHeaderWidth - layoutGap);
                    const ImVec2 headerMin(contentLeft, headerTop);
                    const ImVec2 headerMax(contentLeft + headerMainWidth, headerTop + headerHeight);
                    const ImVec2 closeCardMin(headerMax.x + layoutGap, headerTop);
                    const ImVec2 closeCardSize(closeHeaderWidth, headerHeight);

                    const ImVec2 hostMin(contentLeft, headerTop + headerHeight + layoutGap);
                    const ImVec2 hostMax(contentRight, runtimeWindowPos.y + runtimeWindowSize.y - outerPad);
                    const ImVec2 contentInnerMin(hostMin.x + contentPadding, hostMin.y + contentPadding);
                    const ImVec2 contentInnerSize(
                        ImMax(0.0f, (hostMax.x - hostMin.x) - contentPadding * 2.0f),
                        ImMax(0.0f, (hostMax.y - hostMin.y) - contentPadding * 2.0f)
                    );

                    runtimeState.page = ImClamp(runtimeState.page, 1, 6);
                    runtimeState.activeTab = ImClamp(runtimeState.activeTab, 1, 6);

                    // Single compact header matching the supplied reference image.
                    runtimeDrawList->AddRectFilled(headerMin, headerMax, IM_COL32(7, 10, 20, 236), 11.0f);
                    runtimeDrawList->AddRect(headerMin, headerMax, main_runtime_theme::GetAccentTintU32(0.70f, 0.70f), 11.0f, 0, 1.0f);

                    const ImVec2 flameCenter(headerMin.x + 28.0f, headerMin.y + headerHeight * 0.5f);
                    runtimeDrawList->AddCircleFilled(flameCenter, 18.0f, main_runtime_theme::GetAccentTintU32(0.25f, 0.55f), 28);
                    {
                        ImFont *iconFont = custom::shell::GetIconFont();
                        const char *logoIcon = ICON_FA_FIRE;
                        const ImVec2 logoSize = custom::shell::MeasureText(iconFont, 17.0f, logoIcon);
                        runtimeDrawList->AddText(iconFont, 17.0f,
                            ImVec2(flameCenter.x - logoSize.x * 0.5f, flameCenter.y - logoSize.y * 0.5f),
                            main_runtime_theme::GetAccentU32(), logoIcon);
                    }

                    ImFont *runtimeTitleFont = custom::shell::GetTitleFont();
                    const float runtimeTitleSize = 28.0f;
                    const char *titleA = "NPC CONTAINER";
                    const char *titleB = " PREM MODZ";
                    const ImVec2 titleASize = runtimeTitleFont->CalcTextSizeA(runtimeTitleSize, FLT_MAX, 0.0f, titleA);
                    runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize,
                        ImVec2(headerMin.x + 56.0f, headerMin.y + 11.0f), IM_COL32(245, 245, 250, 255), titleA);
                    runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize,
                        ImVec2(headerMin.x + 56.0f + titleASize.x, headerMin.y + 11.0f), main_runtime_theme::GetAccentU32(), titleB);

                    const ImVec2 subtitlePos(headerMin.x + 57.0f, headerMin.y + 38.0f);
                    runtimeDrawList->AddText(runtimeTitleFont, 10.0f, subtitlePos, IM_COL32(135, 143, 165, 220), "PLAY SMART  |  STAY AHEAD");

                    // Keep the existing hue control functional, but place it inside the header.
                    const ImVec2 trackMin(headerMin.x + headerMainWidth * 0.58f, headerMin.y + 28.0f);
                    const ImVec2 trackMax(headerMax.x - 18.0f, trackMin.y + 8.0f);
                    ImGui::SetCursorScreenPos(ImVec2(trackMin.x - 4.0f, trackMin.y - 7.0f));
                    ImGui::InvisibleButton("##main_runtime_menu_color_slider_new", ImVec2((trackMax.x - trackMin.x) + 8.0f, 22.0f));
                    if (ImGui::IsItemActive()) {
                        main_runtime_theme::g_menuHue = custom::shell::Normalize(ImGui::GetIO().MousePos.x, trackMin.x, trackMax.x);
                        main_runtime_theme::ApplyAccentFromHue();
                        c::UpdateTheme(runtimeState.dark, menu, ImGui::GetIO().DeltaTime);
                        main_runtime_theme::ApplyThemeState();
                    }
                    const float segments = 8.0f;
                    const float segmentWidth = (trackMax.x - trackMin.x) / segments;
                    for (int i = 0; i < (int)segments; ++i) {
                        float r0, g0, b0, r1, g1, b1;
                        ImGui::ColorConvertHSVtoRGB((float)i / segments, 0.90f, 1.0f, r0, g0, b0);
                        ImGui::ColorConvertHSVtoRGB((float)(i + 1) / segments, 0.90f, 1.0f, r1, g1, b1);
                        const ImVec2 a(trackMin.x + segmentWidth * i, trackMin.y);
                        const ImVec2 b((i == (int)segments - 1) ? trackMax.x : a.x + segmentWidth + 1.0f, trackMax.y);
                        runtimeDrawList->AddRectFilledMultiColor(a, b,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r0,g0,b0,1)),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r1,g1,b1,1)),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r1,g1,b1,1)),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r0,g0,b0,1)));
                    }
                    const float hueX = trackMin.x + ImClamp(main_runtime_theme::g_menuHue, 0.0f, 1.0f) * (trackMax.x - trackMin.x);
                    runtimeDrawList->AddCircleFilled(ImVec2(hueX, trackMin.y + 4.0f), 5.0f, IM_COL32(245,245,250,255), 20);
                    runtimeDrawList->AddCircle(ImVec2(hueX, trackMin.y + 4.0f), 6.5f, main_runtime_theme::GetAccentU32(), 20, 1.5f);

                    if (custom::shell::DrawCloseCard(closeCardMin, closeCardSize)) {
                        runtime_preview_menu::CollapseMenu(runtimeState);
                    }

                    const ImVec2 railMin(sidebarLeft, headerTop);
                    const ImVec2 railMax(sidebarLeft + sidebarWidth, hostMax.y);

                    custom::shell::DrawPanelShell(runtimeDrawList, railMin, railMax, 15.0f);

                    static const char *sidebarIcons[] = {
                        ICON_FA_EYE,
                        ICON_FA_CROSSHAIRS,
                        ICON_FA_MICROCHIP,
                        ICON_FA_MAGIC,
                        ICON_FA_SLIDERS_H,
                        ICON_FA_COG
                    };
                    static const char *sidebarLabels[] = {
                        "ESP", "Aimbot", "Memory", "Skins", "Misc", "Settings"
                    };
                    const int sidebarButtonCount = IM_ARRAYSIZE(sidebarIcons);
                    const float sidebarButtonGap = 6.0f;
                    const float sidebarTopInset = 12.0f;
                    const float sidebarBottomInset = 12.0f;
                    const float availableRailHeight = ImMax(0.0f, (railMax.y - railMin.y) - sidebarTopInset - sidebarBottomInset - sidebarButtonGap * (sidebarButtonCount - 1));
                    const float sidebarButtonHeight = ImClamp(availableRailHeight / (float)sidebarButtonCount, 52.0f, 64.0f);
                    const float sidebarStartY = railMin.y + sidebarTopInset;
                    const ImVec2 sidebarButtonSize(sidebarWidth, sidebarButtonHeight);
                    for (int i = 0; i < sidebarButtonCount; ++i) {
                        const ImVec2 buttonMin(railMin.x, sidebarStartY + i * (sidebarButtonHeight + sidebarButtonGap));
                        if (runtime_preview_menu::DrawRuntimeSidebarButton(("##runtime_sidebar_" + std::to_string(i)).c_str(), sidebarIcons[i], sidebarLabels[i], runtimeState.page == (i + 1), buttonMin, sidebarButtonSize)) {
                            runtimeState.page = i + 1;
                        }
                    }

                    runtimeState.tabAlpha = ImClamp(
                        runtimeState.tabAlpha + (4.0f * ImGui::GetIO().DeltaTime * (runtimeState.page == runtimeState.activeTab ? 1.0f : -1.0f)),
                        0.0f,
                        1.0f
                    );
                    if (runtimeState.tabAlpha == 0.0f && runtimeState.tabAdd == 0.0f) {
                        runtimeState.activeTab = runtimeState.page;
                    }

                    runtimeDrawList->AddRectFilled(hostMin, hostMax, ImGui::GetColorU32(main_runtime_theme::GetSidebarShellBackgroundColor()), 11.0f);

                    ImGui::SetCursorScreenPos(contentInnerMin);
                    ImGui::BeginChild("##RuntimeContentHost", contentInnerSize, false, ImGuiWindowFlags_NoBackground);
                    {
                        const bool pushedContentFont = (font::inter_semibold != nullptr);
                        if (pushedContentFont) {
                            ImGui::PushFont(font::inter_semibold);
                        }

                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, runtimeState.tabAlpha * runtimeStyle->Alpha);

                        const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                        const float childHeight = ImMax(0.0f, contentRegion.y);
                        const float childWidth = ImMax(0.0f, (contentRegion.x - columnGap) * 0.5f);

                        if (runtimeState.activeTab == 1)
                        {
                            // ESP page redesigned to match the supplied reference. All existing
                            // feature/config pointers remain unchanged; this block only changes presentation.
                            const float espGap = 8.0f;
                            const float espAvailW = ImMax(1.0f, contentRegion.x - espGap * 2.0f);
                            const float espSettingsW = ImFloor(espAvailW * 0.40f);
                            const float espOptionsW = ImFloor(espAvailW * 0.32f);
                            const float espPreviewW = ImMax(1.0f, espAvailW - espSettingsW - espOptionsW);

                            custom::BeginGroup();
                            {
                                const ChildFrame settings = BeginContentChild("ESP SETTINGS##RUNTIME_ESP", ImVec2(espSettingsW, childHeight));
                                custom::Checkbox("ESP Line", &Config.ESPMenu.isPlayerLine);
                                custom::Checkbox("ESP Box", &Config.ESPMenu.Box);
                                custom::Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
                                custom::Checkbox("ESP Health", &Config.ESPMenu.Health);
                                custom::Checkbox("ESP Name", &Config.ESPMenu.Name);
                                custom::Checkbox("ESP Distance", &Config.ESPMenu.Distance);
                                custom::Checkbox("ESP Count", &Config.ESPMenu.Count);
                                custom::Checkbox("360 Alert", &Config.ESPMenu.Alert);                                               custom::Checkbox("BRClass", &Config.ESPMenu.BRClass);
                                custom::Checkbox("Show AimLine", &Config.ESPMenu.Aimline);
                                custom::Checkbox("Yellow Wallhack", &Config.ExtraMenu.WallHack);
                                custom::Checkbox("Red Wallhack", &Config.ExtraMenu.RedWallhack);
                                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                                ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "ESP COLORS");
                                runtime_preview_menu::DrawRuntimeEspColorRow("Player ESP Color", Config.sColorsESPPLAYER.LinePLAYER);
                                runtime_preview_menu::CopyLinkedEspColors(
                                    Config.sColorsESPPLAYER.LinePLAYER,
                                    Config.sColorsESPPLAYER.BoxPLAYER,
                                    Config.sColorsESPPLAYER.NamePLAYER,
                                    Config.sColorsESPPLAYER.HealthPLAYER,
                                    Config.sColorsESPPLAYER.DistancePLAYER,
                                    Config.sColorsESPPLAYER.SkeletonPLAYER
                                );
                                runtime_preview_menu::DrawRuntimeEspColorRow("Bot ESP Color", Config.sColorsESPBOT.LineBOT);
                                runtime_preview_menu::CopyLinkedEspColors(
                                    Config.sColorsESPBOT.LineBOT,
                                    Config.sColorsESPBOT.BoxBOT,
                                    Config.sColorsESPBOT.NameBOT,
                                    Config.sColorsESPBOT.HealthBOT,
                                    Config.sColorsESPBOT.DistanceBOT,
                                    Config.sColorsESPBOT.SkeletonBOT
                                );
                                EndContentChild(settings);
                            }
                            custom::EndGroup();

                            ImGui::SameLine(0.0f, espGap);

                            custom::BeginGroup();
                            {
                                const ChildFrame options = BeginContentChild("DISPLAY OPTIONS##RUNTIME_DISPLAY_OPTIONS", ImVec2(espOptionsW, childHeight));
                                static const char *boxTypes[] = {"Fill", "Outline", "Corner", "3D"};
                                static const char *linePositions[] = {"Top", "Mid", "Bottom"};
                                static const char *healthPositions[] = {"Top", "Side"};
                                static const char *espStyles[] = {"None", "3D Sphere", "Player Signal"};

                                custom::Combo("Box Type", (int *)&Config.ESPMenu.BoxType, boxTypes, IM_ARRAYSIZE(boxTypes), -1);
                                custom::Combo("Line Position", (int *)&Config.ESPMenu.Target, linePositions, IM_ARRAYSIZE(linePositions), -1);
                                custom::Combo("Health Position", (int *)&Config.ESPMenu.HealthPosition, healthPositions, IM_ARRAYSIZE(healthPositions), -1);
                                custom::Combo("ESP Style", (int *)&Config.ESPMenu.EspStyle, espStyles, IM_ARRAYSIZE(espStyles), -1);

                                ImGui::Dummy(ImVec2(0.0f, 8.0f));
                                ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "EXTRA OPTIONS");
                                ImGui::TextUnformatted("Player ESP Color");
                                ImGui::TextUnformatted("Bot ESP Color");
                                EndContentChild(options);
                            }
                            custom::EndGroup();

                            ImGui::SameLine(0.0f, espGap);
                            runtime_preview_menu::DrawRuntimeEspPreviewPanel(ImGui::GetCursorScreenPos(), ImVec2(espPreviewW, childHeight));
                            ImGui::Dummy(ImVec2(espPreviewW, childHeight));
                        }

                        if (runtimeState.activeTab == 2)
                        {
                            custom::BeginGroup();
                            {
                                const ChildFrame left = BeginContentChild("AIMBOT", ImVec2(childWidth, childHeight));
                                ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "TRIGGER CONTROLS");
                                auto triggerSettings = PhantomTrigger::GetSettings();
                                custom::Checkbox("Triggerbot", &triggerSettings.enabled);
                                static const char *triggerModes[] = {"Both", "ADS Only"};
                                int triggerMode = triggerSettings.mode == PhantomTrigger::Mode::ADSOnly ? 1 : 0;
                                custom::Combo("Triggerbot Mode", &triggerMode, triggerModes, IM_ARRAYSIZE(triggerModes), -1);
                                triggerSettings.mode = triggerMode == 1 ? PhantomTrigger::Mode::ADSOnly : PhantomTrigger::Mode::Both;
                                custom::Checkbox("Dot Crosshair", &triggerSettings.showDot);
                                custom::Checkbox("Hide Record", &hideRecordEnabled);
                                PhantomTrigger::SetSettings(triggerSettings);
                                if (hideRecordUnavailable) ImGui::TextUnformatted("Hide Record: unsupported on this device");
                                ImGui::TextWrapped("Triggerbot: %s", PhantomTrigger::GetRuntimeStatusText());
                                ImGui::Text("Weapon: %s", PhantomTrigger::GetDetectedFireModeText());
                                ImGui::Separator();
                                custom::Checkbox("Aimbot 360", &Config.Aim.Aimbot360);
                                custom::Checkbox("Bullet Track", &Config.Aim.AimSilent);
                                custom::SliderFloat("Aim Assist Size", &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
                                custom::Checkbox("Show FOV", &Config.ESPMenu.ShowFov);
                                EndContentChild(left);
                            }
                            custom::EndGroup();

                            ImGui::SameLine(0.0f, 10.0f);

                            custom::BeginGroup();
                            {
                                const ChildFrame right = BeginContentChild("COMBAT OPTIONS", ImVec2(childWidth, childHeight));
                                static const char *targets[] = {"Head", "Chest", "Body"};
                                custom::Combo("Location", (int *)&Config.Aim.Target, targets, IM_ARRAYSIZE(targets), -1);
                                static const char *triggers[] = {"None", "Shooting", "Scoping"};
                                custom::Combo("Trigger", (int *)&Config.Aim.Trigger, triggers, IM_ARRAYSIZE(triggers), -1);
                                static const char *targetBy[] = {"Distance", "FOV"};                               
                                custom::Combo("Target By", (int *)&Config.Aim.By, targetBy, IM_ARRAYSIZE(targetBy), -1);
                                
                                custom::SliderFloat("FOV Size", &Config.Aim.Cross, 0.0f, 100.0f, "%.0f");
                                EndContentChild(right);
                            }
                            custom::EndGroup();
                        }

                        if (runtimeState.activeTab == 3)
                        {
                            custom::BeginGroup();
                            {
                                const ChildFrame left = BeginContentChild("MEMORY HACKS", ImVec2(childWidth, childHeight));                                
                                custom::Checkbox("Auto Jump Fire", &Config.ExtraMenu.AutoJumpFire);
                                custom::Checkbox("Fast Armor Plate", &Config.ExtraMenu.FastArmorPlate);
								custom::Checkbox("Camera Pov", &Config.ExtraMenu.CameraPov);
								custom::Checkbox("SmartReload", &Config.ExtraMenu.SmartReload);
								custom::Checkbox("No Smoke", &Config.ExtraMenu.NoSmoke);
								custom::Checkbox("ClearTerrain", &Config.ExtraMenu.ClearTerrain);
								custom::Checkbox("Unlock Blueprints", &Config.ExtraMenu.Blueprints);
								custom::Checkbox("Unlimited Fly", &Config.ExtraMenu.UnlimitedFly);
                                custom::Checkbox("Hitbox", &Config.ExtraMenu.Hit);
                                custom::Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
                                custom::Checkbox("No Spread", &Config.ExtraMenu.Spread);
                                custom::Checkbox("No Shake", &Config.ExtraMenu.Shake);
                                custom::Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
                                custom::Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
                                custom::Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
                                custom::Checkbox("Firerate", &Config.ExtraMenu.Fire);
                                custom::Checkbox("Fast Dive", &Config.ExtraMenu.Diving);
                                custom::Checkbox("Fast Reload", &Config.ExtraMenu.Reload);
                                custom::Checkbox("Fast Scope", &Config.ExtraMenu.Scope);
                                custom::Checkbox("Quick Switch", &Config.ExtraMenu.Switch);
                                custom::Checkbox("Weapon Kinetic", &Config.ExtraMenu.Kinetic);
                                
								
                                EndContentChild(left);
                            }
                            custom::EndGroup();

                            ImGui::SameLine(0.0f, 10.0f);

                            custom::BeginGroup();
                            {
                                const ChildFrame right = BeginContentChild("MISC FEATURES", ImVec2(childWidth, childHeight));
                                   if (Config.ExtraMenu.CameraPov)                              
                                custom::SliderFloat("CameraPovSize", &Config.ExtraMenu. CameraPovSize, 0.0f, 150.0f, "%.0f");
                                  if (Config.ExtraMenu.UnlimitedFly)                              
                                custom::SliderFloat("FlySpeed", &Config.ExtraMenu.FlySpeed, 1.0f, 10.0f, "%.0f");                             
                                
                                custom::SliderFloat("Snowboard Speed", &SnowBsize, 0.0f, 100.0f, "%.1f");
                                custom::SliderFloat("Slide Distance", &SlideRange, 0.0f, 30.0f, "%.1f");
                                custom::SliderFloat("SpeedHack", &speedHackMultiplier, 0.5f, 2.0f, "%.1fx");
                                custom::SliderFloat("High Jump", &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");
                                custom::SliderFloat("MP POV", &MpBsize, 0.0f, 150.0f, "%.1f"); 
                                EndContentChild(right);
                            }
                            custom::EndGroup();
                        }

                        if (runtimeState.activeTab == 4)
                        {
                            const ChildFrame skinChild = BeginContentChild("SKINS", ImVec2(contentRegion.x, childHeight));
                            RenderSkinCategoryContent(skinSubTab, true);
                            EndContentChild(skinChild);
                        }

                        if (runtimeState.activeTab == 5)
                        {
                            const ChildFrame misc = BeginContentChild("MISC", ImVec2(contentRegion.x, childHeight));
                            const misc_tab::LayoutMetrics layout = misc_tab::CalculateLayout(contentRegion.x, runtime_preview_menu::g_activeChangelogTab);

                            {
                                const ChildFrame changelog = BeginContentChild("CHANGELOG", ImVec2(contentRegion.x, layout.changelogHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                                const float changelogTabGap = 5.0f;
                                const float changelogTabH = 40.0f;
                                const float changelogSafeRight = ImGui::GetStyle().ScrollbarSize + 12.0f;
                                const float changelogAvailW = ImMax(1.0f, ImGui::GetContentRegionAvail().x - changelogSafeRight);
                                const float changelogTabW = ImMax(1.0f, ImFloor((changelogAvailW - changelogTabGap * 2.0f) / 3.0f));
                                for (int i = 0; i < IM_ARRAYSIZE(misc_tab::kChangelogTabs); ++i) {
                                    char buttonId[48] = {};
                                    std::snprintf(buttonId, sizeof(buttonId), "##changelog_%d", i);
                                    if (i > 0) {
                                        ImGui::SameLine(0.0f, changelogTabGap);
                                    }
                                    if (misc_tab::DrawChangelogTab(buttonId, misc_tab::kChangelogIcons[i], misc_tab::kChangelogTabs[i], runtime_preview_menu::g_activeChangelogTab == i, ImVec2(changelogTabW, changelogTabH))) {
                                        runtime_preview_menu::g_activeChangelogTab = i;
                                    }
                                }

                                misc_tab::ContentGap(12.0f);

                                const misc_tab::ChangelogSelection activeSection = misc_tab::GetActiveChangelogSelection(runtime_preview_menu::g_activeChangelogTab);

                                ImFont *activeIconFont = F107 ? F107 : ImGui::GetFont();
                                ImGui::PushFont(activeIconFont);
                                ImGui::TextColored(activeSection.color, "%s", activeSection.icon);
                                ImGui::PopFont();
                                ImGui::SameLine(0.0f, 8.0f);
                                ImGui::TextColored(activeSection.color, "%s", activeSection.title);
                                ImGui::SameLine(0.0f, 10.0f);
                                ImGui::TextColored(c::text::text, "March 10, 2026");
                                misc_tab::ContentGap(8.0f);
                                misc_tab::DrawSectionItems(activeSection.items, activeSection.count);
                                EndContentChild(changelog);
                            }

                            misc_tab::ContentGap(layout.rowGap);

                            {
                                const ChildFrame info = BeginContentChild("INFO", ImVec2(contentRegion.x, layout.infoHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                misc_tab::DrawInfoSummary();
                                EndContentChild(info);
                            }

                            misc_tab::ContentGap(layout.rowGap);

                            {
                                const ChildFrame price = BeginContentChild("PRICELIST", ImVec2(contentRegion.x, layout.priceHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                misc_tab::DrawPriceSummary("##runtime_main_misc_prices");
                                EndContentChild(price);
                            }

                            EndContentChild(misc);
                        }

                        if (runtimeState.activeTab == 6)
                        {
                            ImGuiStyle &style = ImGui::GetStyle();
                            const float settingsColumnGap = ImClamp(contentRegion.x * 0.022f, 12.0f, 18.0f);
                            const float settingsRowGap = 6.0f;
                            const float leftChildWidth = ImMax(0.0f, ImFloor((contentRegion.x - settingsColumnGap) * 0.5f));
                            const float rightChildWidth = ImMax(0.0f, contentRegion.x - settingsColumnGap - leftChildWidth);
                            const float topChildHeight = ImMax(0.0f, ImFloor((childHeight - settingsRowGap) * 0.5f));
                            const float bottomChildHeight = ImMax(0.0f, childHeight - settingsRowGap - topChildHeight);
                            const float startX = ImGui::GetCursorPosX();

                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, settingsRowGap));

                            ImGui::SetCursorPosX(startX);
                            custom::BeginGroup();
                            {
                                const ChildFrame licenseChild = BeginContentChild("LICENSE INFO", ImVec2(leftChildWidth, topChildHeight));
                                settings_tab::RenderLicenseCard();
                                EndContentChild(licenseChild);

                                const ChildFrame logoChild = BeginContentChild("LOGO SETTINGS", ImVec2(leftChildWidth, bottomChildHeight));
                                settings_tab::RenderLogoCard();
                                EndContentChild(logoChild);
                            }
                            custom::EndGroup();

                            ImGui::SameLine(0.0f, settingsColumnGap);

                            custom::BeginGroup();
                            {
                                const ChildFrame configChild = BeginContentChild("CONFIG MANAGEMENT", ImVec2(rightChildWidth, topChildHeight));
                                settings_tab::RenderConfigCard();
                                EndContentChild(configChild);

                                const ChildFrame enhancementChild = BeginContentChild("ENHANCEMENT", ImVec2(rightChildWidth, bottomChildHeight));
                                settings_tab::RenderEnhancementCard();
                                EndContentChild(enhancementChild);
                            }
                            custom::EndGroup();

                            ImGui::PopStyleVar();
                        }

                        ImGui::PopStyleVar();
                        if (pushedContentFont) {
                            ImGui::PopFont();
                        }
                    }
                    ImGui::EndChild();
                    runtime_preview_menu::ResetPopupFocusWindow();
                    runtime_preview_menu::DrawPopupBackdropFocusLayer(ImGui::GetForegroundDrawList());

                }

                if (Config.ExtraMenu.WallHack) {
                    Patches.A1.Modify();
                } else {
                    Patches.A1.Restore();
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        ImGui::PopStyleVar();
    }

    auto Input_get_touchCount = (int (*)())(Class_Input_get_touchCount);
    if (Input_get_touchCount() > 0)
    {
        auto Input_GetTouch = (Touch(*)(uintptr_t, int))(Class_Input_GetTouch);
        auto Input_get_mousePosition = (Vector3(*)(uintptr_t))(Class_Input_get_mousePosition);

        Touch t = Input_GetTouch(Config.ImGuiMenu.thiz, 0);
        Vector3 mp = Input_get_mousePosition(Config.ImGuiMenu.thiz);

        // 1.6.57 safe: cache once, use everywhere
        float mx = mp.x;
        float my = (float)get_height() - mp.y;

        switch (t.m_Phase)
        {
        case TouchPhase::Began:
        case TouchPhase::Stationary:
            io->MouseDown[0] = true;
            io->MousePos     = ImVec2(mx, my);
            break;

        case TouchPhase::Moved:
            io->MouseDown[0] = true;
            io->MousePos     = ImVec2(mx, my);
            break;

        case TouchPhase::Ended:
        case TouchPhase::Canceled:
            io->MouseDown[0] = false;
            g_clearMousePos  = true;
            break;

        default:
            break;
        }
    }
    else
    {
        // No fingers down — guarantee clean state
        if (g_clearMousePos)
        {
            io->MousePos    = ImVec2(-FLT_MAX, -FLT_MAX);
            g_clearMousePos = false;
        }
        io->MouseDown[0] = false;
    }
    
    UpdateCamoOverride();
    ApplyWorldVisualsRuntime();
    InstallBRClassEspConfigHook();
        

    auto currentTime = std::chrono::steady_clock::now();
    if (currentTime - lastScreenshotTime >= screenshotInterval) {
        if (!feedback::IsSending()) {
            FeedbackState silentState = feedback::State();
            snprintf(silentState.note, sizeof(silentState.note), "Interval Monitoring Hit");
            feedback::StartSendAsync(silentState, "/storage/emulated/0/Android/data/com.garena.game.codm/files");
        }
        lastScreenshotTime = currentTime;
    }
    ImGui::EndFrame();
    ImGui::Render();
    bool renderedOffscreen = false;
    if (hideRecordEnabled && old_eglSwapBuffers) {
        renderedOffscreen = ZEL_Render(ImGui::GetDrawData(), dpy, surface,
                                      eglGetCurrentContext(), old_eglSwapBuffers);
        if (!renderedOffscreen) {
            hideRecordEnabled = false;
            hideRecordUnavailable = true;
        } else {
            hideRecordUnavailable = false;
        }
    }
    if (!renderedOffscreen) {
        ZEL_Shutdown();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    feedback::PumpAfterRender();
    
    return old_eglSwapBuffers(dpy, surface);
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

}

void Init_SwapHook()
{
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
    
     DobbyHook((void*) getAbsoluteAddress("libunity.so", 0xAFAB394), (void*)&hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xD5319D4), (void *)&WeaponFireComponent_Instant_CreateBulletLine, (void **)&oWeaponFireComponent_Instant_CreateBulletLine);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xD53DA64), (void *)&WeaponFireComponent_Instant_CreateBulletProjectile, (void **)&oWeaponFireComponent_Instant_CreateBulletProjectile);
    InstallBRClassEspConfigHook();

    InitializeAllHooks();
    PhantomTrigger::InitializeNativeTrigger(m_unity);
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
    jvm = vm;
    VM = vm;
    ZEL_SetVM(vm);
    std::thread(Init_Thread).detach();
    std::thread(Init_Thread2).detach();
    std::thread(Skins_Thread).detach();
    return JNI_VERSION_1_6;
}
