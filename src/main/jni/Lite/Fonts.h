#pragma once
#include "../ImGui/imgui.h"
#include <cstddef>
#include "SakugaArt.h"

extern "C" {
extern const unsigned char lite_body_start[], lite_body_end[];
extern const unsigned char lite_title_start[], lite_title_end[];
extern const unsigned char lite_icons_start[], lite_icons_end[];
}

namespace lite_fonts {
struct Fonts { ImFont* body; ImFont* title; };
inline Fonts Load(ImFontAtlas& atlas, const void* body, std::size_t bodySize,
                  const void* title, std::size_t titleSize,
                  const void* icons, std::size_t iconsSize) {
    sakuga_art::Register(atlas);
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH = config.OversampleV = 1;
    config.PixelSnapH = true;
    auto* regular = atlas.AddFontFromMemoryTTF(const_cast<void*>(body), static_cast<int>(bodySize), 17.f,
        &config, atlas.GetGlyphRangesDefault());
    static constexpr ImWchar iconRanges[]{0xf013,0xf013,0xf015,0xf015,0xf05b,0xf05b,0xf07b,0xf07b,
        0xf0e7,0xf0e7,0xf1fc,0xf1fc,0xf500,0xf500,0};
    config.MergeMode = true;
    atlas.AddFontFromMemoryTTF(const_cast<void*>(icons), static_cast<int>(iconsSize), 17.f,&config,iconRanges);
    config.MergeMode = false;
    auto* heading = atlas.AddFontFromMemoryTTF(const_cast<void*>(title), static_cast<int>(titleSize), 30.f,
        &config, atlas.GetGlyphRangesDefault());
    return {regular, heading};
}
inline void Build(ImFontAtlas& atlas) {
    atlas.Build();
    sakuga_art::Build(atlas);
}
}
