#include "FrostedSidebar.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <cmath>

namespace lite_blur {
namespace {
struct SavedState {
    GLint read, draw, viewport[4], program, vao, active, texture, sampler, unpack;
    GLboolean color[4], blend, depth, stencil, cull, scissor, discard;
    SavedState() {
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);
        glGetIntegerv(GL_VIEWPORT,viewport);
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
        glGetIntegerv(GL_SAMPLER_BINDING,&sampler);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);
        glGetBooleanv(GL_COLOR_WRITEMASK,color);
        blend=glIsEnabled(GL_BLEND); depth=glIsEnabled(GL_DEPTH_TEST); stencil=glIsEnabled(GL_STENCIL_TEST);
        cull=glIsEnabled(GL_CULL_FACE); scissor=glIsEnabled(GL_SCISSOR_TEST); discard=glIsEnabled(GL_RASTERIZER_DISCARD);
    }
    ~SavedState() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,read);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glUseProgram(program);
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texture);
        glBindSampler(0,sampler);
        glActiveTexture(active);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);
        glColorMask(color[0],color[1],color[2],color[3]);
        Set(GL_BLEND,blend); Set(GL_DEPTH_TEST,depth); Set(GL_STENCIL_TEST,stencil);
        Set(GL_CULL_FACE,cull); Set(GL_SCISSOR_TEST,scissor); Set(GL_RASTERIZER_DISCARD,discard);
    }
    static void Set(GLenum state, bool enabled) { if (enabled) glEnable(state); else glDisable(state); }
};
struct Resources {
    EGLContext context = EGL_NO_CONTEXT;
    GLuint textures[2]{}, framebuffers[2]{}, program = 0, vao = 0;
    GLint direction = -1;
    int width = 0, height = 0, sourceX = 0, sourceY = 0, sourceWidth = 0, sourceHeight = 0;
    GLint sourceFramebuffer = -1;
    double next = 0;
    bool failed = false, valid = false;
    Statistics statistics;
} resources;
GLuint Shader(GLenum type, const char* text) {
    const auto shader=glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader,1,&text,nullptr); glCompileShader(shader);
    GLint ok=0; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if (!ok) { glDeleteShader(shader); return 0; }
    return shader;
}
bool Program() {
    if (resources.program) return true;
    const char* vertex=R"(#version 300 es
out vec2 uv;
void main() {
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    uv=p; gl_Position=vec4(p*2.0-1.0,0,1);
})";
    const char* fragment=R"(#version 300 es
precision mediump float;
in vec2 uv;
uniform sampler2D source;
uniform vec2 direction;
out vec4 color;
void main() {
    vec3 value=texture(source,uv).rgb*0.2270270270;
    value+=texture(source,uv+direction*1.3846153846).rgb*0.3162162162;
    value+=texture(source,uv-direction*1.3846153846).rgb*0.3162162162;
    value+=texture(source,uv+direction*3.2307692308).rgb*0.0702702703;
    value+=texture(source,uv-direction*3.2307692308).rgb*0.0702702703;
    color=vec4(value,1);
})";
    const auto vs=Shader(GL_VERTEX_SHADER,vertex), fs=Shader(GL_FRAGMENT_SHADER,fragment);
    if (!vs || !fs) { if(vs) glDeleteShader(vs); if(fs) glDeleteShader(fs); return false; }
    const auto program=glCreateProgram();
    if (!program) { glDeleteShader(vs); glDeleteShader(fs); return false; }
    glAttachShader(program,vs); glAttachShader(program,fs); glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint ok=0; glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if (!ok) { glDeleteProgram(program); return false; }
    resources.program=program;
    resources.direction=glGetUniformLocation(program,"direction");
    glUseProgram(program); glUniform1i(glGetUniformLocation(program,"source"),0);
    glGenVertexArrays(1,&resources.vao);
    return resources.vao!=0;
}
bool Targets(int width, int height) {
    if (resources.width==width && resources.height==height && resources.textures[0]) return true;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    if (!resources.textures[0]) glGenTextures(2,resources.textures);
    if (!resources.framebuffers[0]) glGenFramebuffers(2,resources.framebuffers);
    if (!resources.textures[0] || !resources.textures[1] || !resources.framebuffers[0] || !resources.framebuffers[1]) return false;
    for (int index=0;index<2;++index) {
        glBindTexture(GL_TEXTURE_2D,resources.textures[index]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER,resources.framebuffers[index]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,resources.textures[index],0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) return false;
    }
    resources.width=width; resources.height=height;
    resources.statistics.textureBytes=std::size_t(width)*height*8;
    return true;
}
}
Texture Capture(Region region, int framebufferWidth, int framebufferHeight, double now) {
    const auto context=eglGetCurrentContext();
    if (context==EGL_NO_CONTEXT || framebufferWidth<=0 || framebufferHeight<=0 || !std::isfinite(now) ||
        !std::isfinite(region.x) || !std::isfinite(region.y) || !std::isfinite(region.width) || !std::isfinite(region.height) ||
        region.width<=0 || region.height<=0) return {};
    if (context!=resources.context) { resources={}; resources.context=context; }
    if (resources.failed) return {};
    const int x=int(std::floor(std::clamp(double(region.x),0.,double(framebufferWidth))));
    const int y=int(std::floor(std::clamp(double(framebufferHeight)-region.y-region.height,0.,double(framebufferHeight))));
    const int right=int(std::ceil(std::clamp(double(region.x)+region.width,double(x),double(framebufferWidth))));
    const int top=int(std::ceil(std::clamp(double(framebufferHeight)-region.y,double(y),double(framebufferHeight))));
    const int sourceWidth=right-x, sourceHeight=top-y;
    if (!sourceWidth || !sourceHeight) return {};
    const float scale=std::min(float(maximumWidth)/sourceWidth,float(maximumHeight)/sourceHeight);
    const int width=std::max(1,int(std::floor(sourceWidth*std::min(scale,1.f))));
    const int height=std::max(1,int(std::floor(sourceHeight*std::min(scale,1.f))));
    GLint source=0, samples=0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&source);
    glGetIntegerv(GL_SAMPLES,&samples);
    if (samples>1) return {};
    const bool moved=x!=resources.sourceX || y!=resources.sourceY || sourceWidth!=resources.sourceWidth ||
        sourceHeight!=resources.sourceHeight || source!=resources.sourceFramebuffer;
    if (!moved && resources.valid && now<resources.next && now>=resources.next-refreshSeconds)
        return {resources.textures[0],resources.width,resources.height};
    SavedState saved;
    glBindFramebuffer(GL_READ_FRAMEBUFFER,source);
    GLint readBuffer=0; glGetIntegerv(GL_READ_BUFFER,&readBuffer);
    if (readBuffer==GL_NONE || glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) return {};
    if (!Program() || !Targets(width,height)) { resources.failed=true; return {}; }
    glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST); glDisable(GL_CULL_FACE); glDisable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,source);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,resources.framebuffers[0]);
    glBlitFramebuffer(x,y,right,top,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_LINEAR);
    glViewport(0,0,width,height);
    glUseProgram(resources.program); glBindVertexArray(resources.vao); glBindSampler(0,0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,resources.framebuffers[1]);
    glBindTexture(GL_TEXTURE_2D,resources.textures[0]);
    glUniform2f(resources.direction,1.f/width,0); glDrawArrays(GL_TRIANGLES,0,3);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,resources.framebuffers[0]);
    glBindTexture(GL_TEXTURE_2D,resources.textures[1]);
    glUniform2f(resources.direction,0,1.f/height); glDrawArrays(GL_TRIANGLES,0,3);
    resources.next=now+refreshSeconds; resources.valid=true;
    resources.sourceX=x; resources.sourceY=y; resources.sourceWidth=sourceWidth; resources.sourceHeight=sourceHeight;
    resources.sourceFramebuffer=source; ++resources.statistics.captures;
    return {resources.textures[0],width,height};
}
Statistics GetStatistics() { return resources.statistics; }
void Release() {
    if (eglGetCurrentContext()==resources.context && resources.context!=EGL_NO_CONTEXT) {
        glDeleteTextures(2,resources.textures); glDeleteFramebuffers(2,resources.framebuffers);
        if (resources.vao) glDeleteVertexArrays(1,&resources.vao);
        if (resources.program) glDeleteProgram(resources.program);
    }
    resources={};
}
}
