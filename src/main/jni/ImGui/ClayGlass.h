#pragma once
#include "imgui.h"
#include <cmath>
#ifndef CLAY_CPU_PREVIEW
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#endif

namespace clay {
struct Spring {
    float value = 0, velocity = 0;
    void Step(float target, float delta) {
        const float dt = ImClamp(delta, 0.f, .05f);
        const float omega = 24.f;
        const float displacement = value - target;
        const float momentum = velocity + omega * displacement;
        const float decay = std::exp(-omega * dt);
        value = target + (displacement + momentum * dt) * decay;
        velocity = (velocity - omega * momentum * dt) * decay;
    }
};

inline void Surface(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius, bool raised = true) {
    if (!draw || hi.x <= lo.x || hi.y <= lo.y) return;
    draw->AddRectFilled(lo + ImVec2(3,5), hi + ImVec2(3,5), IM_COL32(4,5,12,130), radius + 2);
    draw->AddRectFilled(lo - ImVec2(1,1), hi - ImVec2(1,1), IM_COL32(118,113,150,32), radius);
    draw->AddRectFilled(lo, hi, raised ? IM_COL32(39,38,51,250) : IM_COL32(24,24,34,250), radius);
    draw->AddRect(lo + ImVec2(1,1), hi - ImVec2(1,1), IM_COL32(225,220,255,22), radius, 0, 1);
}

#ifndef CLAY_CPU_PREVIEW
inline GLuint scene = 0, framebuffer = 0;
inline int sceneWidth = 0, sceneHeight = 0;
inline EGLContext sceneContext = EGL_NO_CONTEXT;
inline bool sceneValid = false;
inline void Capture(int width, int height, bool visible) {
    sceneValid = false;
    if (!visible || width <= 0 || height <= 0) return;
    const EGLContext context = eglGetCurrentContext();
    if (context == EGL_NO_CONTEXT) return;
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (samples > 0) return;
    if (context != sceneContext) {
        scene = framebuffer = 0;
        sceneWidth = sceneHeight = 0;
        sceneContext = context;
    }
    GLint read = 0, write = 0, boundTexture = 0, unpackBuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &write);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
    const bool scissor = glIsEnabled(GL_SCISSOR_TEST);
    const float scale = ImMin(1.f, 384.f / static_cast<float>(ImMax(width,height)));
    const int w = ImMax(1, static_cast<int>(width * scale));
    const int h = ImMax(1, static_cast<int>(height * scale));
    if (!scene) glGenTextures(1, &scene);
    if (!framebuffer) glGenFramebuffers(1, &framebuffer);
    glBindTexture(GL_TEXTURE_2D, scene);
    if (sceneWidth != w || sceneHeight != h) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpackBuffer);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        sceneWidth = w; sceneHeight = h;
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,scene,0);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,write);
        glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0,0,width,height,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_LINEAR);
        sceneValid = true;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER,read);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,write);
    glBindTexture(GL_TEXTURE_2D,boundTexture);
    if (scissor) glEnable(GL_SCISSOR_TEST);
}
#endif

inline bool SceneImage(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius,
                       float magnification, float opacity = 1.f) {
#ifndef CLAY_CPU_PREVIEW
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    if (sceneValid && size.x > 0.f && size.y > 0.f) {
        const ImVec2 center = (lo + hi) * .5f;
        const ImVec2 a = center + (lo - center) / magnification;
        const ImVec2 b = center + (hi - center) / magnification;
        draw->AddImageRounded(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(scene)),lo,hi,
            ImVec2(a.x / size.x,1.f-a.y / size.y),ImVec2(b.x / size.x,1.f-b.y / size.y),
            ImGui::GetColorU32(ImVec4(1,1,1,opacity)),radius);
        return true;
    }
#else
    (void)draw; (void)lo; (void)hi; (void)radius;
    (void)magnification; (void)opacity;
#endif
    return false;
}

inline void Backdrop(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius,
                     float tintOpacity = .80f) {
    if (!draw || hi.x <= lo.x || hi.y <= lo.y) return;
    draw->AddRectFilled(lo + ImVec2(0,4),hi + ImVec2(0,4),IM_COL32(5,4,12,110),radius + 1.f);
    const bool sampled = SceneImage(draw,lo,hi,radius,1.018f);
    draw->AddRectFilled(lo,hi,ImGui::GetColorU32(ImVec4(.105f,.098f,.15f,
        sampled ? ImClamp(tintOpacity,0.f,1.f) : 1.f)),radius);
    draw->AddRect(lo,hi,IM_COL32(224,214,255,46),radius,0,1.f);
    draw->AddRect(lo + ImVec2(1,1),hi - ImVec2(1,1),IM_COL32(255,255,255,10),
                  ImMax(1.f,radius - 1.f),0,1.f);
    draw->AddLine(ImVec2(lo.x + radius,lo.y + 1.f),
                  ImVec2(hi.x - radius,lo.y + 1.f),IM_COL32(255,245,255,36),1.f);
}

inline void Glass(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius, float amount) {
    amount = ImClamp(amount, 0.f, 1.f);
    float tintAlpha = 1.f;
    draw->AddRectFilled(lo + ImVec2(0,3), hi + ImVec2(0,3), IM_COL32(0,0,0,65), radius);
    if (amount > .01f && SceneImage(draw,lo,hi,radius,1.12f,amount)) {
        tintAlpha = 1.f - amount * .78f;
    }
    draw->AddRectFilled(lo,hi,ImGui::GetColorU32(ImVec4(.93f,.91f,1.f,tintAlpha)),radius);
    draw->AddRect(lo,hi,ImGui::GetColorU32(ImVec4(1,1,1,.35f+amount*.45f)),radius,0,1.2f);
    draw->AddLine(ImVec2(lo.x+radius*.7f,lo.y+2),ImVec2(hi.x-radius*.7f,lo.y+2),
                  ImGui::GetColorU32(ImVec4(1,1,1,.35f+amount*.4f)),1);
    draw->AddRect(lo+ImVec2(2,2),hi-ImVec2(2,2),IM_COL32(255,255,255,32),ImMax(1.f,radius-2),0,1);
}
}
