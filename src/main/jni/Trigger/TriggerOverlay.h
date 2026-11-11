#pragma once

#include "TriggerRuntime.h"
#include "TriggerDotStyle.h"
#include "../ImGui/imgui.h"
#include <cmath>

namespace PhantomTrigger {

inline void DrawStyledDot(ImDrawList* draw, const ImVec2& center) {
    if (!draw) return;
    auto& style = TriggerDotStyle::appearance;
    TriggerDotStyle::Normalize(style);
    const int segments = 24;
    if (style.outline > 0.0f)
        draw->AddCircleFilled(center, style.radius + style.outline,
                              ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, style.opacity)), segments);
    draw->AddCircleFilled(center, style.radius,
                          ImGui::GetColorU32(ImVec4(style.color[0], style.color[1],
                                                   style.color[2], style.opacity)), segments);
}

inline void DrawTriggerDot(ImDrawList* draw, float width, float height) {
    if (!draw || !GetSettings().showDot || !std::isfinite(width) ||
        !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
        return;

    float normalizedX = 0.5f;
    float normalizedY = 0.5f;
    GetViewportCenter(normalizedX, normalizedY);
    if (!std::isfinite(normalizedX) || !std::isfinite(normalizedY) ||
        normalizedX < 0.0f || normalizedX > 1.0f ||
        normalizedY < 0.0f || normalizedY > 1.0f) {
        normalizedX = 0.5f;
        normalizedY = 0.5f;
    }

    DrawStyledDot(draw, ImVec2(normalizedX * width, normalizedY * height));
}

}
