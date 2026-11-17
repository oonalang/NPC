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
    GLuint program = 0;
    GLuint vao = 0;
    GLint sampler = -1;
    GLint ratio = -1;
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

inline GLuint Compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

inline bool CompileProgram() {
    if (buffer.program && buffer.vao) return true;
    const char* vertex = R"GLSL(#version 300 es
precision highp float;
out vec2 coord;
void main() {
    vec2 point = vec2(gl_VertexID == 1 ? 2.0 : 0.0,
                      gl_VertexID == 2 ? 2.0 : 0.0);
    coord = point;
    gl_Position = vec4(point * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";
    const char* fragment = R"GLSL(#version 300 es
precision highp float;
in vec2 coord;
uniform sampler2D frameTexture;
uniform float sourceRatio;
out vec4 fragColor;
void main() {
    float u = 0.5 + (coord.x - 0.5) * sourceRatio;
    fragColor = texture(frameTexture, vec2(u, coord.y));
}
)GLSL";
    const GLuint vs = Compile(GL_VERTEX_SHADER, vertex);
    const GLuint fs = Compile(GL_FRAGMENT_SHADER, fragment);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        return false;
    }
    buffer.sampler = glGetUniformLocation(program, "frameTexture");
    buffer.ratio = glGetUniformLocation(program, "sourceRatio");
    if (buffer.sampler < 0 || buffer.ratio < 0) {
        glDeleteProgram(program);
        return false;
    }
    if (!buffer.vao) glGenVertexArrays(1, &buffer.vao);
    if (!buffer.vao) {
        glDeleteProgram(program);
        return false;
    }
    buffer.program = program;
    return true;
}

struct Saved {
    GLint read = 0;
    GLint draw = 0;
    GLint active = GL_TEXTURE0;
    GLint texture = 0;
    GLint texture0 = 0;
    GLint sampler0 = 0;
    GLint program = 0;
    GLint vao = 0;
    GLint viewport[4] = {};
    GLboolean mask[4] = {};
    GLboolean scissor, blend, depth, cull, stencil, discard;

    Saved() {
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0);
        glGetIntegerv(GL_SAMPLER_BINDING, &sampler0);
        glActiveTexture(active);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetBooleanv(GL_COLOR_WRITEMASK, mask);
        scissor = glIsEnabled(GL_SCISSOR_TEST);
        blend = glIsEnabled(GL_BLEND);
        depth = glIsEnabled(GL_DEPTH_TEST);
        cull = glIsEnabled(GL_CULL_FACE);
        stencil = glIsEnabled(GL_STENCIL_TEST);
        discard = glIsEnabled(GL_RASTERIZER_DISCARD);
    }

    static void Restore(GLenum cap, GLboolean enabled) {
        if (enabled) glEnable(cap);
        else glDisable(cap);
    }

    ~Saved() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
        glUseProgram(static_cast<GLuint>(program));
        glBindVertexArray(static_cast<GLuint>(vao));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture0));
        glBindSampler(0, static_cast<GLuint>(sampler0));
        glActiveTexture(active);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glColorMask(mask[0], mask[1], mask[2], mask[3]);
        Restore(GL_SCISSOR_TEST, scissor);
        Restore(GL_BLEND, blend);
        Restore(GL_DEPTH_TEST, depth);
        Restore(GL_CULL_FACE, cull);
        Restore(GL_STENCIL_TEST, stencil);
        Restore(GL_RASTERIZER_DISCARD, discard);
    }
};

inline bool Prepare(int width, int height) {
    const EGLContext context = eglGetCurrentContext();
    if (context == EGL_NO_CONTEXT) return false;
    if (buffer.context != context) {
        buffer = {};
        buffer.context = context;
    }
    GLint maximum = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
    if (width > maximum || height > maximum || !CompileProgram()) return false;
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
    Saved saved;
    if (saved.draw != 0 || !Prepare(width, height)) return false;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer.framebuffer);
    glDisable(GL_SCISSOR_TEST);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, buffer.texture);
    glBindSampler(0, 0);
    glUseProgram(buffer.program);
    glUniform1i(buffer.sampler, 0);
    glUniform1f(buffer.ratio, static_cast<float>(sourceWidth) / static_cast<float>(width));
    glBindVertexArray(buffer.vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    return true;
}

inline void RemapOverlay(ImDrawList* draw, int firstVertex, int width, int height, float amount) {
    if (!draw || Amount(amount) <= 0.0f) return;
    const int sourceWidth = SourceWidth(width, height, amount);
    if (sourceWidth == width) return;
    const float center = static_cast<float>(width) * 0.5f;
    const float factor = static_cast<float>(width) / sourceWidth;
    for (int i = std::max(0, firstVertex); i < draw->VtxBuffer.Size; ++i)
        draw->VtxBuffer[i].pos.x = center + (draw->VtxBuffer[i].pos.x - center) * factor;
}

}
