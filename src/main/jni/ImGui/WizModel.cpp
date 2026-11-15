#include "WizModel.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <thread>
#include <zlib.h>

namespace wiz_model {
namespace {
std::uint32_t Little(const unsigned char* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
std::uint32_t Big(const unsigned char* p) {
    return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
}
int Paeth(int a,int b,int c) {
    const int p=a+b-c,x=std::abs(p-a),y=std::abs(p-b),z=std::abs(p-c);
    return x<=y&&x<=z?a:y<=z?b:c;
}
bool Texture(Material& m) {
    const unsigned char signature[]{137,80,78,71,13,10,26,10};
    if(m.png.size()<33||std::memcmp(m.png.data(),signature,8)) return false;
    bool header=false,end=false;
    std::vector<unsigned char> compressed;
    for(std::size_t offset=8;offset+12<=m.png.size();) {
        const auto count=Big(m.png.data()+offset);
        if(count>m.png.size()-offset-12) return false;
        const auto* type=m.png.data()+offset+4;const auto* p=type+4;
        if(crc32(0,type,count+4)!=Big(p+count)) return false;
        if(!std::memcmp(type,"IHDR",4)) {
            if(header||count!=13||Big(p)<1||Big(p)>1024||Big(p+4)<1||Big(p+4)>1024||p[8]!=8||p[9]!=6||p[10]||p[11]||p[12]) return false;
            m.width=Big(p);m.height=Big(p+4);header=true;
        } else if(!std::memcmp(type,"IDAT",4)) {
            if(!header) return false;
            compressed.insert(compressed.end(),p,p+count);
        } else if(!std::memcmp(type,"IEND",4)) {end=true;break;}
        offset+=count+12;
    }
    if(!header||!end||compressed.empty()) return false;
    const std::size_t stride=m.width*4;
    std::vector<unsigned char> raw((stride+1)*m.height);
    uLongf size=raw.size();
    if(uncompress(raw.data(),&size,compressed.data(),compressed.size())!=Z_OK||size!=raw.size()) return false;
    m.rgba.resize(stride*m.height);
    for(int y=0;y<m.height;++y) {
        const auto filter=raw[y*(stride+1)];if(filter>4) return false;
        const auto* source=raw.data()+y*(stride+1)+1;
        auto* row=m.rgba.data()+y*stride;const auto* previous=y?row-stride:nullptr;
        for(std::size_t x=0;x<stride;++x) {
            const int a=x>=4?row[x-4]:0,b=previous?previous[x]:0,c=previous&&x>=4?previous[x-4]:0;
            row[x]=static_cast<unsigned char>(source[x]+(filter==1?a:filter==2?b:filter==3?(a+b)/2:filter==4?Paeth(a,b,c):0));
        }
    }
    return true;
}
}
bool Decode(const unsigned char* bytes,std::size_t size,Model& model) {
    if(!bytes||size<12||size>8*1024*1024||std::memcmp(bytes,"WIZSKN01",8)) return false;
    const auto rawSize=Little(bytes+8);if(rawSize<32||rawSize>16*1024*1024) return false;
    std::vector<unsigned char> raw(rawSize);uLongf length=rawSize;
    if(uncompress(raw.data(),&length,bytes+12,size-12)!=Z_OK||length!=rawSize) return false;
    const auto* p=raw.data();const auto* end=p+raw.size();
    const auto version=Little(p),vertices=Little(p+4),indices=Little(p+8),bones=Little(p+12),frames=Little(p+16),fps=Little(p+20),materials=Little(p+24);
    float duration;std::memcpy(&duration,p+28,4);p+=32;
    if(version!=1||vertices<3||vertices>100000||indices<3||indices>300000||indices%3||!bones||bones>128||frames<2||frames>1000||!fps||fps>60||materials<1||materials>8||!std::isfinite(duration)||duration<=0||duration>30||std::fabs((frames-1.f)/fps-duration)>.001f) return false;
    const std::size_t meshSize=vertices*sizeof(Vertex)+indices*4;
    if(std::size_t(end-p)<meshSize) return false;
    Model result;result.bones=bones;result.frames=frames;result.fps=fps;result.duration=duration;
    result.vertices.resize(vertices);std::memcpy(result.vertices.data(),p,vertices*sizeof(Vertex));p+=vertices*sizeof(Vertex);
    result.indices.resize(indices);std::memcpy(result.indices.data(),p,indices*4);p+=indices*4;
    for(const auto index:result.indices) if(index>=vertices) return false;
    for(const auto& v:result.vertices) {
        unsigned sum=0;for(int i=0;i<4;++i) {if(v.joints[i]>=bones) return false;sum+=v.weights[i];}
        if(sum!=65535) return false;
        for(int i=0;i<3;++i) if(!std::isfinite(v.position[i])||!std::isfinite(v.normal[i])||!std::isfinite(v.blink[i])||!std::isfinite(v.smile[i])||!std::isfinite(v.speak[i])) return false;
        for(float uv:v.uv) if(!std::isfinite(uv)) return false;
    }
    std::uint32_t next=0;
    for(unsigned i=0;i<materials;++i) {
        if(end-p<12) return false;
        Material m;m.first=Little(p);m.count=Little(p+4);const auto pngSize=Little(p+8);p+=12;
        if(m.first!=next||m.count>indices-m.first||m.count%3||pngSize>4*1024*1024||std::size_t(end-p)<pngSize) return false;
        m.png.assign(p,p+pngSize);p+=pngSize;next+=m.count;result.materials.push_back(std::move(m));
    }
    if(next!=indices) return false;
    const auto count=result.Stride()*frames;
    if(std::size_t(end-p)!=count*sizeof(float)) return false;
    result.animation.resize(count);std::memcpy(result.animation.data(),p,count*sizeof(float));
    for(float value:result.animation) if(!std::isfinite(value)) return false;
    model=std::move(result);return true;
}
bool DecodeTextures(Model& model) {
    for(auto& material:model.materials) if(!Texture(material)) return false;
    return true;
}
bool Sample(const Model& model,double seconds,std::vector<float>& frame) {
    if(model.frames<2||!model.fps||!std::isfinite(model.duration)||model.duration<=0||model.animation.size()!=model.Stride()*model.frames) return false;
    if(!std::isfinite(seconds)) seconds=0;
    double elapsed=std::fmod(std::max(0.,seconds),model.duration)*model.fps;
    const auto first=std::min<std::size_t>(static_cast<std::size_t>(elapsed),model.frames-2),second=first+1;
    const float mix=static_cast<float>(elapsed-first);frame.resize(model.Stride());
    for(std::size_t i=0;i<frame.size();++i) frame[i]=model.animation[first*frame.size()+i]*(1-mix)+model.animation[second*frame.size()+i]*mix;
    return true;
}
}

#ifndef CLAY_CPU_PREVIEW
#include <EGL/egl.h>
#include <GLES3/gl3.h>
namespace wiz_model {
extern "C" const unsigned char wiz_model_start[],wiz_model_end[];
namespace {
struct Loader {
    Model model;
    std::atomic<int> state{0};
    std::thread worker;
    ~Loader() {if(worker.joinable()) worker.join();}
} loader;
struct Gpu {
    EGLContext context=EGL_NO_CONTEXT;
    GLuint vao=0,vbo=0,ibo=0,ubo=0,program=0,fbo=0,color=0,depth=0;
    std::vector<GLuint> textures;
    std::vector<float> sample;
    GLint projection=-1,morph=-1,outline=-1;
    double bucket=-1;
    float aspect=0;
    Frame frame;
    bool failed=false;
} gpu;
unsigned renders=0,uploads=0;
struct State {
    GLint program,drawFbo,readFbo,vao,array,ubo,baseUbo,active,texture,sampler,viewport[4],scissor[4],depthFunc,unpack,pbo,cullFace,frontFace;
    GLint unpackRow,unpackSkipRows,unpackSkipPixels;
    GLint64 uboStart=0,uboSize=0;
    GLfloat clearColor[4],clearDepth;
    GLboolean depthMask,colorMask[4],depth,blend,cull,scissorOn,stencil,discard,offset,coverage,alphaCoverage;
    State() {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&array);glGetIntegerv(GL_UNIFORM_BUFFER_BINDING,&ubo);
        glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING,7,&baseUbo);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);
        glGetInteger64i_v(GL_UNIFORM_BUFFER_START,7,&uboStart);glGetInteger64i_v(GL_UNIFORM_BUFFER_SIZE,7,&uboSize);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegerv(GL_SAMPLER_BINDING,&sampler);
        glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SCISSOR_BOX,scissor);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);
        glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&pbo);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH,&unpackRow);glGetIntegerv(GL_UNPACK_SKIP_ROWS,&unpackSkipRows);glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&unpackSkipPixels);
        glGetIntegerv(GL_CULL_FACE_MODE,&cullFace);glGetIntegerv(GL_FRONT_FACE,&frontFace);
        glGetFloatv(GL_COLOR_CLEAR_VALUE,clearColor);glGetFloatv(GL_DEPTH_CLEAR_VALUE,&clearDepth);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
        depth=glIsEnabled(GL_DEPTH_TEST);blend=glIsEnabled(GL_BLEND);cull=glIsEnabled(GL_CULL_FACE);scissorOn=glIsEnabled(GL_SCISSOR_TEST);stencil=glIsEnabled(GL_STENCIL_TEST);discard=glIsEnabled(GL_RASTERIZER_DISCARD);offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);coverage=glIsEnabled(GL_SAMPLE_COVERAGE);alphaCoverage=glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    static void Enable(GLenum value,GLboolean enabled) {if(enabled) glEnable(value);else glDisable(value);}
    ~State() {
        glUseProgram(program);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);
        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,array);
        if(baseUbo&&uboSize>0) glBindBufferRange(GL_UNIFORM_BUFFER,7,baseUbo,uboStart,uboSize);else glBindBufferBase(GL_UNIFORM_BUFFER,7,baseUbo);
        glBindBuffer(GL_UNIFORM_BUFFER,ubo);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glBindSampler(0,sampler);glActiveTexture(active);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
        glDepthFunc(depthFunc);glDepthMask(depthMask);glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
        glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);glClearDepthf(clearDepth);glCullFace(cullFace);glFrontFace(frontFace);
        glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);glPixelStorei(GL_UNPACK_ROW_LENGTH,unpackRow);glPixelStorei(GL_UNPACK_SKIP_ROWS,unpackSkipRows);glPixelStorei(GL_UNPACK_SKIP_PIXELS,unpackSkipPixels);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);
        Enable(GL_DEPTH_TEST,depth);Enable(GL_BLEND,blend);Enable(GL_CULL_FACE,cull);Enable(GL_SCISSOR_TEST,scissorOn);Enable(GL_STENCIL_TEST,stencil);Enable(GL_RASTERIZER_DISCARD,discard);Enable(GL_POLYGON_OFFSET_FILL,offset);Enable(GL_SAMPLE_COVERAGE,coverage);Enable(GL_SAMPLE_ALPHA_TO_COVERAGE,alphaCoverage);
    }
};
GLuint Shader(GLenum type,const char* source) {
    const auto shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
    GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);if(!ok) {glDeleteShader(shader);return 0;}return shader;
}
bool Create() {
    const char* vertex=R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(location=3) in uvec4 joints;
layout(location=4) in vec4 weights;
layout(location=5) in vec3 blink;
layout(location=6) in vec3 smile;
layout(location=7) in vec3 speak;
layout(std140) uniform Rig {mat4 bones[128];};
uniform mat4 projection;
uniform vec3 morph;
uniform float outline;
out vec2 texcoord;
out vec3 direction;
void main() {
    mat4 skin=bones[joints.x]*weights.x+bones[joints.y]*weights.y+bones[joints.z]*weights.z+bones[joints.w]*weights.w;
    vec3 p=position+blink*morph.x+smile*morph.y+speak*morph.z+normal*outline;
    gl_Position=projection*skin*vec4(p,1.0);
    texcoord=uv;direction=normalize(mat3(skin)*normal);
})GLSL";
    const char* fragment=R"GLSL(#version 300 es
precision highp float;
in vec2 texcoord;
in vec3 direction;
uniform sampler2D image;
uniform float outline;
out vec4 color;
void main() {
    vec4 paint=texture(image,texcoord);
    if(paint.a<0.5) discard;
    float light=0.82+0.18*smoothstep(-0.3,0.8,dot(normalize(direction),normalize(vec3(-0.3,-0.8,0.8))));
    color=vec4(outline>0.0?vec3(0.12,0.08,0.18):paint.rgb*light,1.0);
})GLSL";
    const GLuint vs=Shader(GL_VERTEX_SHADER,vertex),fs=Shader(GL_FRAGMENT_SHADER,fragment);
    if(!vs||!fs) {if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return false;}
    gpu.program=glCreateProgram();glAttachShader(gpu.program,vs);glAttachShader(gpu.program,fs);glLinkProgram(gpu.program);glDeleteShader(vs);glDeleteShader(fs);
    GLint ok=0;glGetProgramiv(gpu.program,GL_LINK_STATUS,&ok);if(!ok) return false;
    glUseProgram(gpu.program);gpu.projection=glGetUniformLocation(gpu.program,"projection");gpu.morph=glGetUniformLocation(gpu.program,"morph");gpu.outline=glGetUniformLocation(gpu.program,"outline");
    glUniform1i(glGetUniformLocation(gpu.program,"image"),0);glUniformBlockBinding(gpu.program,glGetUniformBlockIndex(gpu.program,"Rig"),7);
    glGenVertexArrays(1,&gpu.vao);glBindVertexArray(gpu.vao);
    glGenBuffers(1,&gpu.vbo);glBindBuffer(GL_ARRAY_BUFFER,gpu.vbo);glBufferData(GL_ARRAY_BUFFER,loader.model.vertices.size()*sizeof(Vertex),loader.model.vertices.data(),GL_STATIC_DRAW);
    glGenBuffers(1,&gpu.ibo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu.ibo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,loader.model.indices.size()*4,loader.model.indices.data(),GL_STATIC_DRAW);
    for(GLuint i=0;i<8;++i) glEnableVertexAttribArray(i);
    const auto attribute=[](unsigned index,int count,GLenum type,bool normalized,std::size_t offset) {glVertexAttribPointer(index,count,type,normalized,80,reinterpret_cast<void*>(offset));};
    attribute(0,3,GL_FLOAT,false,0);attribute(1,3,GL_FLOAT,false,12);attribute(2,2,GL_FLOAT,false,24);
    glVertexAttribIPointer(3,4,GL_UNSIGNED_BYTE,80,reinterpret_cast<void*>(32));attribute(4,4,GL_UNSIGNED_SHORT,true,36);
    attribute(5,3,GL_FLOAT,false,44);attribute(6,3,GL_FLOAT,false,56);attribute(7,3,GL_FLOAT,false,68);
    glGenBuffers(1,&gpu.ubo);glBindBuffer(GL_UNIFORM_BUFFER,gpu.ubo);glBufferData(GL_UNIFORM_BUFFER,128*64,nullptr,GL_DYNAMIC_DRAW);
    glActiveTexture(GL_TEXTURE0);glBindSampler(0,0);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    gpu.textures.resize(loader.model.materials.size());glGenTextures(gpu.textures.size(),gpu.textures.data());
    for(std::size_t i=0;i<gpu.textures.size();++i) {
        auto& m=loader.model.materials[i];glBindTexture(GL_TEXTURE_2D,gpu.textures[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m.width,m.height,0,GL_RGBA,GL_UNSIGNED_BYTE,m.rgba.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        std::vector<unsigned char>().swap(m.rgba);
    }
    glGenTextures(1,&gpu.color);glBindTexture(GL_TEXTURE_2D,gpu.color);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,320,512,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glGenTextures(1,&gpu.depth);glBindTexture(GL_TEXTURE_2D,gpu.depth);glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT16,320,512,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_SHORT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glGenFramebuffers(1,&gpu.fbo);glBindFramebuffer(GL_FRAMEBUFFER,gpu.fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,gpu.color,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,gpu.depth,0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) return false;
    ++uploads;return true;
}
using Matrix=std::array<float,16>;
using Vector=std::array<float,3>;
Vector Normalize(Vector v) {float n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);for(auto& x:v)x/=n;return v;}
Vector Cross(Vector a,Vector b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
float Dot(Vector a,Vector b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
Matrix Camera(float aspect) {
    const float f=1.f/std::tan(16.f*3.141592654f/180.f),distance=std::max(4.6f,.75f*f/aspect);
    Vector eye{0,-distance,1.3f},target{0,.04f,1.12f};Vector forward=Normalize({target[0]-eye[0],target[1]-eye[1],target[2]-eye[2]});
    const auto side=Normalize(Cross(forward,{0,0,1})),up=Cross(side,forward);
    Matrix view{side[0],up[0],-forward[0],0,side[1],up[1],-forward[1],0,side[2],up[2],-forward[2],0,-Dot(side,eye),-Dot(up,eye),Dot(forward,eye),1};
    Matrix projection{f/aspect,0,0,0,0,f,0,0,0,0,-20.1f/19.9f,-1,0,0,-4.f/19.9f,0},result{};
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) for(int k=0;k<4;++k) result[c*4+r]+=projection[k*4+r]*view[c*4+k];
    return result;
}
std::array<float,2> Project(const Matrix& m,const float* p) {
    const float w=m[3]*p[0]+m[7]*p[1]+m[11]*p[2]+m[15];
    return {(.5f+.5f*(m[0]*p[0]+m[4]*p[1]+m[8]*p[2]+m[12])/w),(.5f-.5f*(m[1]*p[0]+m[5]*p[1]+m[9]*p[2]+m[13])/w)};
}
}
void Prepare() {
    int expected=0;if(!loader.state.compare_exchange_strong(expected,1,std::memory_order_acq_rel)) return;
    if(loader.worker.joinable()) loader.worker.join();
    loader.worker=std::thread([] {
        bool valid=loader.model.frames?DecodeTextures(loader.model):Decode(wiz_model_start,std::size_t(wiz_model_end-wiz_model_start),loader.model)&&DecodeTextures(loader.model);
        loader.state.store(valid?2:3,std::memory_order_release);
    });
}
bool Ready() {return loader.state.load(std::memory_order_acquire)==2;}
bool Failed() {return loader.state.load(std::memory_order_acquire)==3||gpu.failed;}
unsigned RenderCount() {return renders;}
unsigned UploadCount() {return uploads;}
void Release() {
    if(gpu.context!=EGL_NO_CONTEXT&&gpu.context==eglGetCurrentContext()) {
        glDeleteVertexArrays(1,&gpu.vao);glDeleteBuffers(1,&gpu.vbo);glDeleteBuffers(1,&gpu.ibo);glDeleteBuffers(1,&gpu.ubo);glDeleteTextures(gpu.textures.size(),gpu.textures.data());glDeleteTextures(1,&gpu.color);glDeleteTextures(1,&gpu.depth);glDeleteFramebuffers(1,&gpu.fbo);glDeleteProgram(gpu.program);
    }
    const bool decoded=Ready();gpu=Gpu{};if(decoded) loader.state.store(0,std::memory_order_release);
}
Frame Render(double seconds,float aspect) {
    if(!std::isfinite(seconds)) seconds=0;
    aspect=std::isfinite(aspect)?std::max(.3f,std::min(2.f,aspect)):.625f;
    const auto context=eglGetCurrentContext();if(context==EGL_NO_CONTEXT) return {};
    if(gpu.context!=context) {
        const bool rebuild=gpu.context!=EGL_NO_CONTEXT;
        gpu=Gpu{};gpu.context=context;
        if(rebuild&&Ready()) loader.state.store(0,std::memory_order_release);
    }
    Prepare();if(!Ready()||gpu.failed) return {};
    const double bucket=std::floor(std::max(0.,seconds)*30.+.00001);
    if(gpu.frame.texture&&gpu.bucket==bucket&&std::fabs(gpu.aspect-aspect)<.001f) return gpu.frame;
    State state;
    if(!gpu.program&&!Create()) {gpu.failed=true;return {};}
    auto& frame=gpu.sample;Sample(loader.model,bucket/30.,frame);
    const auto matrix=Camera(aspect);const auto offset=loader.model.bones*16;
    glBindFramebuffer(GL_FRAMEBUFFER,gpu.fbo);glViewport(0,0,320,512);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_RASTERIZER_DISCARD);glDisable(GL_BLEND);glDisable(GL_POLYGON_OFFSET_FILL);glDisable(GL_SAMPLE_COVERAGE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(0,0,0,0);glClearDepthf(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(gpu.program);glUniformMatrix4fv(gpu.projection,1,GL_FALSE,matrix.data());glUniform3fv(gpu.morph,1,frame.data()+offset);
    glBindVertexArray(gpu.vao);glBindBufferBase(GL_UNIFORM_BUFFER,7,gpu.ubo);glBufferSubData(GL_UNIFORM_BUFFER,0,offset*4,frame.data());glActiveTexture(GL_TEXTURE0);glBindSampler(0,0);
    glFrontFace(GL_CCW);
    for(int pass=0;pass<2;++pass) {
        if(pass==0) {glEnable(GL_CULL_FACE);glCullFace(GL_FRONT);} else glDisable(GL_CULL_FACE);
        glUniform1f(gpu.outline,pass==0?.0025f:0.f);
        for(std::size_t i=0;i<gpu.textures.size();++i) {
            const auto& material=loader.model.materials[i];glBindTexture(GL_TEXTURE_2D,gpu.textures[i]);glDrawElements(GL_TRIANGLES,material.count,GL_UNSIGNED_INT,reinterpret_cast<void*>(std::size_t(material.first)*4));
        }
    }
    Frame result;result.texture=gpu.color;
    for(unsigned i=0;i<15;++i) result.points[i]=Project(matrix,frame.data()+offset+3+i*3);
    result.min={1,1};result.max={0,0};
    const auto* low=frame.data()+offset+48;const auto* high=low+3;
    for(unsigned corner=0;corner<8;++corner) {
        float point[3];for(int i=0;i<3;++i) point[i]=corner&(1u<<i)?high[i]:low[i];
        const auto projected=Project(matrix,point);for(int i=0;i<2;++i) {result.min[i]=std::min(result.min[i],projected[i]);result.max[i]=std::max(result.max[i],projected[i]);}
    }
    const float floor[]{0,0,.01f};result.floor=Project(matrix,floor);result.lift=frame[offset+54];result.excited=frame[offset+55]>.1f;
    gpu.frame=result;gpu.bucket=bucket;gpu.aspect=aspect;++renders;return result;
}
}
#endif
