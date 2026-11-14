#pragma once
#include "imgui.h"
#include <cstddef>
#include <cstdint>
#ifndef CLAY_CPU_PREVIEW
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <zlib.h>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#endif

namespace anime_artwork {
inline constexpr int width=1024,height=1536;
#ifndef CLAY_CPU_PREVIEW
extern "C" const unsigned char anime_artwork_start[];
extern "C" const unsigned char anime_artwork_end[];
inline std::uint32_t Word(const unsigned char* p) {
    return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
}
inline int Paeth(int a,int b,int c) {
    const int p=a+b-c,pa=std::abs(p-a),pb=std::abs(p-b),pc=std::abs(p-c);
    return pa<=pb && pa<=pc?a:pb<=pc?b:c;
}
inline bool Decode(std::vector<unsigned char>& rgba) {
    const auto* bytes=anime_artwork_start;
    const auto length=static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(anime_artwork_end)-reinterpret_cast<std::uintptr_t>(anime_artwork_start));
    const unsigned char signature[]{137,80,78,71,13,10,26,10};
    if(length<33 || std::memcmp(bytes,signature,8)!=0) return false;
    std::vector<unsigned char> compressed;
    bool header=false,ended=false;
    for(std::size_t offset=8;offset+12<=length;) {
        const auto count=static_cast<std::size_t>(Word(bytes+offset));
        if(count>length-offset-12) return false;
        const auto* type=bytes+offset+4;
        const auto* data=bytes+offset+8;
        if(crc32(0,type,static_cast<uInt>(count+4))!=Word(data+count)) return false;
        if(std::memcmp(type,"IHDR",4)==0) {
            if(header || count!=13 || Word(data)!=width || Word(data+4)!=height ||
               data[8]!=8 || data[9]!=6 || data[10]!=0 || data[11]!=0 || data[12]!=0) return false;
            header=true;
        } else if(std::memcmp(type,"IDAT",4)==0) {
            if(!header) return false;
            compressed.insert(compressed.end(),data,data+count);
        } else if(std::memcmp(type,"IEND",4)==0) { ended=true; break; }
        offset+=count+12;
    }
    if(!header || !ended || compressed.empty()) return false;
    constexpr std::size_t stride=width*4;
    std::vector<unsigned char> raw((stride+1)*height);
    uLongf size=static_cast<uLongf>(raw.size());
    if(uncompress(raw.data(),&size,compressed.data(),static_cast<uLong>(compressed.size()))!=Z_OK || size!=raw.size()) return false;
    rgba.resize(stride*height);
    for(int y=0;y<height;++y) {
        const auto filter=raw[std::size_t(y)*(stride+1)];
        if(filter>4) return false;
        const auto* source=raw.data()+std::size_t(y)*(stride+1)+1;
        auto* row=rgba.data()+std::size_t(y)*stride;
        const auto* previous=y?row-stride:nullptr;
        for(std::size_t x=0;x<stride;++x) {
            const int a=x>=4?row[x-4]:0,b=previous?previous[x]:0,c=previous && x>=4?previous[x-4]:0;
            const int predictor=filter==1?a:filter==2?b:filter==3?(a+b)/2:filter==4?Paeth(a,b,c):0;
            row[x]=static_cast<unsigned char>(source[x]+predictor);
        }
    }
    return true;
}
struct Decoder {
    std::atomic<int> state{0};
    std::mutex mutex;
    std::vector<unsigned char> rgba;
    std::thread worker;
    ~Decoder() { if(worker.joinable()) worker.join(); }
};
inline Decoder decoder;
inline GLuint texture=0;
inline EGLContext context=EGL_NO_CONTEXT;
inline unsigned uploads=0;
inline void Prepare() {
    int expected=0;
    if(!decoder.state.compare_exchange_strong(expected,1,std::memory_order_acq_rel)) return;
    decoder.worker=std::thread([] {
        std::vector<unsigned char> pixels;
        const bool valid=Decode(pixels);
        if(valid) { std::lock_guard<std::mutex> lock(decoder.mutex); decoder.rgba.swap(pixels); }
        decoder.state.store(valid?2:3,std::memory_order_release);
    });
}
inline bool Ready() { return decoder.state.load(std::memory_order_acquire)==2; }
inline bool Failed() { return decoder.state.load(std::memory_order_acquire)==3; }
inline ImTextureID Ensure() {
    const auto current=eglGetCurrentContext();
    if(current==EGL_NO_CONTEXT) return nullptr;
    if(current!=context) { texture=0;context=current; }
    if(texture) return reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(texture));
    Prepare();
    if(!Ready()) return nullptr;
    std::lock_guard<std::mutex> lock(decoder.mutex);
    GLint binding=0,pbo=0,alignment=0,rowLength=0,skipRows=0,skipPixels=0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&pbo);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skipRows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skipPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,decoder.rgba.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,rowLength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,skipRows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,skipPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);
    glBindTexture(GL_TEXTURE_2D,binding);
    ++uploads;
    return reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(texture));
}
inline void Release() {
    if(texture && context==eglGetCurrentContext()) glDeleteTextures(1,&texture);
    texture=0;context=EGL_NO_CONTEXT;
}
#else
inline void Prepare() {}
inline bool Ready() { return false; }
inline bool Failed() { return false; }
inline ImTextureID Ensure() { return nullptr; }
inline void Release() {}
#endif
}
