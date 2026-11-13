#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/ClayGlass.h"
#include <cmath>
#ifndef CLAY_CPU_PREVIEW
#include "StartupArtData.h"
#include "../../ImGui/stb_image.h"
#endif

namespace startup_ui { namespace media {
struct Crop { ImVec2 lo,hi; };
inline Crop BannerCrop(float aspect,double elapsed) {
    const float time=std::isfinite(elapsed)?static_cast<float>(std::fmod(ImMax(0.0,elapsed),1000.0)):0.f;
    aspect=std::isfinite(aspect)?ImMax(.1f,aspect):3.f;
    const float zoom=1.045f+.010f*std::sin(time*.30f);
    const float sx=ImMin(1.f,aspect/3.f)/zoom,sy=ImMin(1.f,3.f/aspect)/zoom;
    const float cx=ImClamp(.5f+.010f*std::sin(time*.19f),sx*.5f,1.f-sx*.5f);
    const float cy=ImClamp(.5f+.008f*std::cos(time*.23f),sy*.5f,1.f-sy*.5f);
    return {{cx-sx*.5f,cy-sy*.5f},{cx+sx*.5f,cy+sy*.5f}};
}
#ifndef CLAY_CPU_PREVIEW
inline GLuint texture=0;
inline EGLContext context=EGL_NO_CONTEXT;
inline bool attempted=false;
inline unsigned uploads=0;
inline GLuint Ensure() {
    const auto current=eglGetCurrentContext();
    if(current==EGL_NO_CONTEXT) return 0;
    if(current!=context) { context=current;texture=0;attempted=false; }
    if(texture || attempted) return texture;
    attempted=true;
    int width=0,height=0,components=0;
    unsigned char* pixels=stbi_load_from_memory(artwork,sizeof(artwork),&width,&height,&components,4);
    if(!pixels || width!=artworkWidth || height!=artworkHeight) { stbi_image_free(pixels); return 0; }
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
    stbi_image_free(pixels);++uploads;
    return texture;
}
#endif
inline bool Image(ImDrawList* draw,ImVec2 lo,ImVec2 hi,float radius,Crop crop) {
#ifndef CLAY_CPU_PREVIEW
    const auto id=Ensure();
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
    Image(draw,lo+ImVec2(inset,inset),lo+ImVec2(size-inset,size-inset),size*.24f,{{.22f,.02f},{.54f,.98f}});
    draw->AddRect(lo,lo+ImVec2(size,size),IM_COL32(231,216,255,110),size*.30f,0,1.f);
}
inline void Banner(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,double elapsed) {
    const float radius=28.f*scale;
    Image(draw,lo,lo+size,radius,BannerCrop(size.x/size.y,elapsed));
    draw->PushClipRect(lo+ImVec2(22,22)*scale,lo+size-ImVec2(22,22)*scale,true);
    const float time=std::isfinite(elapsed)?static_cast<float>(std::fmod(ImMax(0.0,elapsed),1000.0)):0.f;
    for(int i=0;i<6;++i) {
        const float spanX=ImMax(1.f,size.x-52.f*scale),spanY=ImMax(1.f,size.y-52.f*scale);
        const float x=lo.x+26.f*scale+std::fmod(i*.173f*spanX+time*(5.f+i*.4f)*scale,spanX);
        const float y=lo.y+26.f*scale+std::fmod(i*.291f*spanY+time*(2.f+i*.2f)*scale,spanY);
        const float radius=(2.0f+(i%3)*.6f)*scale;
        draw->AddNgonFilled({x,y},radius,IM_COL32(231,194,255,85),5);
    }
    draw->PopClipRect();
    draw->AddRect(lo,lo+size,IM_COL32(230,217,255,84),radius,0,1.f*scale);
    draw->AddLine(lo+ImVec2(radius,1.f*scale),{lo.x+size.x-radius,lo.y+1.f*scale},IM_COL32(255,240,255,82),1.f*scale);
}
}}
