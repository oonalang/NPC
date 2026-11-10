#pragma once

#include <string>
#include <cstring>
#include <cstdio>
#include <android/log.h>

namespace AntiChangePanel {

    inline std::string GetExpectedUrl() {
        return "https://ethnir.x10.mx/connect.php";
    }

    inline bool IsVirtualInjectionDetected() {
        FILE *f = std::fopen("/proc/self/maps", "r");
        if (!f) return false;
        
        char line[512];
        bool detected = false;
        
        while (std::fgets(line, sizeof(line), f)) {
            if (std::strstr(line, "VirtualApp") || 
                std::strstr(line, "parallel") || 
                std::strstr(line, "frida") || 
                std::strstr(line, "xposed") || 
                std::strstr(line, "magisk") ||
                std::strstr(line, "substrate") ||
                std::strstr(line, "SandVxposed") ||
                std::strstr(line, "VirtualXposed")) {
                
                detected = true;
                break;
            }
        }
        std::fclose(f);
        return detected;
    }

    inline bool IsPanelSafe(const std::string& currentUrl) {
        if (currentUrl != GetExpectedUrl()) {
            __android_log_print(ANDROID_LOG_ERROR, "AntiChangePanel", "Panel link changed!");
            return false;
        }
        
        if (IsVirtualInjectionDetected()) {
            __android_log_print(ANDROID_LOG_ERROR, "AntiChangePanel", "Virtual Space/Hook detected!");
            return false;
        }
        return true;
    }
}
