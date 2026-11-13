#pragma once
#include "imgui.h"
#include <cmath>
#include <array>
#include <chrono>
#include "GlassTiming.h"
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
inline GLuint scene = 0, framebuffer = 0, softScene = 0, softFramebuffer = 0;
inline GLuint blurProgram = 0, liquidProgram = 0, glassVao = 0;
inline int sceneWidth = 0, sceneHeight = 0, sourceWidth = 0, sourceHeight = 0;
inline EGLContext sceneContext = EGL_NO_CONTEXT;
inline bool sceneValid = false, materialValid = false, shaderAttempted = false;
inline CaptureSchedule captureSchedule;
inline unsigned long long captureSerial = 0;
inline char shaderError[512]{};
inline GLint blurSource=-1, blurTexel=-1, lensBackdrop=-1, lensDiffuse=-1, lensLight=-1, lensDisplay=-1, lensBounds=-1, lensTint=-1, lensMaterial=-1;
struct MaterialDraw {
    ImVec2 lo, hi;
    ImVec4 tint;
    float radius, strength, alpha;
};
inline std::array<MaterialDraw, 128> materialDraws{};
inline unsigned materialCount = 0;

inline GLuint Program(const char* vertex, const char* fragment) {
    GLuint shaders[2]{glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER)};
    const char* sources[2]{vertex, fragment};
    bool valid = true;
    for (int i = 0; i < 2; ++i) {
        glShaderSource(shaders[i], 1, &sources[i], nullptr);
        glCompileShader(shaders[i]);
        GLint ready = 0; glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ready);
        if (!ready) { glGetShaderInfoLog(shaders[i], sizeof(shaderError), nullptr, shaderError); valid = false; }
    }
    GLuint program = valid ? glCreateProgram() : 0;
    if (program) {
        glAttachShader(program, shaders[0]); glAttachShader(program, shaders[1]);
        glLinkProgram(program);
        GLint ready = 0; glGetProgramiv(program, GL_LINK_STATUS, &ready);
        if (!ready) { glGetProgramInfoLog(program, sizeof(shaderError), nullptr, shaderError); glDeleteProgram(program); program = 0; }
    }
    glDeleteShader(shaders[0]); glDeleteShader(shaders[1]);
    return program;
}

inline void InitShaders() {
    if (shaderAttempted) return;
    shaderAttempted = true;
    const char* fullscreen = R"(#version 300 es
precision highp float;
out vec2 uv;
void main(){
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    uv=p; gl_Position=vec4(p*2.0-1.0,0.0,1.0);
})";
    const char* blur = R"(#version 300 es
precision highp float;
in vec2 uv; out vec4 color;
uniform sampler2D source;
uniform vec2 texel;
void main(){
    vec3 c=texture(source,uv).rgb*0.28;
    c+=texture(source,uv+vec2(texel.x,texel.y)*1.8).rgb*0.18;
    c+=texture(source,uv+vec2(-texel.x,texel.y)*1.8).rgb*0.18;
    c+=texture(source,uv+vec2(texel.x,-texel.y)*1.8).rgb*0.18;
    c+=texture(source,uv-texel*1.8).rgb*0.18;
    color=vec4(c,1.0);
})";
    const char* lensVertex = R"(#version 300 es
precision highp float;
uniform vec4 bounds;
uniform vec2 display;
out vec2 local;
void main(){
    vec2 p;
    if(gl_VertexID==0) p=vec2(0,0);
    else if(gl_VertexID==1) p=vec2(1,0);
    else if(gl_VertexID==2) p=vec2(0,1);
    else if(gl_VertexID==3) p=vec2(0,1);
    else if(gl_VertexID==4) p=vec2(1,0);
    else p=vec2(1,1);
    local=p*bounds.zw;
    vec2 xy=bounds.xy+local;
    gl_Position=vec4(xy.x/display.x*2.0-1.0,1.0-xy.y/display.y*2.0,0,1);
})";
    const char* lensFragment = R"(#version 300 es
precision highp float;
in vec2 local; out vec4 color;
uniform sampler2D backdrop;
uniform sampler2D diffuse;
uniform vec2 light;
uniform vec2 display;
uniform vec4 bounds;
uniform vec4 tint;
uniform vec4 material;
void main(){
    vec2 halfSize=bounds.zw*0.5;
    float r=min(material.x,min(halfSize.x,halfSize.y));
    vec2 p=local-halfSize;
    vec2 q=abs(p)-halfSize+r;
    vec2 outer=max(q,vec2(0));
    float sd=length(outer)+min(max(q.x,q.y),0.0)-r;
    float coverage=1.0-smoothstep(-0.8,0.8,sd);
    if(coverage<=0.0) discard;
    vec2 normal=length(outer)>0.001 ? normalize(outer)*sign(p) :
        (q.x>q.y?vec2(sign(p.x),0):vec2(0,sign(p.y)));
    float minSide=min(bounds.z,bounds.w);
    float inset=minSide*0.07;
    vec2 innerHalf=halfSize-vec2(inset);
    float innerRadius=min(max(r-inset,2.0*material.w),min(innerHalf.x,innerHalf.y));
    vec2 innerQ=abs(p)-innerHalf+innerRadius;
    float innerSd=length(max(innerQ,vec2(0)))+min(max(innerQ.x,innerQ.y),0.0)-innerRadius;
    float curve=min(12.0*material.w,minSide*0.12);
    float neutral=1.0-smoothstep(-curve*1.5,curve*1.5,innerSd);
    vec2 ramp=(local/bounds.zw-0.5)*(1.0-neutral*0.93);
    float optics=material.y*min(1.0,minSide/(180.0*material.w))*material.w;
    vec2 direction=vec2(ramp.x,-ramp.y)/display;
    vec2 uv=gl_FragCoord.xy/display;
    vec3 refracted;
    refracted.r=texture(backdrop,clamp(uv+direction*(-112.0*optics),vec2(0),vec2(1))).r;
    refracted.g=texture(backdrop,clamp(uv+direction*(-106.0*optics),vec2(0),vec2(1))).g;
    refracted.b=texture(backdrop,clamp(uv+direction*(-100.0*optics),vec2(0),vec2(1))).b;
    vec3 scattered=texture(diffuse,clamp(uv+direction*(-106.0*optics),vec2(0),vec2(1))).rgb;
    refracted=mix(refracted,scattered,0.18);
    float luminance=dot(refracted,vec3(0.2126,0.7152,0.0722));
    refracted=clamp(mix(vec3(luminance),refracted,1.5),vec3(0),vec3(1));
    float tintAlpha=tint.a;
    if(tintAlpha<0.4) tintAlpha*=mix(0.6666667,1.1851852,local.y/bounds.w);
    vec3 base=mix(refracted,tint.rgb,tintAlpha);
    float edge=1.0-neutral;
    float rim=1.0-smoothstep(0.4,2.0,-sd);
    float highlight=pow(max(dot(normal,normalize(light)),0.0),4.0);
    float reflection=pow(max(dot(normal,-normalize(light)),0.0),6.0);
    float insetGlow=exp(-max(0.0,-sd)/max(1.0,8.0*material.w));
    base+=vec3(0.88,0.88,1.0)*rim*(0.12+0.46*highlight);
    base+=vec3(0.74,0.64,1.0)*pow(edge,3.0)*reflection*0.20;
    base+=vec3(0.86,0.82,1.0)*insetGlow*reflection*0.06;
    color=vec4(base,coverage*material.z);
})";
    blurProgram = Program(fullscreen, blur);
    liquidProgram = Program(lensVertex, lensFragment);
    if(blurProgram) {
        blurSource=glGetUniformLocation(blurProgram,"source"); blurTexel=glGetUniformLocation(blurProgram,"texel");
    }
    if(liquidProgram) {
        lensBackdrop=glGetUniformLocation(liquidProgram,"backdrop"); lensDisplay=glGetUniformLocation(liquidProgram,"display");
        lensDiffuse=glGetUniformLocation(liquidProgram,"diffuse"); lensLight=glGetUniformLocation(liquidProgram,"light");
        lensBounds=glGetUniformLocation(liquidProgram,"bounds"); lensTint=glGetUniformLocation(liquidProgram,"tint");
        lensMaterial=glGetUniformLocation(liquidProgram,"material");
    }
    glGenVertexArrays(1, &glassVao);
}

struct SavedGl {
    GLint read, write, active, texture, texture0, sampler0, unpack, program, vao, viewport[4];
    GLboolean scissor, blend, depth, cull, stencil, discard, mask[4];
    SavedGl() {
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read); glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &write);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active); glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glActiveTexture(GL_TEXTURE0); glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0);
        glGetIntegerv(GL_SAMPLER_BINDING, &sampler0);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpack);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program); glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_VIEWPORT, viewport); glGetBooleanv(GL_COLOR_WRITEMASK, mask);
        scissor=glIsEnabled(GL_SCISSOR_TEST); blend=glIsEnabled(GL_BLEND);
        depth=glIsEnabled(GL_DEPTH_TEST); cull=glIsEnabled(GL_CULL_FACE);
        stencil=glIsEnabled(GL_STENCIL_TEST); discard=glIsEnabled(GL_RASTERIZER_DISCARD);
    }
    ~SavedGl() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, read); glBindFramebuffer(GL_DRAW_FRAMEBUFFER, write);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack); glUseProgram(program); glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, texture0); glBindSampler(0, sampler0);
        glActiveTexture(active); glBindTexture(GL_TEXTURE_2D, texture);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glColorMask(mask[0],mask[1],mask[2],mask[3]);
        Restore(GL_SCISSOR_TEST,scissor); Restore(GL_BLEND,blend); Restore(GL_DEPTH_TEST,depth);
        Restore(GL_CULL_FACE,cull); Restore(GL_STENCIL_TEST,stencil); Restore(GL_RASTERIZER_DISCARD,discard);
    }
    static void Restore(GLenum cap, GLboolean enabled) { if(enabled) glEnable(cap); else glDisable(cap); }
};

inline void Allocate(GLuint& texture, GLuint& target, int w, int h) {
    if (!texture) glGenTextures(1, &texture);
    if (!target) glGenFramebuffers(1, &target);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,target);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
}

inline void Capture(int width, int height, bool visible, bool moving = false, double now = -1.0) {
    materialCount = 0;
    if (!visible || width <= 0 || height <= 0) { sceneValid=false; materialValid=false; captureSchedule.Reset(); return; }
    const EGLContext context=eglGetCurrentContext();
    if (context==EGL_NO_CONTEXT) { sceneValid=false; materialValid=false; captureSchedule.Reset(); return; }
    const bool contextChanged=context!=sceneContext;
    if (contextChanged) {
        scene=framebuffer=softScene=softFramebuffer=blurProgram=liquidProgram=glassVao=0;
        sceneWidth=sceneHeight=0; shaderAttempted=false; sceneContext=context;
        sceneValid=false; materialValid=false; captureSchedule.Reset();
    }
    const bool resized=sourceWidth!=width || sourceHeight!=height;
    if (now<0.0) now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (!captureSchedule.Due(now,moving,resized || !sceneValid)) return;
    GLint samples=0; glGetIntegerv(GL_SAMPLES,&samples);
    if (samples>0) { sceneValid=false; materialValid=false; return; }
    SavedGl saved;
    materialValid=false;
    InitShaders();
    const float scale=ImMin(1.f,384.f/static_cast<float>(ImMax(width,height)));
    const int w=ImMax(1,static_cast<int>(width*scale)), h=ImMax(1,static_cast<int>(height*scale));
    if (w!=sceneWidth || h!=sceneHeight) {
        Allocate(scene,framebuffer,w,h);
        Allocate(softScene,softFramebuffer,w,h);
        sceneWidth=w; sceneHeight=h;
    }
    sourceWidth=width; sourceHeight=height;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,framebuffer);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) { sceneValid=false; return; }
    glBindFramebuffer(GL_READ_FRAMEBUFFER,saved.write); glDisable(GL_SCISSOR_TEST);
    glBlitFramebuffer(0,0,width,height,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_LINEAR);
    sceneValid=true; ++captureSerial;
    if (!blurProgram || !liquidProgram) return;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,softFramebuffer);
    if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) return;
    glViewport(0,0,w,h); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); glDisable(GL_STENCIL_TEST); glDisable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glBindSampler(0,0); glBindTexture(GL_TEXTURE_2D,scene);
    glUseProgram(blurProgram); glBindVertexArray(glassVao);
    glUniform1i(blurSource,0);
    glUniform2f(blurTexel,1.f/w,1.f/h);
    glDrawArrays(GL_TRIANGLES,0,3);
    materialValid=true;
}

inline void DrawMaterial(const ImDrawList*, const ImDrawCmd* cmd) {
    const auto& p=*static_cast<const MaterialDraw*>(cmd->UserCallbackData);
    const auto* data=ImGui::GetDrawData();
    if(!data || !materialValid || !liquidProgram || eglGetCurrentContext()!=sceneContext) return;
    const auto scale=data->FramebufferScale;
    const int width=static_cast<int>(data->DisplaySize.x*scale.x), height=static_cast<int>(data->DisplaySize.y*scale.y);
    if(width<=0 || height<=0) return;
    const ImVec2 clipMin=(ImVec2(cmd->ClipRect.x,cmd->ClipRect.y)-data->DisplayPos)*scale;
    const ImVec2 clipMax=(ImVec2(cmd->ClipRect.z,cmd->ClipRect.w)-data->DisplayPos)*scale;
    if(clipMax.x<=clipMin.x || clipMax.y<=clipMin.y) return;
    glScissor(ImMax(0,static_cast<int>(clipMin.x)),ImMax(0,height-static_cast<int>(clipMax.y)),
        ImMax(0,static_cast<int>(clipMax.x-clipMin.x)),ImMax(0,static_cast<int>(clipMax.y-clipMin.y)));
    glUseProgram(liquidProgram); glBindVertexArray(glassVao);
    glActiveTexture(GL_TEXTURE1);
    GLint previousTexture1=0,previousSampler1=0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&previousTexture1); glGetIntegerv(GL_SAMPLER_BINDING,&previousSampler1);
    glBindSampler(1,0); glBindTexture(GL_TEXTURE_2D,softScene);
    glActiveTexture(GL_TEXTURE0);
    GLint previousSampler=0; glGetIntegerv(GL_SAMPLER_BINDING,&previousSampler);
    glBindSampler(0,0); glBindTexture(GL_TEXTURE_2D,scene);
    const auto lo=(p.lo-data->DisplayPos)*scale, size=(p.hi-p.lo)*scale;
    glUniform1i(lensBackdrop,0);
    glUniform1i(lensDiffuse,1);
    const auto pointer=ImGui::GetIO().MousePos;
    const bool hasPointer=pointer.x>-10000.f && pointer.y>-10000.f;
    const auto light=hasPointer?ImVec2(ImClamp((pointer.x-(p.lo.x+p.hi.x)*.5f)/ImMax(1.f,p.hi.x-p.lo.x),-.5f,.5f)-.45f,
        ImClamp((pointer.y-(p.lo.y+p.hi.y)*.5f)/ImMax(1.f,p.hi.y-p.lo.y),-.5f,.5f)-.9f):ImVec2(-.45f,-.9f);
    glUniform2f(lensLight,light.x,light.y);
    glUniform2f(lensDisplay,static_cast<float>(width),static_cast<float>(height));
    glUniform4f(lensBounds,lo.x,lo.y,size.x,size.y);
    glUniform4f(lensTint,p.tint.x,p.tint.y,p.tint.z,p.tint.w);
    glUniform4f(lensMaterial,p.radius*scale.x,p.strength,p.alpha,scale.x);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindSampler(0,previousSampler);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,previousTexture1); glBindSampler(1,previousSampler1);
    glActiveTexture(GL_TEXTURE0);
}
#endif

inline bool Liquid(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius, ImVec4 tint,
                   float strength=1.f, float opacity=1.f) {
    if(!draw || hi.x<=lo.x || hi.y<=lo.y) return false;
#ifndef CLAY_CPU_PREVIEW
    if(materialValid && liquidProgram && materialCount<materialDraws.size()) {
        auto& p=materialDraws[materialCount++];
        p={lo,hi,tint,radius,strength,ImGui::GetStyle().Alpha*opacity};
        if(draw->VtxBuffer.Size==0) draw->AddRectFilled(lo,lo,IM_COL32_WHITE);
        draw->AddCallback(DrawMaterial,&p);
        draw->AddCallback(ImDrawCallback_ResetRenderState,nullptr);
        return true;
    }
#endif
    draw->AddRectFilled(lo,hi,ImGui::GetColorU32(ImVec4(tint.x,tint.y,tint.z,opacity)),radius);
    return false;
}

inline bool SceneImage(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius,
                       float magnification, float opacity = 1.f) {
    return Liquid(draw,lo,hi,radius,ImVec4(.07f,.06f,.12f,.22f),magnification,opacity);
}

inline void Backdrop(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius,
                     float tintOpacity=.76f) {
    if(!draw || hi.x<=lo.x || hi.y<=lo.y) return;
    draw->AddRectFilled(lo+ImVec2(0,4),hi+ImVec2(0,4),IM_COL32(4,3,10,85),radius);
    if(tintOpacity<=.78f) Liquid(draw,lo,hi,radius,ImVec4(.055f,.045f,.095f,.25f),1.2f);
    else draw->AddRectFilled(lo,hi,ImGui::GetColorU32(ImVec4(.09f,.075f,.125f,tintOpacity*.64f)),radius);
    draw->AddRect(lo,hi,ImGui::GetColorU32(ImVec4(.86f,.80f,1.f,.13f)),radius,0,1.f);
}

inline void Glass(ImDrawList* draw, ImVec2 lo, ImVec2 hi, float radius, float amount) {
    amount=ImClamp(amount,0.f,1.f);
    if(amount>.01f) Liquid(draw,lo,hi,radius,ImVec4(.93f,.91f,1.f,1.f-amount*.84f),amount*1.3f);
    else draw->AddRectFilled(lo,hi,ImGui::GetColorU32(ImVec4(.93f,.91f,1.f,1.f)),radius);
    draw->AddRect(lo,hi,ImGui::GetColorU32(ImVec4(1,1,1,.30f+amount*.32f)),radius,0,1.f);
}
}
