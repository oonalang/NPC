#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/ClayGlass.h"
#include "StartupArtData.h"
#include <cmath>
#ifndef CLAY_CPU_PREVIEW
#include "../../ImGui/stb_image.h"
#endif

namespace startup_ui { namespace media {
struct Crop { ImVec2 lo,hi; };
inline int FrameAt(double elapsed) {
    const double safe=std::isfinite(elapsed)?ImMax(0.0,elapsed):0.0;
    const double phase=std::fmod(safe,bannerDurationMs/1000.0);
    return ImMin(bannerFrameCount-1,static_cast<int>(phase*1000.0*bannerFrameCount/bannerDurationMs));
}
inline Crop BannerCrop(float aspect,double elapsed) {
    aspect=std::isfinite(aspect)?ImMax(.1f,aspect):3.2f;
    const float width=ImMin(float(bannerFrameWidth),bannerFrameHeight*aspect);
    const float height=ImMin(float(bannerFrameHeight),bannerFrameWidth/aspect);
    const int frame=FrameAt(elapsed);
    const float x=(frame%bannerColumns)*(bannerFrameWidth+2*bannerGutter)+bannerGutter+(bannerFrameWidth-width)*.5f;
    const float y=(frame/bannerColumns)*(bannerFrameHeight+2*bannerGutter)+bannerGutter+(bannerFrameHeight-height)*.5f;
    return {{(x+.5f)/bannerWidth,(y+.5f)/bannerHeight},{(x+width-.5f)/bannerWidth,(y+height-.5f)/bannerHeight}};
}
#ifndef CLAY_CPU_PREVIEW
inline GLuint textures[2]{};
inline EGLContext context=EGL_NO_CONTEXT;
inline bool attempted[2]{};
inline unsigned uploads=0,uploadCount[2]{};
inline GLuint Ensure(bool animated=false) {
    const auto current=eglGetCurrentContext();
    if(current==EGL_NO_CONTEXT) return 0;
    if(current!=context) {
        context=current;
        for(int i=0;i<2;++i) { textures[i]=0;attempted[i]=false; }
    }
    const int index=animated?1:0;
    auto& texture=textures[index];
    if(texture || attempted[index]) return texture;
    attempted[index]=true;
    const auto* encoded=animated?bannerArtwork:artwork;
    const int length=animated?sizeof(bannerArtwork):sizeof(artwork);
    int width=0,height=0,components=0;
    unsigned char* pixels=stbi_load_from_memory(encoded,length,&width,&height,&components,4);
    if(!pixels || width!=(animated?bannerWidth:artworkWidth) || height!=(animated?bannerHeight:artworkHeight)) {
        stbi_image_free(pixels); return 0;
    }
    clay::SavedGl saved;
    GLint alignment,rowLength,skipRows,skipPixels;
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment); glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowLength);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skipRows); glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skipPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1); glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0); glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,rowLength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,skipRows);glPixelStorei(GL_UNPACK_SKIP_PIXELS,skipPixels);
    stbi_image_free(pixels);++uploads;++uploadCount[index];
    return texture;
}
#endif
inline bool Image(ImDrawList* draw,ImVec2 lo,ImVec2 hi,float radius,Crop crop,bool animated=false) {
#ifndef CLAY_CPU_PREVIEW
    const auto id=Ensure(animated);
    if(id) {
        draw->AddImageRounded(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(id)),lo,hi,crop.lo,crop.hi,IM_COL32_WHITE,radius);
        return true;
    }
#endif
    draw->AddRectFilled(lo,hi,IM_COL32(60,39,79,255),radius);
    return false;
}
inline void Avatar(ImDrawList* draw,ImVec2 lo,float size) {
    const float inset=size*.065f;
    clay::Liquid(draw,lo,lo+ImVec2(size,size),size*.30f,ImVec4(.52f,.37f,.71f,.16f),1.2f);
    Image(draw,lo+ImVec2(inset,inset),lo+ImVec2(size-inset,size-inset),size*.24f,{{0,0},{1,1}});
    draw->AddRect(lo,lo+ImVec2(size,size),IM_COL32(231,216,255,110),size*.30f,0,1.f);
}
inline void Banner(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,double elapsed) {
    const float radius=28.f*scale;
    Image(draw,lo,lo+size,radius,BannerCrop(size.x/size.y,elapsed),true);
    draw->AddRect(lo,lo+size,IM_COL32(230,217,255,84),radius,0,1.f*scale);
    draw->AddLine(lo+ImVec2(radius,1.f*scale),{lo.x+size.x-radius,lo.y+1.f*scale},IM_COL32(255,240,255,82),1.f*scale);
}
}}
