#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../Fonts/Icon.h"
#include "MusicPlayer.h"
#include <cstdio>

namespace home_tab {
struct HomeStats {
    float fps = 0.f;
    float frameMs = 0.f;
    double uptime = 0.0;
    int width = 0;
    int height = 0;
    int activeFeatures = 0;
    bool keySaved = false;
};

inline void Row(const char *value, const char *label)
{
    ImGui::TextColored(c::accent, "%s", value);
    ImGui::SameLine();
    ImGui::TextColored(c::text::text, "%s", label);
}

inline void Uptime(char *out, std::size_t size, double seconds)
{
    const int total = seconds > 0.0 ? static_cast<int>(seconds) : 0;
    std::snprintf(out, size, "%02d:%02d:%02d", total / 3600, (total / 60) % 60, total % 60);
}

inline void DrawStats(const HomeStats &stats)
{
    char buffer[64] = {};
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f * c::scale, 7.f * c::scale));
    std::snprintf(buffer, sizeof(buffer), "%.0f fps", stats.fps);
    Row(buffer, "render rate");
    std::snprintf(buffer, sizeof(buffer), "%.1f ms", stats.frameMs);
    Row(buffer, "frame time");
    Uptime(buffer, sizeof(buffer), stats.uptime);
    Row(buffer, "session uptime");
    std::snprintf(buffer, sizeof(buffer), "%d x %d", stats.width, stats.height);
    Row(buffer, "surface");
    std::snprintf(buffer, sizeof(buffer), "%d", stats.activeFeatures);
    Row(buffer, "features enabled");
    Row(stats.keySaved ? "Cached" : "None", "access key");
    ImGui::PopStyleVar();
}
}
