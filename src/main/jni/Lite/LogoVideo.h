#pragma once
#include "../ImGui/imgui.h"
#include "LogoVideoArt.h"
#include <cmath>
#include <cstdint>
#if defined(__ANDROID__) || defined(LITE_GPU_TEST)
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include "../ImGui/stb_image.h"
#endif

namespace lite_logo {
inline constexpr int columns = 12, rows = 9, frameCount = 104;
inline constexpr int frameSize = 80, cellSize = 84, padding = 2;
inline constexpr int atlasWidth = columns * cellSize, atlasHeight = rows * cellSize;
inline constexpr double duration = 8.709002;

inline int Frame(double time, bool reducedMotion) {
    if (reducedMotion || !std::isfinite(time) || time <= 0.) return 0;
    const auto loop = std::fmod(time, duration);
    return ImClamp(static_cast<int>(loop * frameCount / duration), 0, frameCount - 1);
}

inline void Uv(int frame, ImVec2& lo, ImVec2& hi) {
    frame = ImClamp(frame, 0, frameCount - 1);
    const int column = frame % columns, row = frame / columns;
    lo = {static_cast<float>(column * cellSize + padding) / atlasWidth,
        static_cast<float>(row * cellSize + padding) / atlasHeight};
    hi = {static_cast<float>(column * cellSize + padding + frameSize) / atlasWidth,
        static_cast<float>(row * cellSize + padding + frameSize) / atlasHeight};
}

#if defined(__ANDROID__) || defined(LITE_GPU_TEST)
inline GLuint texture = 0;
inline EGLContext context = EGL_NO_CONTEXT;

inline ImTextureID Texture() {
    return reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(texture));
}

inline ImTextureID Ensure() {
    const auto current = eglGetCurrentContext();
    if (current == EGL_NO_CONTEXT) return nullptr;
    if (current != context) { texture = 0; context = current; }
    if (texture) return Texture();
    int width = 0, height = 0, components = 0;
    auto* pixels = stbi_load_from_memory(logo_video_atlas, sizeof(logo_video_atlas), &width, &height, &components, 4);
    if (!pixels || width != atlasWidth || height != atlasHeight) {
        if (pixels) stbi_image_free(pixels);
        return nullptr;
    }
    GLint binding = 0, unpack = 0, alignment = 0, rowLength = 0, skipRows = 0, skipPixels = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpack);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack);
    glBindTexture(GL_TEXTURE_2D, binding);
    stbi_image_free(pixels);
    return Texture();
}

inline void Release() {
    if (texture && context == eglGetCurrentContext()) glDeleteTextures(1, &texture);
    texture = 0;
    context = EGL_NO_CONTEXT;
}
#else
inline ImTextureID Texture() { return nullptr; }
inline ImTextureID Ensure() { return nullptr; }
inline void Release() {}
#endif
}
