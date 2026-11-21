#pragma once
#include <cstddef>
#include <cstdint>

namespace lite_blur {
inline constexpr int maximumWidth = 64, maximumHeight = 192;
inline constexpr double refreshSeconds = 1./20.;
struct Region { float x, y, width, height; };
struct Texture { std::uint32_t id = 0; int width = 0, height = 0; };
struct Statistics { std::size_t textureBytes = 0; std::uint64_t captures = 0; };
Texture Capture(Region region, int framebufferWidth, int framebufferHeight, double now);
Statistics GetStatistics();
void Release();
}
