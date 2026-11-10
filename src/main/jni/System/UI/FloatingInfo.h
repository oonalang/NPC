#pragma once

#include "../../ImGui/imgui_settings.h"

#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <string>

namespace font {
    extern ImFont* inter_semibold;
}

extern std::chrono::steady_clock::time_point appStartTime;

namespace floating_info {

inline std::string FormatTimeDuration(std::chrono::steady_clock::duration duration) {
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1)).count();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1)).count();
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", (int)hours, (int)minutes, (int)seconds);
    return std::string(buffer);
}

inline void Render(ImDrawList* draw, float screenWidth, float screenHeight) {
    IM_UNUSED(screenWidth);
    if (draw == nullptr || Config.ExtraMenu.ClearDisplay) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);

    char dateStr[64] = {};
    char timeStr[64] = {};
    std::strftime(dateStr, sizeof(dateStr), "%B %d, %Y", localTime);
    std::strftime(timeStr, sizeof(timeStr), "%I:%M %p", localTime);

    for (char* p = dateStr; *p; ++p) *p = (char)std::toupper((unsigned char)*p);
    for (char* p = timeStr; *p; ++p) *p = (char)std::toupper((unsigned char)*p);

    auto currentDuration = std::chrono::steady_clock::now() - appStartTime;
    std::string playTime = FormatTimeDuration(currentDuration);
    for (char& ch : playTime) ch = (char)std::toupper((unsigned char)ch);

    const float boxW = 334.0f * c::scale;
    const float boxH = 66.0f * c::scale;
    const float padding = 22.0f * c::scale;
    const float boxX = padding;
    const float boxY = screenHeight - padding - boxH;
    const float lineSpacing = 17.0f * c::scale;
    const float textStartX = boxX + 12.0f * c::scale;
    const float startY = boxY + 12.0f * c::scale;

    auto drawTextWithSpacing = [&](const char* text, ImVec2 pos, ImColor color) {
        float xOffset = 0.0f;
        const float charSpacing = 1.2f * c::scale;
        for (int i = 0; text[i] != '\0'; ++i) {
            char glyph[2] = { text[i], '\0' };
            draw->AddText(font::inter_semibold, 13.0f * c::scale, ImVec2(pos.x + xOffset, pos.y), color, glyph);
            xOffset += font::inter_semibold->CalcTextSizeA(13.0f * c::scale, FLT_MAX, 0.0f, glyph).x + charSpacing;
        }
    };

    if (!font::inter_semibold) {
        return;
    }

    char line2[128] = {};
    char line3[64] = {};
    char line4[64] = {};
    std::snprintf(line2, sizeof(line2), "DATE : %s", dateStr);
    std::snprintf(line3, sizeof(line3), "TIME : %s", timeStr);
    std::snprintf(line4, sizeof(line4), "PLAYING TIME : %s", playTime.c_str());

    const ImColor accent(188, 110, 255, 228);
    const ImColor white(255, 255, 255, 255);
    drawTextWithSpacing("DEV -✥NPC|•CONTAINER✥", ImVec2(textStartX, startY), accent);
    drawTextWithSpacing(line2, ImVec2(textStartX, startY + lineSpacing), white);
    drawTextWithSpacing(line3, ImVec2(textStartX, startY + lineSpacing * 2), white);
    drawTextWithSpacing(line4, ImVec2(textStartX, startY + lineSpacing * 3), white);
}

} // namespace floating_info
