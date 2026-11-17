#pragma once
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <algorithm>
#include <cmath>

namespace stretch_screen {

struct Buffer {
    EGLContext context = EGL_NO_CONTEXT;
    GLuint texture = 0;
    GLuint framebuffer = 0;
    int width = 0;
    int height = 0;
};

inline Buffer buffer;

inline float Amount(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 100.0f) : 0.0f;
}

inline int SourceWidth(int width, int height, float amount) {
    if (width <= 0 || height <= 0 || width <= height) return width;
    const int target = std::min(width, std::max(1, static_cast<int>(std::lround(height * (4.0 / 3.0)))));
    const float progress = Amount(amount) / 100.0f;
    return std::clamp(static_cast<int>(std::lround(width + (target - width) * progress)), 1, width);
}

inline float VisualX(float x, int width, int height, float amount) {
    const int source = SourceWidth(width, height, amount);
    if (source <= 0 || source == width) return x;
    return (x - static_cast<float>(width) * 0.5f) * (static_cast<float>(width) / source)
        + static_cast<float>(width) * 0.5f;
}

inline bool Prepare(int width, int height) {
    const EGLContext current = eglGetCurrentContext();
    if (current == EGL_NO_CONTEXT) return false;
    if (buffer.context != current) {
        buffer = {};
        buffer.context = current;
    }
    GLint maximum = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
    if (width > maximum || height > maximum) return false;
    if (!buffer.texture) glGenTextures(1, &buffer.texture);
    if (!buffer.framebuffer) glGenFramebuffers(1, &buffer.framebuffer);
    if (!buffer.texture || !buffer.framebuffer) return false;
    glBindTexture(GL_TEXTURE_2D, buffer.texture);
    if (buffer.width != width || buffer.height != height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer.framebuffer);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.texture, 0);
        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            buffer.width = 0;
            buffer.height = 0;
            return false;
        }
        buffer.width = width;
        buffer.height = height;
    }
    return true;
}

inline bool Apply(int width, int height, float amount) {
    const int sourceWidth = SourceWidth(width, height, amount);
    if (width <= 0 || height <= 0 || sourceWidth == width) return false;

    GLint oldRead = 0;
    GLint oldDraw = 0;
    GLint oldTexture = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldRead);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDraw);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    if (oldDraw != 0) return false;

    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    const bool ready = Prepare(width, height);
    if (ready) {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer.framebuffer);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        const int left = (width - sourceWidth) / 2;
        glBlitFramebuffer(left, 0, left + sourceWidth, height, 0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(oldRead));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(oldDraw));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(oldTexture));
    if (scissor) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
    return ready;
}

inline void RemapOverlay(ImDrawList* draw, int firstVertex, int width, int height, float amount) {
    if (!draw || Amount(amount) <= 0.0f) return;
    const int sourceWidth = SourceWidth(width, height, amount);
    if (sourceWidth == width) return;
    const float center = static_cast<float>(width) * 0.5f;
    const float factor = static_cast<float>(width) / sourceWidth;
    for (int i = std::max(0, firstVertex); i < draw->VtxBuffer.Size; ++i) {
        draw->VtxBuffer[i].pos.x = center + (draw->VtxBuffer[i].pos.x - center) * factor;
    }
}

}
