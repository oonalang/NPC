#include "../src/main/jni/ImGui/WizModel.h"
#include "../src/main/jni/ImGui/AnimePreview.h"
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
#include <cstdio>
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
    for(unsigned x=0;x<w;++x) assert(!pixels[x*4+3]&&!pixels[((h-1)*w+x)*4+3]);
    for(unsigned y=0;y<h;++y) assert(!pixels[(y*w)*4+3]&&!pixels[(y*w+w-1)*4+3]);
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
    GLuint shader[2],program=glCreateProgram();
    const char* sources[]{"#version 300 es\nvoid main(){gl_Position=vec4(0,0,0,1);}","#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1);}"};
    for(unsigned i=0;i<2;++i) {shader[i]=glCreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);glShaderSource(shader[i],1,&sources[i],nullptr);glCompileShader(shader[i]);glAttachShader(program,shader[i]);}
    glLinkProgram(program);GLint linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);
    GLuint framebuffer[2],attachment[2];glGenFramebuffers(2,framebuffer);glGenTextures(2,attachment);
    for(unsigned i=0;i<2;++i) {glBindTexture(GL_TEXTURE_2D,attachment[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,4,4,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer[i]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,attachment[i],0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);}
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,framebuffer[0]);glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer[1]);
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
    const auto steps=wiz_model::PhysicsStats().steps;wiz_model::Render(0,.625f);assert(wiz_model::PhysicsStats().steps==steps);
    assert(glGetError()==GL_NO_ERROR);assert(Get(GL_VERTEX_ARRAY_BINDING)==GLint(vao));assert(Get(GL_ACTIVE_TEXTURE)==GL_TEXTURE3);
    assert(Get(GL_CURRENT_PROGRAM)==GLint(program)&&Get(GL_DRAW_FRAMEBUFFER_BINDING)==GLint(framebuffer[0])&&Get(GL_READ_FRAMEBUFFER_BINDING)==GLint(framebuffer[1]));
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
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(2,framebuffer);glDeleteTextures(2,attachment);glUseProgram(0);glDeleteProgram(program);for(auto id:shader) glDeleteShader(id);
}
int main(int argc,char** argv) {
    assert(argc==2||argc==3);const std::string directory=argv[1];
    const auto getDisplay=reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    display=getDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,nullptr);EGLint major,minor;assert(eglInitialize(display,&major,&minor));assert(eglBindAPI(EGL_OPENGL_ES_API));
    const EGLint attrs[]{EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLint count;
    assert(eglChooseConfig(display,attrs,&config,1,&count)&&count);const EGLint sa[]{EGL_WIDTH,320,EGL_HEIGHT,512,EGL_NONE};surface=eglCreatePbufferSurface(display,config,sa);
    auto context=Context();
    if(argc==3) {
        wiz_model::Prepare();const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!wiz_model::Failed()&&std::chrono::steady_clock::now()<deadline) std::this_thread::yield();
        assert(wiz_model::Failed()&&!wiz_model::Ready()&&!wiz_model::Render(0,.625f).texture);
        ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={320,512};io.DeltaTime=1.f/60;
        io.Fonts->AddFontDefault();io.Fonts->Build();anime_artwork::Prepare();
        const auto pngDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!anime_artwork::Ready()&&!anime_artwork::Failed()&&std::chrono::steady_clock::now()<pngDeadline) std::this_thread::yield();
        assert(anime_artwork::Ready());
        ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({320,512});ImGui::Begin("Native failure fallback");
        const auto frame=anime_preview::Draw(ImGui::GetWindowDrawList(),{24,40},{296,488},0,1.f/60);
        assert(!frame.native3d&&!frame.loading&&frame.height>0&&anime_preview::artwork.texture&&anime_artwork::uploads==1);
        assert(ImGui::GetWindowDrawList()->VtxBuffer.Size>3000);ImGui::End();ImGui::Render();ImGui::DestroyContext();
        assert(glGetError()==GL_NO_ERROR);wiz_model::Release();
        assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
        std::puts("Failure path: unsupported native asset reports failure; real PNG decodes/uploads once; AnimePreview selects the textured compatibility fallback with native3d=false");return 0;
    }
    Wait();StateTest();const auto initialUploads=wiz_model::UploadCount();
    const auto initialResources=wiz_model::Resources();
    assert(initialResources.bones==120&&initialResources.springs==42&&initialResources.cpuBytes<8*1024*1024&&initialResources.gpuBytes<16*1024*1024);
    for(int i=0;i<48*60;++i) {
        const double t=i/60.;const auto before=wiz_model::RenderCount();const auto frame=wiz_model::Render(t,.625f);assert(frame.texture);
        for(int axis=0;axis<2;++axis) {
            if(!(frame.min[axis]>0&&frame.max[axis]<1&&frame.max[axis]>frame.min[axis])) std::fprintf(stderr,"Bounds at %.3fs axis %d: %.4f .. %.4f\n",t,axis,frame.min[axis],frame.max[axis]);
            assert(frame.min[axis]>0&&frame.max[axis]<1&&frame.max[axis]>frame.min[axis]);
        }
        const auto physicsSteps=wiz_model::PhysicsStats().steps;wiz_model::Render(t,.625f);assert(wiz_model::PhysicsStats().steps==physicsSteps);
        assert(wiz_model::RenderCount()<=before+1);assert(wiz_model::UploadCount()==initialUploads);
        const auto resources=wiz_model::Resources();assert(resources.cpuBytes==initialResources.cpuBytes&&resources.gpuBytes==initialResources.gpuBytes);
        const auto feet=frame.points[14];assert(feet[1]>.6f&&feet[1]<.97f);
        for(const auto& point:frame.points) assert(point[0]>=frame.min[0]-.01f&&point[0]<=frame.max[0]+.01f&&point[1]>=frame.min[1]-.01f&&point[1]<=frame.max[1]+.01f);
        if(i%3==0) Save(frame,directory+"/frame-"+std::to_string(i/3)+".png");
    }
    assert(wiz_model::RenderCount()==48*30);
    const auto beforeOrbit=wiz_model::RenderCount();
    for(int i=1;i<=120;++i) {wiz_model::SetView(std::sin(i/120.f)*.8f);wiz_model::Render(12.+i/120.,.625f);}
    assert(wiz_model::RenderCount()-beforeOrbit<=31);wiz_model::SetView(0);
    double aspectTime=4.4;
    for(float aspect:{.45f,.75f,1.2f}) {const auto count=wiz_model::RenderCount();const auto f=wiz_model::Render(aspectTime,aspect);assert(f.texture&&f.min[0]>0&&f.max[0]<1&&wiz_model::RenderCount()==count+1);aspectTime+=1./30;}
    double orbitTime=8.7;wiz_model::Render(orbitTime,.625f);
    for(float yaw:{0.f,.785398163f,1.570796327f,-1.570796327f}) {
        const auto orbitSteps=wiz_model::PhysicsStats().steps,previousRenders=wiz_model::RenderCount();
        wiz_model::SetView(yaw);wiz_model::Render(orbitTime,.625f);assert(wiz_model::PhysicsStats().steps==orbitSteps&&wiz_model::RenderCount()==previousRenders);
        orbitTime+=1./30;const auto frame=wiz_model::Render(orbitTime,.625f);assert(frame.texture&&wiz_model::View()[0]==yaw);
        for(int axis=0;axis<2;++axis) assert(frame.min[axis]>0&&frame.max[axis]<1);
        Save(frame,directory+"/orbit-"+std::to_string(int(yaw*180/3.141592654f))+".png");
    }
    wiz_model::SetView(100,-100);assert(wiz_model::View()[0]<=1.570797f&&wiz_model::View()[1]>=-.18001f);wiz_model::SetView(0);
    wiz_model::SetDynamics(wiz_model::Dynamics::Hair);assert(wiz_model::Render(8.7,.625f).texture);
    wiz_model::SetDynamics(wiz_model::Dynamics::Off);Save(wiz_model::Render(8.7,.625f),directory+"/physics-off.png");
    wiz_model::SetDynamics(wiz_model::Dynamics::Full);Save(wiz_model::Render(8.7,.625f),directory+"/physics-warm.png");
    wiz_model::ResetMotion();const auto reset=wiz_model::Render(120,.625f);assert(reset.texture&&wiz_model::PhysicsStats().lastSteps<=wiz_model::Simulation::warmSteps+1);
    std::printf("Resource storage estimate: CPU %zu bytes; GLES buffers/textures %zu bytes; packed asset %zu bytes; 42 spring bones; 30-Hz rendering / 60-Hz simulation\n",initialResources.cpuBytes,initialResources.gpuBytes,initialResources.assetBytes);
    auto owned=wiz_model::Render(120,.625f);GLuint removedTexture=owned.texture;glDeleteTextures(1,&removedTexture);
    assert(!wiz_model::Render(120,.625f).texture);Wait();assert(wiz_model::Render(120,.625f).texture);const auto recoveredUploads=wiz_model::UploadCount();assert(recoveredUploads==initialUploads+1);
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);context=Context();
    wiz_model::Render(0,.625f);Wait();const auto rebuilt=wiz_model::Render(0,.625f);assert(rebuilt.texture&&wiz_model::UploadCount()==recoveredUploads+1);
    assert(glGetError()==GL_NO_ERROR);wiz_model::Release();assert(!wiz_model::Ready());
    Wait();assert(wiz_model::Render(0,.625f).texture);wiz_model::Release();
    assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    std::puts("Wiz GLES: 48s gesture schedule, no clipped capture pixels, runtime secondary motion, front/quarter/side views, overlay landmarks, 30-Hz render cache / 60-Hz simulation, bounded storage, shared GL-state restoration, context recovery and release/reload passed");
}
