#define IMGUI_DEFINE_MATH_OPERATORS
#include "../src/main/jni/ImGui/imgui_internal.h"
#include "../src/main/jni/ImGui/ClayGlass.h"
#include "../src/main/jni/System/UI/StartupMedia.h"
#include <EGL/eglext.h>
#include "../src/main/jni/ImGui/imgui_impl_opengl3.h"
#include <cassert>
#include <cstdio>
#include <array>

ImFont* F50 = nullptr;
namespace font { ImFont* inter_semibold = nullptr; }

int main() {
    const auto getDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    assert(getDisplay);
    EGLDisplay display = getDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    EGLint major, minor;
    assert(eglInitialize(display, &major, &minor));
    assert(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint attributes[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    EGLConfig config;
    EGLint count;
    assert(eglChooseConfig(display,attributes,&config,1,&count) && count);
    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    EGLContext context = eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttributes);
    const EGLint surfaceAttributes[] = {EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};
    EGLSurface surface = eglCreatePbufferSurface(display,config,surfaceAttributes);
    assert(eglMakeCurrent(display,surface,surface,context));
    std::printf("GLES: %s\n",glGetString(GL_VERSION));
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().MouseDown[0] = true;
    ImGui::GetIO().DisplaySize = {64,64};
    GLuint textures[3], buffers[2], pbo;
    glGenTextures(3,textures);
    glGenFramebuffers(2,buffers);
    for (int i=0;i<2;++i) {
        glBindTexture(GL_TEXTURE_2D,textures[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER,buffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[i],0);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
        glClearColor(i==0?0.f:1.f,i==0?1.f:0.f,0.f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER,buffers[1]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,buffers[0]);
    glBindTexture(GL_TEXTURE_2D,textures[2]);
    glGenBuffers(1,&pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0,0,1,1);
    glViewport(3,4,51,49);
    glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_FALSE);
    glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE); glEnable(GL_STENCIL_TEST); glEnable(GL_RASTERIZER_DISCARD);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D,textures[2]);
    clay::Capture(64,64,true,false,0.0);
    assert(glGetError()==GL_NO_ERROR);
    assert(clay::sceneValid && clay::materialValid && clay::liquidProgram && clay::blurProgram);
    GLboolean mask[4]; glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    assert(!mask[0] && mask[1] && !mask[2] && !mask[3]);
    assert(glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_CULL_FACE) && glIsEnabled(GL_STENCIL_TEST) && glIsEnabled(GL_RASTERIZER_DISCARD));
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    assert(viewport[0]==3 && viewport[1]==4 && viewport[2]==51 && viewport[3]==49);
    GLint active; glGetIntegerv(GL_ACTIVE_TEXTURE,&active); assert(active==GL_TEXTURE3);
    GLint actual;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&actual); assert(actual==static_cast<GLint>(buffers[1]));
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&actual); assert(actual==static_cast<GLint>(buffers[0]));
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&actual); assert(actual==static_cast<GLint>(textures[2]));
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&actual); assert(actual==static_cast<GLint>(pbo));
    assert(glIsEnabled(GL_SCISSOR_TEST));
    glBindFramebuffer(GL_READ_FRAMEBUFFER,clay::framebuffer);
    unsigned char pixel[4]{};
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0]==0 && pixel[1]==255 && pixel[2]==0 && pixel[3]==255);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,clay::softFramebuffer);
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0]==0 && pixel[1]==255 && pixel[2]==0 && pixel[3]==255);
    const auto before=clay::captureSerial;
    for(int i=1;i<=60;++i) clay::Capture(64,64,true,true,i/60.0);
    assert(clay::captureSerial-before==20);
    assert(glGetError()==GL_NO_ERROR);
    clay::Capture(64,64,false);
    assert(!clay::sceneValid);
    assert(glGetError()==GL_NO_ERROR);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_STENCIL_TEST); glDisable(GL_RASTERIZER_DISCARD);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    assert(ImGui_ImplOpenGL3_Init("#version 300 es"));
    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    clay::Capture(64,64,true,false,2.0);
    ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({64,64});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::Begin("Lens",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground);
    assert(clay::Liquid(ImGui::GetWindowDrawList(),{8,8},{56,56},12,ImVec4(.10f,.06f,.16f,.46f)));
    ImGui::End(); ImGui::PopStyleVar(); ImGui::Render();
    GLuint sampler; glGenSamplers(1,&sampler); glBindSampler(0,sampler);
    glSamplerParameteri(sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glSamplerParameteri(sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    GLuint sampler1; glGenSamplers(1,&sampler1);
    glSamplerParameteri(sampler1,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glSamplerParameteri(sampler1,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,textures[2]); glBindSampler(1,sampler1);
    glActiveTexture(GL_TEXTURE0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glActiveTexture(GL_TEXTURE0); glGetIntegerv(GL_SAMPLER_BINDING,&actual); assert(actual==static_cast<GLint>(sampler));
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&actual); assert(actual==static_cast<GLint>(textures[2]));
    glGetIntegerv(GL_SAMPLER_BINDING,&actual); assert(actual==static_cast<GLint>(sampler1));
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,buffers[0]);
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0]>=8 && pixel[0]<=16 && pixel[1]>=140 && pixel[1]<=150 && pixel[2]>=14 && pixel[2]<=24);
    glReadPixels(9,9,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0]==0 && pixel[1]==255 && pixel[2]==0);
    assert(glGetError()==GL_NO_ERROR);
    std::array<unsigned char,64*64*4> pattern;
    for(int y=0;y<64;++y) for(int x=0;x<64;++x) {
        const int offset=(y*64+x)*4;
        pattern[offset]=pattern[offset+1]=pattern[offset+2]=static_cast<unsigned char>(x*3);
        pattern[offset+3]=255;
    }
    const auto renderPattern=[&](float strength,double now,int sampleX,int sampleY,std::array<unsigned char,4>& edge,std::array<unsigned char,4>& center) {
        glBindFramebuffer(GL_FRAMEBUFFER,buffers[0]);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,textures[0]);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,pattern.data());
        ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
        clay::Capture(64,64,true,false,now);
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({64,64});
        ImGui::Begin("Pattern lens",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground);
        assert(clay::Liquid(ImGui::GetWindowDrawList(),{8,8},{56,56},12,ImVec4(0,0,0,0),strength));
        ImGui::End(); ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glBindFramebuffer(GL_READ_FRAMEBUFFER,buffers[0]);
        glReadPixels(sampleX,sampleY,1,1,GL_RGBA,GL_UNSIGNED_BYTE,edge.data());
        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,center.data());
        glActiveTexture(GL_TEXTURE1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&actual); assert(actual==static_cast<GLint>(textures[2]));
        glGetIntegerv(GL_SAMPLER_BINDING,&actual); assert(actual==static_cast<GLint>(sampler1));
        glActiveTexture(GL_TEXTURE0);
        assert(glGetError()==GL_NO_ERROR);
    };
    std::array<unsigned char,4> flatEdge,flatCenter,bentEdge,bentCenter;
    renderPattern(0.f,3.0,12,32,flatEdge,flatCenter);
    renderPattern(1.25f,4.0,12,32,bentEdge,bentCenter);
    for(int channel=0;channel<3;++channel) {
        assert(bentEdge[channel]-flatEdge[channel]>=10);
        assert(std::abs(int(flatCenter[channel])-int(bentCenter[channel]))<=1);
        assert(std::abs(int(bentCenter[channel])-int(pattern[(32*64+32)*4+channel]))<=2);
    }
    const int redShift=int(bentEdge[0])-int(flatEdge[0]);
    const int greenShift=int(bentEdge[1])-int(flatEdge[1]);
    const int blueShift=int(bentEdge[2])-int(flatEdge[2]);
    assert(redShift>=greenShift && greenShift>=blueShift);
    assert(redShift-blueShift>=2);
    renderPattern(0.f,5.0,51,32,flatEdge,flatCenter);
    renderPattern(1.25f,6.0,51,32,bentEdge,bentCenter);
    for(int channel=0;channel<3;++channel) {
        assert(flatEdge[channel]-bentEdge[channel]>=10);
        assert(std::abs(int(flatCenter[channel])-int(bentCenter[channel]))<=1);
    }
    renderPattern(0.f,6.2,25,32,flatEdge,flatCenter);
    renderPattern(1.25f,6.4,25,32,bentEdge,bentCenter);
    for(int channel=0;channel<3;++channel)
        assert(std::abs(int(flatEdge[channel])-int(bentEdge[channel]))<=2);
    for(int y=0;y<64;++y) for(int x=0;x<64;++x) {
        const int offset=(y*64+x)*4;
        pattern[offset]=pattern[offset+1]=pattern[offset+2]=static_cast<unsigned char>(y*3);
    }
    renderPattern(0.f,7.0,32,51,flatEdge,flatCenter);
    renderPattern(1.25f,8.0,32,51,bentEdge,bentCenter);
    for(int channel=0;channel<3;++channel) {
        assert(flatEdge[channel]-bentEdge[channel]>=10);
        assert(std::abs(int(flatCenter[channel])-int(bentCenter[channel]))<=1);
        assert(std::abs(int(bentCenter[channel])-int(pattern[(32*64+32)*4+channel]))<=2);
    }
    renderPattern(0.f,9.0,32,12,flatEdge,flatCenter);
    renderPattern(1.25f,10.0,32,12,bentEdge,bentCenter);
    for(int channel=0;channel<3;++channel) {
        assert(bentEdge[channel]-flatEdge[channel]>=10);
        assert(std::abs(int(flatCenter[channel])-int(bentCenter[channel]))<=1);
    }
    for(float opacity : {.76f,.80f,.88f,.93f}) {
        glBindFramebuffer(GL_FRAMEBUFFER,buffers[0]);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.f,1.f,0.f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
        const auto serial=clay::captureSerial;
        clay::Capture(64,64,true,false,11.0+opacity);
        assert(clay::captureSerial==serial+1 && clay::materialCount==0);
        ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize({64,64});
        ImGui::Begin("Shared backdrop",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground);
        auto* draw=ImGui::GetWindowDrawList();
        clay::Backdrop(draw,{8,8},{56,56},12,opacity);
        assert(clay::materialCount==1);
        int liquidCallbacks=0;
        for(const auto& command : draw->CmdBuffer)
            if(command.UserCallback==clay::DrawMaterial) ++liquidCallbacks;
        assert(liquidCallbacks==1);
        ImGui::End(); ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        assert(clay::captureSerial==serial+1);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,buffers[0]);
        glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        assert(pixel[1]>pixel[0]+50 && pixel[1]>pixel[2]+50);
        glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
        assert(pixel[0]==0 && pixel[1]==255 && pixel[2]==0 && pixel[3]==255);
        glActiveTexture(GL_TEXTURE1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&actual); assert(actual==static_cast<GLint>(textures[2]));
        glGetIntegerv(GL_SAMPLER_BINDING,&actual); assert(actual==static_cast<GLint>(sampler1));
        glActiveTexture(GL_TEXTURE0);
        assert(glGetError()==GL_NO_ERROR);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);
    glPixelStorei(GL_UNPACK_ALIGNMENT,8);glPixelStorei(GL_UNPACK_ROW_LENGTH,33);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,2);glPixelStorei(GL_UNPACK_SKIP_PIXELS,3);
    const auto art=startup_ui::media::Ensure();
    assert(art && startup_ui::media::uploads==1);
    for(int i=0;i<90;++i) assert(startup_ui::media::Ensure()==art);
    assert(startup_ui::media::uploads==1);
    const auto banner=startup_ui::media::Ensure(true);
    assert(banner && banner!=art && startup_ui::media::uploads==2);
    for(int i=0;i<90;++i) assert(startup_ui::media::Ensure(true)==banner);
    assert(startup_ui::media::uploads==2 && startup_ui::media::uploadCount[0]==1 && startup_ui::media::uploadCount[1]==1);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&actual);assert(actual==8);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,&actual);assert(actual==33);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,&actual);assert(actual==2);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&actual);assert(actual==3);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&actual);assert(actual==static_cast<GLint>(pbo));
    glGetIntegerv(GL_SAMPLER_BINDING,&actual);assert(actual==static_cast<GLint>(sampler));
    int imageWidth,imageHeight,components;
    auto* decoded=stbi_load_from_memory(startup_ui::media::artwork,sizeof(startup_ui::media::artwork),&imageWidth,&imageHeight,&components,4);
    assert(decoded && imageWidth==startup_ui::media::artworkWidth && imageHeight==startup_ui::media::artworkHeight);
    GLuint artFramebuffer;glGenFramebuffers(1,&artFramebuffer);glBindFramebuffer(GL_READ_FRAMEBUFFER,artFramebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,art,0);
    assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glReadPixels(80,80,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    for(int i=0;i<4;++i) assert(pixel[i]==decoded[(80*imageWidth+80)*4+i]);
    stbi_image_free(decoded);
    decoded=stbi_load_from_memory(startup_ui::media::bannerArtwork,sizeof(startup_ui::media::bannerArtwork),&imageWidth,&imageHeight,&components,4);
    assert(decoded && imageWidth==startup_ui::media::bannerWidth && imageHeight==startup_ui::media::bannerHeight);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,banner,0);
    assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    const auto firstFrame=startup_ui::media::BannerCrop(3.2f,0.);
    const auto laterFrame=startup_ui::media::BannerCrop(3.2f,.9);
    unsigned char firstPixel[4]{};
    const auto readFrame=[&](startup_ui::media::Crop crop,unsigned char* result) {
        const int x=int((crop.lo.x+crop.hi.x)*.5f*imageWidth),y=int((crop.lo.y+crop.hi.y)*.5f*imageHeight);
        glReadPixels(x,y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,result);
        for(int channel=0;channel<4;++channel) assert(result[channel]==decoded[(y*imageWidth+x)*4+channel]);
    };
    readFrame(firstFrame,firstPixel); readFrame(laterFrame,pixel);
    assert(firstPixel[0]!=pixel[0] || firstPixel[1]!=pixel[1] || firstPixel[2]!=pixel[2]);
    stbi_image_free(decoded);glDeleteFramebuffers(1,&artFramebuffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    assert(glGetError()==GL_NO_ERROR);
    glBindSampler(0,0); glBindSampler(1,0); glDeleteSamplers(1,&sampler); glDeleteSamplers(1,&sampler1);
    ImGui_ImplOpenGL3_Shutdown();
    std::puts("Capture and blur pixels, GL state, drag capture budget, inward X/Y refraction, chroma fringe, neutral center, shared backdrop modes, clipping and sampler restoration passed");
    ImGui::DestroyContext();
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroySurface(display,surface); eglDestroyContext(display,context); eglTerminate(display);
}
