#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/imgui_impl_opengl3.h"
#include <algorithm>
#include <cstddef>
#include <optional>

namespace auth_scene {
enum class Mode { Card, Suck, Singularity, Flash, Explosion, Empty };
struct Frame {
    Mode mode=Mode::Card;
    float progress=0.f;
    ImVec2 center={.5f,.5f};
};

#ifndef CLAY_CPU_PREVIEW
struct SavedState {
    EGLContext context=eglGetCurrentContext();
    GLint read=0,write=0,active=0,texture=0,texture0=0,texture1=0,sampler0=0,sampler1=0;
    GLint unpack=0,program=0,vao=0,viewport[4]{},scissorBox[4]{};
    GLfloat clear[4]{};
    GLboolean scissor=0,blend=0,depth=0,cull=0,stencil=0,discard=0,dither=0,coverage=0,alphaCoverage=0,mask[4]{};
    SavedState() {
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&write);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture0);
        glGetIntegerv(GL_SAMPLER_BINDING,&sampler0);
        glActiveTexture(GL_TEXTURE1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture1);
        glGetIntegerv(GL_SAMPLER_BINDING,&sampler1);
        glActiveTexture(active);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_VIEWPORT,viewport);
        glGetIntegerv(GL_SCISSOR_BOX,scissorBox);
        glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);
        glGetBooleanv(GL_COLOR_WRITEMASK,mask);
        scissor=glIsEnabled(GL_SCISSOR_TEST); blend=glIsEnabled(GL_BLEND);
        depth=glIsEnabled(GL_DEPTH_TEST); cull=glIsEnabled(GL_CULL_FACE);
        stencil=glIsEnabled(GL_STENCIL_TEST); discard=glIsEnabled(GL_RASTERIZER_DISCARD);
        dither=glIsEnabled(GL_DITHER); coverage=glIsEnabled(GL_SAMPLE_COVERAGE);
        alphaCoverage=glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    void Restore() const {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,read);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,write);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);
        glUseProgram(program); glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texture0); glBindSampler(0,sampler0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,texture1); glBindSampler(1,sampler1);
        glActiveTexture(active); glBindTexture(GL_TEXTURE_2D,texture);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glScissor(scissorBox[0],scissorBox[1],scissorBox[2],scissorBox[3]);
        glClearColor(clear[0],clear[1],clear[2],clear[3]);
        glColorMask(mask[0],mask[1],mask[2],mask[3]);
        Set(GL_SCISSOR_TEST,scissor); Set(GL_BLEND,blend); Set(GL_DEPTH_TEST,depth);
        Set(GL_CULL_FACE,cull); Set(GL_STENCIL_TEST,stencil); Set(GL_RASTERIZER_DISCARD,discard);
        Set(GL_DITHER,dither); Set(GL_SAMPLE_COVERAGE,coverage); Set(GL_SAMPLE_ALPHA_TO_COVERAGE,alphaCoverage);
    }
    static void Set(GLenum cap,GLboolean enabled) { if(enabled) glEnable(cap); else glDisable(cap); }
};
inline std::optional<SavedState> saved;
inline EGLContext context=EGL_NO_CONTEXT;
inline GLuint baseTexture=0,baseFramebuffer=0,uiTexture=0,uiFramebuffer=0;
inline GLuint checkerProgram=0,compositeProgram=0,vao=0;
inline GLint checkerDisplay=-1,sceneSource=-1,uiSource=-1,sceneDisplay=-1,sceneMode=-1,sceneProgress=-1,sceneCenter=-1;
inline int width=0,height=0;
inline bool active=false,attempted=false,baseReady=false;

inline bool Active() { return active; }
inline std::size_t TextureBytes() { return static_cast<std::size_t>(width)*height*8; }
inline void Abort() {
    if(saved && eglGetCurrentContext()==saved->context) saved->Restore();
    saved.reset(); active=false;
}
inline void Release() {
    Abort();
    if(context!=EGL_NO_CONTEXT && context==eglGetCurrentContext()) {
        glDeleteTextures(1,&baseTexture); glDeleteTextures(1,&uiTexture);
        glDeleteFramebuffers(1,&baseFramebuffer); glDeleteFramebuffers(1,&uiFramebuffer);
        if(checkerProgram) glDeleteProgram(checkerProgram);
        if(compositeProgram) glDeleteProgram(compositeProgram);
        glDeleteVertexArrays(1,&vao);
    }
    baseTexture=baseFramebuffer=uiTexture=uiFramebuffer=checkerProgram=compositeProgram=vao=0;
    width=height=0;attempted=baseReady=false;context=EGL_NO_CONTEXT;
}
inline void RasterState(int w,int h) {
    glViewport(0,0,w,h);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); glDisable(GL_STENCIL_TEST); glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_DITHER); glDisable(GL_SAMPLE_COVERAGE); glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
}
inline bool Init() {
    if(checkerProgram && compositeProgram && vao) return true;
    if(attempted) return false;
    attempted=true;
    const char* vertex=R"(#version 300 es
precision highp float;
out vec2 uv;
void main(){
    vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));
    uv=p;gl_Position=vec4(p*2.0-1.0,0.0,1.0);
})";
    const char* checker=R"(#version 300 es
precision highp float;
in vec2 uv;out vec4 color;
uniform vec2 display;
void main(){
    float cell=max(1.0,min(display.x,display.y)/9.0);
    vec2 pixel=vec2(gl_FragCoord.x,display.y-gl_FragCoord.y);
    float alternate=1.0-mod(floor(pixel.x/cell)+floor(pixel.y/cell),2.0);
    vec3 lowBottom=mix(vec3(10,18,31),vec3(23,23,51),uv.x)/255.0;
    vec3 lowTop=mix(vec3(11,31,38),vec3(19,22,43),uv.x)/255.0;
    vec3 highBottom=mix(vec3(29,39,73),vec3(54,40,105),uv.x)/255.0;
    vec3 highTop=mix(vec3(22,83,85),vec3(47,66,104),uv.x)/255.0;
    vec3 low=mix(lowBottom,lowTop,uv.y);
    vec3 high=mix(highBottom,highTop,uv.y);
    color=vec4(mix(low,high,alternate),1.0);
})";
    const char* composite=R"(#version 300 es
precision highp float;
in vec2 uv;out vec4 color;
uniform sampler2D sceneSource;
uniform sampler2D uiSource;
uniform vec2 display;
uniform int mode;
uniform float progress;
uniform vec2 center;
vec2 rotatePoint(vec2 p,float angle){
    float s=sin(angle),c=cos(angle);return vec2(c*p.x-s*p.y,s*p.x+c*p.y);
}
vec3 scene(vec2 p){return texture(sceneSource,clamp(p,.5/display,vec2(1)-.5/display)).rgb;}
vec2 boundedShift(vec2 at,vec2 shift){
    vec2 margin=min(at,vec2(1)-at)*display;
    vec2 boundary=smoothstep(vec2(0),vec2(36.0),max(vec2(0),margin));
    vec2 pixels=shift*display*boundary;
    vec2 room=max(vec2(0),margin-vec2(.5));
    return pixels*room/(room+abs(pixels)+vec2(.0001))/display;
}
float bell(float x){return exp(-x*x);}
vec3 singularity(vec3 base,float radius,float phase){
    float unit=min(display.x,display.y);
    float d=radius*unit,pixelScale=unit/576.0;
    float pulse=.9+.1*sin(phase*6.2831853);
    float halo=exp(-d*d/(600.0*pixelScale*pixelScale))*pulse;
    float rim=bell((d-19.0*pixelScale)/(2.3*pixelScale));
    float dot=1.0-smoothstep(13.0*pixelScale,16.0*pixelScale,d);
    base+=vec3(.29,.08,.49)*halo;
    base=mix(base,vec3(.006,.003,.014),dot);
    return base+vec3(.87,.65,1.0)*rim;
}
void main(){
    float p=clamp(progress,0.0,1.0);
    vec3 base=scene(uv);
    if(mode==5 || (mode==4 && p>=1.0)){color=vec4(base,1);return;}
    if(mode==0 || mode==1){
        vec2 sampleUV=uv;
        float opacity=1.0;
        if(mode==1){
            float collapse=p*p*(3.0-2.0*p);
            float scale=max(.0001,pow(1.0-p,2.4));
            vec2 delta=(uv-center)*display/scale;
            vec2 local=delta/min(display.x,display.y);
            float twist=collapse*(2.2+2.8/(1.0+dot(local,local)*5.0));
            sampleUV=center+rotatePoint(delta,twist)/display;
            opacity=1.0-smoothstep(.84,1.0,p);
        }
        vec4 foreground=vec4(0);
        if(all(greaterThanEqual(sampleUV,vec2(0)))&&all(lessThanEqual(sampleUV,vec2(1))))
            foreground=texture(uiSource,sampleUV)*opacity;
        color=vec4(foreground.rgb+base*(1.0-foreground.a),1);return;
    }
    float unit=min(display.x,display.y);
    vec2 q=(uv-center)*display/unit;
    float r=length(q);
    if(mode==2){
        color=vec4(singularity(base,r,p),1);return;
    }
    if(mode==3){
        base=mix(base,singularity(base,r,1.0),(1.0-p)*(1.0-p));
        float hot=p*p*(3.0-2.0*p);
        float core=exp(-r*r/(.00006+.00030*hot));
        float glow=exp(-r*r/(.0008+.0030*hot));
        float streak=exp(-q.y*q.y/.000012)*exp(-q.x*q.x/.008)*hot;
        color=vec4(base+vec3(.60,.32,.95)*glow*.7*hot+vec3(1.0)*(core*2.8+streak*.65)*hot,1);return;
    }
    float energy=1.0-smoothstep(.64,1.0,p);
    float maximum=max(max(length(center*display/unit),length((vec2(1,0)-center)*display/unit)),
                      max(length((vec2(0,1)-center)*display/unit),length((vec2(1)-center)*display/unit)));
    float radius=mix(.022,maximum*1.18,pow(p,.78));
    float a=r>.00001?atan(q.y,q.x):0.0;
    float lumps=.105*sin(3.0*a+p*2.16)+.064*sin(5.0*a-p*2.88+.7)
               +.035*sin(8.0*a+p*3.78+1.9)+.021*sin(13.0*a-p*5.76)
               +.018*sin(19.0*a+p*6.84)+.012*sin(27.0*a-p*8.1)
               +.006*sin(37.0*a+p*11.7);
    float frontRadius=radius*(1.0+lumps);
    float variation=.5+.5*sin(7.0*a+p*4.32+.45*sin(11.0*a-p*1.8));
    float fine=.5+.5*sin(43.0*a+p*12.6+.5*sin(79.0*a-p*18.2));
    float thickness=(.013+.024*variation)*(.82+.18*fine);
    float distance=r-frontRadius;
    float band=distance/thickness;
    float shock=exp(-.5*band*band);
    float front=exp(-band*band);
    float inside=1.0-smoothstep(-thickness,thickness,distance);
    vec2 normal=vec2(cos(a),sin(a)),tangent=vec2(-normal.y,normal.x);
    float bend=-.052*band*shock+.007*sin(band*3.0)*exp(-abs(band)*.4);
    vec2 swirl=tangent*.013*sin(r*12.0-a*2.0-p*5.04)
              +vec2(sin(q.y*14.0+p*3.6),cos(q.x*11.0-p*3.24))*.006;
    float envelope=1.0-smoothstep(4.0,7.0,abs(band));
    vec2 shift=(normal*bend*envelope+swirl*inside)*energy;
    vec2 sampleUV=uv+boundedShift(uv,shift*unit/display);
    float split=(.004+.009*variation)*front*energy;
    vec2 prism=boundedShift(sampleUV,normal*split*unit/display);
    vec3 refracted=scene(sampleUV);
    if(front>.001) refracted=vec3(scene(sampleUV+prism).r,refracted.g,scene(sampleUV-prism).b);
    refracted=mix(refracted,vec3(.42,.19,.08),inside*.025*energy);
    float centerFlash=exp(-r*r/.0012)*pow(1.0-p,12.0);
    refracted+=vec3(1.0,.90,.98)*centerFlash*1.2;
    float flicker=.5+.5*sin(31.0*a+p*8.7+.4*sin(13.0*a-p*5.6));
    float brightness=(.35+.85*variation)*(.75+.25*flicker)*(.72+.28*fine);
    float spectralWidth=thickness*(.32+.12*flicker);
    float spread=thickness*(.38+.24*variation);
    vec3 spectrum=(vec3(distance)+vec3(spread,0.0,-spread))/spectralWidth;
    vec3 spectral=exp(-spectrum*spectrum);
    float leading=bell((distance-.004)/(thickness*.32));
    float leadingGlow=bell((distance-.007)/(thickness*1.50));
    float trailing=bell((distance+thickness*.75)/(thickness*1.10));
    float trailingGlow=bell((distance+thickness*1.20)/(thickness*2.15));
    refracted+=spectral*brightness*energy*.50;
    refracted+=vec3(.68,.84,1.0)*leading*brightness*energy*.85;
    refracted+=vec3(.24,.32,.48)*leadingGlow*brightness*energy*.26;
    refracted+=vec3(1.0,.48,.065)*trailing*brightness*energy*.48;
    refracted+=vec3(1.0,.42,.04)*trailingGlow*brightness*energy*.16;
    color=vec4(refracted,1);
})";
    checkerProgram=clay::Program(vertex,checker);
    compositeProgram=clay::Program(vertex,composite);
    if(!checkerProgram || !compositeProgram) {
        if(checkerProgram) glDeleteProgram(checkerProgram);
        if(compositeProgram) glDeleteProgram(compositeProgram);
        checkerProgram=compositeProgram=0;
        return false;
    }
    checkerDisplay=glGetUniformLocation(checkerProgram,"display");
    sceneSource=glGetUniformLocation(compositeProgram,"sceneSource");
    uiSource=glGetUniformLocation(compositeProgram,"uiSource");
    sceneDisplay=glGetUniformLocation(compositeProgram,"display");
    sceneMode=glGetUniformLocation(compositeProgram,"mode");
    sceneProgress=glGetUniformLocation(compositeProgram,"progress");
    sceneCenter=glGetUniformLocation(compositeProgram,"center");
    glGenVertexArrays(1,&vao);
    return vao!=0;
}
inline bool Allocate(GLuint& texture,GLuint& target,int w,int h) {
    if(!texture) glGenTextures(1,&texture);
    if(!target) glGenFramebuffers(1,&target);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texture); glBindSampler(0,0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,target);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    return glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
}
inline bool Begin(int w,int h,bool liveBackdrop=false) {
    if(active) Abort();
    const auto current=eglGetCurrentContext();
    if(current==EGL_NO_CONTEXT || w<=0 || h<=0) return false;
    saved.emplace();
    if(context!=current) {
        context=current;
        baseTexture=baseFramebuffer=uiTexture=uiFramebuffer=checkerProgram=compositeProgram=vao=0;
        width=height=0;attempted=baseReady=false;
    }
    if(!Init()) { Abort();return false; }
    if(w!=width || h!=height) {
        if(!Allocate(baseTexture,baseFramebuffer,w,h) || !Allocate(uiTexture,uiFramebuffer,w,h)) {
            Abort();return false;
        }
        width=w;height=h;
        baseReady=false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,baseFramebuffer);
    RasterState(w,h);
    if(liveBackdrop) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,saved->write);
        glBlitFramebuffer(0,0,w,h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        baseReady=false;
    } else if(!baseReady) {
        glUseProgram(checkerProgram); glBindVertexArray(vao);
        glUniform2f(checkerDisplay,static_cast<float>(w),static_cast<float>(h));
        glDrawArrays(GL_TRIANGLES,0,3);
        baseReady=true;
    }
    active=true;
    return true;
}
inline bool Render(ImDrawData* data,const Frame& frame={}) {
    if(!active || !saved || context!=eglGetCurrentContext()) { Abort();return false; }
    if(frame.mode==Mode::Card || frame.mode==Mode::Suck) {
        glBindFramebuffer(GL_FRAMEBUFFER,uiFramebuffer);
        RasterState(width,height);
        glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0); glBindSampler(0,0);
        if(data && data->Valid) ImGui_ImplOpenGL3_RenderDrawData(data);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,saved->write);
    RasterState(width,height);
    glUseProgram(compositeProgram); glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0); glBindSampler(0,0); glBindTexture(GL_TEXTURE_2D,baseTexture);
    glActiveTexture(GL_TEXTURE1); glBindSampler(1,0); glBindTexture(GL_TEXTURE_2D,uiTexture);
    glUniform1i(sceneSource,0); glUniform1i(uiSource,1);
    glUniform2f(sceneDisplay,static_cast<float>(width),static_cast<float>(height));
    glUniform1i(sceneMode,static_cast<int>(frame.mode));
    glUniform1f(sceneProgress,std::isfinite(frame.progress)?std::clamp(frame.progress,0.f,1.f):0.f);
    const float cx=std::isfinite(frame.center.x)?std::clamp(frame.center.x,0.f,1.f):.5f;
    const float cy=std::isfinite(frame.center.y)?std::clamp(frame.center.y,0.f,1.f):.5f;
    glUniform2f(sceneCenter,cx,1.f-cy);
    glDrawArrays(GL_TRIANGLES,0,3);
    Abort();
    return true;
}
#else
inline bool Active() { return false; }
inline std::size_t TextureBytes() { return 0; }
inline void Abort() {}
inline void Release() {}
inline bool Begin(int,int,bool=false) { return false; }
inline bool Render(ImDrawData*,const Frame& = {}) { return false; }
#endif
}
