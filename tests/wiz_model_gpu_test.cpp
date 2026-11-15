#include "../src/main/jni/ImGui/WizModel.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <zlib.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
static EGLDisplay display;
static EGLConfig config;
static EGLSurface surface;
static EGLContext Context() {
    const EGLint attributes[]{EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    const auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,attributes);assert(context!=EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display,surface,surface,context));return context;
}
static void Wait() {
    wiz_model::Prepare();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(!wiz_model::Ready()&&!wiz_model::Failed()&&std::chrono::steady_clock::now()<deadline) std::this_thread::yield();
    assert(wiz_model::Ready()&&!wiz_model::Failed());
}
static void Save(const wiz_model::Frame& frame,const std::string& path) {
    GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,frame.texture,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    constexpr unsigned w=320,h=512;std::vector<unsigned char> pixels(w*h*4),rows((w*4+1)*h);
    glBindBuffer(GL_PIXEL_PACK_BUFFER,0);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    unsigned visible=0;for(unsigned i=3;i<pixels.size();i+=4) if(pixels[i]) ++visible;assert(visible>5000&&visible<w*h/2);
    for(unsigned y=0;y<h;++y) std::memcpy(rows.data()+y*(w*4+1)+1,pixels.data()+(h-1-y)*w*4,w*4);
    uLongf size=compressBound(rows.size());std::vector<unsigned char> packed(size);assert(compress2(packed.data(),&size,rows.data(),rows.size(),3)==Z_OK);
    std::ofstream output(path,std::ios::binary);assert(output);
    const unsigned char sig[]{137,80,78,71,13,10,26,10};output.write(reinterpret_cast<const char*>(sig),8);
    const auto word=[&](unsigned v) {const unsigned char b[]{static_cast<unsigned char>(v>>24),static_cast<unsigned char>(v>>16),static_cast<unsigned char>(v>>8),static_cast<unsigned char>(v)};output.write(reinterpret_cast<const char*>(b),4);};
    const auto chunk=[&](const char* type,const unsigned char* data,unsigned count) {word(count);output.write(type,4);if(count) output.write(reinterpret_cast<const char*>(data),count);auto crc=crc32(0,reinterpret_cast<const Bytef*>(type),4);if(count) crc=crc32(crc,data,count);word(crc);};
    const unsigned char header[]{0,0,1,64,0,0,2,0,8,6,0,0,0};chunk("IHDR",header,13);chunk("IDAT",packed.data(),size);chunk("IEND",nullptr,0);
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fbo);
}
static GLint Get(GLenum key) {GLint v;glGetIntegerv(key,&v);return v;}
static void StateTest() {
    GLuint vao,buffer,texture[2],sampler,pbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    glGenBuffers(1,&buffer);glBindBuffer(GL_UNIFORM_BUFFER,buffer);const int alignment=Get(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    glBufferData(GL_UNIFORM_BUFFER,alignment+256,nullptr,GL_STATIC_DRAW);glBindBufferRange(GL_UNIFORM_BUFFER,7,buffer,alignment,128);
    glGenBuffers(1,&pbo);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);glBufferData(GL_PIXEL_UNPACK_BUFFER,2048,nullptr,GL_STATIC_DRAW);
    glGenTextures(2,texture);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture[0]);glGenSamplers(1,&sampler);glBindSampler(0,sampler);
    glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D,texture[1]);
    glEnable(GL_RASTERIZER_DISCARD);glEnable(GL_SCISSOR_TEST);glEnable(GL_BLEND);glEnable(GL_CULL_FACE);glCullFace(GL_FRONT);glFrontFace(GL_CW);
    glEnable(GL_STENCIL_TEST);glEnable(GL_SAMPLE_COVERAGE);glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);glEnable(GL_POLYGON_OFFSET_FILL);
    glViewport(11,7,83,99);glScissor(12,8,32,24);glDepthFunc(GL_GREATER);glDepthMask(GL_FALSE);glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,8);glPixelStorei(GL_UNPACK_ROW_LENGTH,72);glPixelStorei(GL_UNPACK_SKIP_ROWS,4);glPixelStorei(GL_UNPACK_SKIP_PIXELS,2);
    const auto frame=wiz_model::Render(0,.625f);assert(frame.texture);
    assert(glGetError()==GL_NO_ERROR);assert(Get(GL_VERTEX_ARRAY_BINDING)==GLint(vao));assert(Get(GL_ACTIVE_TEXTURE)==GL_TEXTURE3);
    assert(Get(GL_TEXTURE_BINDING_2D)==GLint(texture[1]));assert(Get(GL_UNIFORM_BUFFER_BINDING)==GLint(buffer));
    GLint64 start,size;glGetInteger64i_v(GL_UNIFORM_BUFFER_START,7,&start);glGetInteger64i_v(GL_UNIFORM_BUFFER_SIZE,7,&size);assert(start==alignment&&size==128);
    assert(Get(GL_PIXEL_UNPACK_BUFFER_BINDING)==GLint(pbo)&&Get(GL_UNPACK_ALIGNMENT)==8&&Get(GL_UNPACK_ROW_LENGTH)==72&&Get(GL_UNPACK_SKIP_ROWS)==4&&Get(GL_UNPACK_SKIP_PIXELS)==2);
    assert(Get(GL_DEPTH_FUNC)==GL_GREATER&&Get(GL_CULL_FACE_MODE)==GL_FRONT&&Get(GL_FRONT_FACE)==GL_CW);
    for(GLenum cap:{GL_RASTERIZER_DISCARD,GL_SCISSOR_TEST,GL_BLEND,GL_CULL_FACE,GL_STENCIL_TEST,GL_SAMPLE_COVERAGE,GL_SAMPLE_ALPHA_TO_COVERAGE,GL_POLYGON_OFFSET_FILL}) assert(glIsEnabled(cap));
    GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);assert(viewport[0]==11&&viewport[1]==7&&viewport[2]==83&&viewport[3]==99);
    GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0]&&mask[1]&&!mask[2]&&mask[3]);
    glActiveTexture(GL_TEXTURE0);assert(Get(GL_TEXTURE_BINDING_2D)==GLint(texture[0]));assert(Get(GL_SAMPLER_BINDING)==GLint(sampler));
    glBindSampler(0,0);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    for(GLenum cap:{GL_RASTERIZER_DISCARD,GL_SCISSOR_TEST,GL_BLEND,GL_CULL_FACE,GL_STENCIL_TEST,GL_SAMPLE_COVERAGE,GL_SAMPLE_ALPHA_TO_COVERAGE,GL_POLYGON_OFFSET_FILL}) glDisable(cap);
    glBindVertexArray(0);glBindBufferBase(GL_UNIFORM_BUFFER,7,0);glBindBuffer(GL_UNIFORM_BUFFER,0);glDeleteBuffers(1,&buffer);glDeleteBuffers(1,&pbo);glDeleteSamplers(1,&sampler);glDeleteTextures(2,texture);glDeleteVertexArrays(1,&vao);
}
int main(int argc,char** argv) {
    assert(argc==2);const std::string directory=argv[1];
    const auto getDisplay=reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    display=getDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,nullptr);EGLint major,minor;assert(eglInitialize(display,&major,&minor));assert(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint attrs[]{EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLint count;
    assert(eglChooseConfig(display,attrs,&config,1,&count)&&count);const EGLint sa[]{EGL_WIDTH,320,EGL_HEIGHT,512,EGL_NONE};surface=eglCreatePbufferSurface(display,config,sa);
    auto context=Context();Wait();StateTest();const auto initialUploads=wiz_model::UploadCount();
    for(int i=0;i<720;++i) {
        const double t=i/60.;const auto before=wiz_model::RenderCount();const auto frame=wiz_model::Render(t,.625f);assert(frame.texture);
        for(int axis=0;axis<2;++axis) {
            if(!(frame.min[axis]>0&&frame.max[axis]<1&&frame.max[axis]>frame.min[axis])) std::fprintf(stderr,"Bounds at %.3fs axis %d: %.4f .. %.4f\n",t,axis,frame.min[axis],frame.max[axis]);
            assert(frame.min[axis]>0&&frame.max[axis]<1&&frame.max[axis]>frame.min[axis]);
        }
        wiz_model::Render(t,.625f);assert(wiz_model::RenderCount()<=before+1);assert(wiz_model::UploadCount()==initialUploads);
        if(i%3==0) Save(frame,directory+"/frame-"+std::to_string(i/3)+".png");
    }
    assert(wiz_model::RenderCount()==360);
    for(float aspect:{.45f,.75f,1.2f}) {const auto f=wiz_model::Render(4.4,aspect);assert(f.texture&&f.min[0]>0&&f.max[0]<1);}
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);context=Context();
    wiz_model::Render(0,.625f);Wait();const auto rebuilt=wiz_model::Render(0,.625f);assert(rebuilt.texture&&wiz_model::UploadCount()==initialUploads+1);
    assert(glGetError()==GL_NO_ERROR);wiz_model::Release();assert(!wiz_model::Ready());
    Wait();assert(wiz_model::Render(0,.625f).texture);wiz_model::Release();
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    std::puts("Wiz GLES: full loop, 30-Hz render cache, single GPU upload, projected bounds, shared GL-state restoration, context-loss recovery and release/reload passed");
}
