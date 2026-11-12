#define IMGUI_DEFINE_MATH_OPERATORS
#include "../src/main/jni/ImGui/imgui_internal.h"
#include "../src/main/jni/ImGui/ClayGlass.h"
#include <EGL/eglext.h>
#include <cassert>
#include <cstdio>

ImFont* F50 = nullptr;
namespace font { ImFont* inter_semibold = nullptr; }

// Optional integration check: requires a surfaceless Mesa EGL/GLES3 runtime.
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
    // Empty unpack buffer catches an allocation that mistakenly reads offset 0.
    glEnable(GL_SCISSOR_TEST);
    glScissor(0,0,1,1);
    clay::Capture(64,64,true);
    assert(glGetError()==GL_NO_ERROR);
    assert(clay::sceneValid);
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
    clay::Capture(64,64,false);
    assert(!clay::sceneValid);
    assert(glGetError()==GL_NO_ERROR);
    std::puts("Capture pixels, hidden-menu skip and GL state restoration passed");
    ImGui::DestroyContext();
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroySurface(display,surface); eglDestroyContext(display,context); eglTerminate(display);
}
