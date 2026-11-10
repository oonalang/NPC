/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                                                                       ║
 * ║   ██╗  ██╗   ██╗ ██████╗███╗  ██╗██████╗ ██████╗  ██████╗  ██████╗    ║
 * ║   ██║  ╚██╗ ██╔╝██╔════╝████╗ ██║██╔══██╗██╔══██╗██╔═══██╗██╔════╝    ║
 * ║   ██║   ╚████╔╝ ██║     ██╔██╗██║██████╔╝██████╔╝██║   ██║██║         ║
 * ║   ██║    ╚██╔╝  ██║     ██║╚████║██╔═══╝ ██╔══██╗██║   ██║██║         ║
 * ║   ███████╗██║   ╚██████╗██║ ╚███║██║     ██║  ██║╚██████╔╝╚██████╗    ║
 * ║   ╚══════╝╚═╝    ╚═════╝╚═╝  ╚══╝╚═╝     ╚═╝  ╚═╝ ╚═════╝  ╚═════╝    ║
 * ║                                                                       ║
 * ║                ANTICRACK & ANTI-TAMPERING  | v2.1 | ^w^               ║
 * ║                                                                       ║
 * ║   Protection  │  Random Key - Login Removal (W8) - Hex Changes        ║
 * ║   Author      │  Lyc4nLĐ (@lyc4nld) >w<                               ║
 * ║   License     │  Private — do NOT redistribute  OwO                   ║
 * ║                                                                       ║
 * ║               Version 2.2 | Build 21032026 for Nyxar                  ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */
#pragma once
#include <string>
#include "ImGui/imgui.h"

namespace AntiCrack {
    void Init(ImFont* boldFont = nullptr);

    void NotifyLoginSuccess(const char*        keyBuffer,
                            const std::string& storedKey,
                            const std::string& expString);

    void NotifyIsLogin(bool isLogin);

    void Tick();
    void RenderScreen();

    bool        IsCrackerVisible();
    bool        WasCrackDetected();
    const char* GetRemainingTimeString(const std::string& expirationTimeStr);
    std::string GetFormattedCurrentTime();
    enum class LoginValidation { OK, RTT_TOO_FAST };
    LoginValidation ValidateLoginResponse(long rtt_ms);
    const char*     LoginValidationMessage(LoginValidation result);
    bool IsAuthenticatedSession();
    void NotifyMenuRendering(); 
    void ForceTrigger();

}