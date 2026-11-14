#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../Fonts/Icon.h"
#include "StartupState.h"
#include <array>
#include <cmath>

namespace startup_ui { namespace button_art {
inline float Smooth(float value) {
    value=ImClamp(value,0.f,1.f);
    return value*value*(3.f-2.f*value);
}
inline void Text(ImDrawList* draw,ImFont* font,float size,ImVec2 pos,ImU32 color,const char* text) {
    const auto shadow=IM_COL32(4,3,9,((color>>IM_COL32_A_SHIFT)&255)*.85f);
    const float spread=ImClamp(size/24.f,.55f,1.f);
    for(const auto offset : {ImVec2(-spread,0),ImVec2(spread,0),ImVec2(0,-spread),ImVec2(0,spread)})
        draw->AddText(font,size,pos+offset,shadow,text);
    draw->AddText(font,size,pos,color,text);
}
inline void SignIn(ImDrawList* draw,ImFont* font,ImVec2 center,float size,float rotation,float opacity,float morph=0.f) {
    if(opacity<=0.f) return;
    const auto measured=font->CalcTextSizeA(size,FLT_MAX,0.f,ICON_FA_SIGN_IN);
    const int first=draw->VtxBuffer.Size;
    Text(draw,font,size,center-measured*.5f,ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,opacity)),ICON_FA_SIGN_IN);
    const float sine=std::sin(rotation),cosine=std::cos(rotation);
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& point=draw->VtxBuffer[i].pos;
        auto offset=point-center;
        offset.x*=ImLerp(1.f,.28f,morph);
        offset.y*=ImLerp(1.f,1.15f,morph);
        point=center+ImVec2(offset.x*cosine-offset.y*sine,offset.x*sine+offset.y*cosine);
    }
}
inline ImDrawList* fluidDraw=nullptr;
inline ImDrawListSharedData* fluidData=nullptr;
inline ImDrawList* FluidDraw(ImDrawList* target) {
    if(!fluidDraw || fluidData!=target->_Data) {
        if(fluidDraw) IM_DELETE(fluidDraw);
        fluidData=target->_Data;
        fluidDraw=IM_NEW(ImDrawList)(fluidData);
        fluidDraw->VtxBuffer.reserve(2048);
        fluidDraw->IdxBuffer.reserve(4096);
    }
    fluidDraw->_ResetForNewFrame();
    fluidDraw->Flags=target->Flags&~(ImDrawListFlags_AllowVtxOffset|ImDrawListFlags_AntiAliasedLinesUseTex);
    fluidDraw->PushTextureID(target->_CmdHeader.TextureId);
    fluidDraw->PushClipRectFullScreen();
    return fluidDraw;
}
struct ClipVertex { ImVec2 pos,uv; ImVec4 color; };
inline float Cross(ImVec2 a,ImVec2 b) { return a.x*b.y-a.y*b.x; }
inline ClipVertex Mix(const ClipVertex& a,const ClipVertex& b,float amount) {
    return {ImLerp(a.pos,b.pos,amount),ImLerp(a.uv,b.uv,amount),ImLerp(a.color,b.color,amount)};
}
inline bool InsidePill(ImVec2 point,ImVec2 lo,ImVec2 size) {
    const float radius=size.y*.5f;
    const auto q=ImVec2(std::fabs(point.x-lo.x-size.x*.5f),std::fabs(point.y-lo.y-radius))-
        ImVec2(size.x*.5f-radius,0);
    const auto outer=ImVec2(ImMax(q.x,0.f),ImMax(q.y,0.f));
    const float distance=std::sqrt(ImLengthSqr(outer))+ImMin(ImMax(q.x,q.y),0.f)-radius;
    return distance<-.3f;
}
inline void Emit(ImDrawList* draw,const ClipVertex& a,const ClipVertex& b,const ClipVertex& c) {
    if(std::fabs(Cross(b.pos-a.pos,c.pos-a.pos))<.0001f) return;
    draw->PrimReserve(3,3);
    const auto index=draw->_VtxCurrentIdx;
    draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
    draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+1));
    draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+2));
    draw->PrimWriteVtx(a.pos,a.uv,ImGui::ColorConvertFloat4ToU32(a.color));
    draw->PrimWriteVtx(b.pos,b.uv,ImGui::ColorConvertFloat4ToU32(b.color));
    draw->PrimWriteVtx(c.pos,c.uv,ImGui::ColorConvertFloat4ToU32(c.color));
}
inline void ClipFluid(ImDrawList* target,const ImDrawList* source,ImVec2 lo,ImVec2 size) {
    std::array<ImVec2,20> boundary;
    const float radius=size.y*.5f;
    for(int i=0;i<=9;++i) {
        const float angle=-IM_PI*.5f+IM_PI*i/9.f;
        boundary[i]=lo+ImVec2(size.x-radius+std::cos(angle)*radius,radius+std::sin(angle)*radius);
    }
    for(int i=0;i<=9;++i) {
        const float angle=IM_PI*.5f+IM_PI*i/9.f;
        boundary[10+i]=lo+ImVec2(radius+std::cos(angle)*radius,radius+std::sin(angle)*radius);
    }
    for(int triangle=0;triangle+2<source->IdxBuffer.Size;triangle+=3) {
        std::array<ClipVertex,40> polygon,next;
        int count=3;
        bool interior=true;
        for(int j=0;j<3;++j) {
            const auto& vertex=source->VtxBuffer[source->IdxBuffer[triangle+j]];
            polygon[j]={vertex.pos,vertex.uv,ImGui::ColorConvertU32ToFloat4(vertex.col)};
            interior=interior && InsidePill(vertex.pos,lo,size);
        }
        if(!interior) for(int edge=0;edge<static_cast<int>(boundary.size()) && count;++edge) {
            const auto a=boundary[edge],b=boundary[(edge+1)%boundary.size()];
            int output=0;
            auto previous=polygon[count-1];
            float previousDistance=Cross(b-a,previous.pos-a);
            for(int j=0;j<count;++j) {
                const auto current=polygon[j];
                const float currentDistance=Cross(b-a,current.pos-a);
                if((currentDistance>=0.f)!=(previousDistance>=0.f))
                    next[output++]=Mix(previous,current,previousDistance/(previousDistance-currentDistance));
                if(currentDistance>=0.f) next[output++]=current;
                previous=current;
                previousDistance=currentDistance;
            }
            count=output;
            polygon.swap(next);
        }
        for(int j=1;j+1<count;++j) Emit(target,polygon[0],polygon[j],polygon[j+1]);
    }
}
inline void Quad(ImDrawList* draw,ImVec2 a,ImVec2 b,ImVec2 c,ImVec2 d,ImU32 color) {
    draw->PrimReserve(6,4);
    const auto uv=draw->_Data->TexUvWhitePixel;
    draw->PrimQuadUV(a,b,c,d,uv,uv,uv,uv,color);
}
inline void Droplet(ImDrawList* draw,ImVec2 center,float scale,float opacity) {
    if(opacity<=0.f || scale<=0.f) return;
    const auto point=[&](float x,float y) { return center+ImVec2(x,y)*scale; };
    draw->PathLineTo(point(0,-16));
    draw->PathBezierCubicCurveTo(point(-3,-8),point(-10,-2),point(-9,5),8);
    draw->PathBezierCubicCurveTo(point(-8,16),point(8,16),point(9,5),12);
    draw->PathBezierCubicCurveTo(point(10,-2),point(3,-8),point(0,-16),8);
    draw->PathFillConvex(ImGui::GetColorU32(ImVec4(1,1,1,opacity)));
}
inline void Splash(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,float progress) {
    const float t=ImClamp(progress,0.f,1.f);
    const auto impact=lo+ImVec2(size.x*.5f,size.y-1.5f*scale);
    const float crown=std::sin(IM_PI*ImClamp(t/.75f,0.f,1.f))*19.f*scale;
    const float width=(12.f+49.f*t)*scale;
    const float opacity=1.f-Smooth((t-.35f)/.65f);
    const ImVec2 crest[]{
        {-.95f,0},{-.57f,-.12f},{-.44f,-.9f},{-.25f,-.28f},{-.12f,-.58f},
        {0,-.18f},{.17f,-.6f},{.3f,-.26f},{.48f,-1.f},{.62f,-.12f},{.95f,0}
    };
    const auto white=ImGui::GetColorU32(ImVec4(1,1,1,opacity));
    for(int i=0;i+1<static_cast<int>(IM_ARRAYSIZE(crest));++i) {
        const auto a=impact+ImVec2(crest[i].x*width,crest[i].y*crown);
        const auto b=impact+ImVec2(crest[i+1].x*width,crest[i+1].y*crown);
        Quad(draw,a,b,{b.x,impact.y+5.f*scale},{a.x,impact.y+5.f*scale},white);
    }
    const float velocities[][2]{{-76,-72},{-44,-91},{-20,-56},{23,-88},{48,-66},{79,-80}};
    for(int i=0;i<6;++i) {
        const auto center=impact+ImVec2(velocities[i][0]*t,velocities[i][1]*t+105.f*t*t)*scale;
        const float radius=(i%2?2.5f:1.8f)*scale*(1.f-.45f*t);
        draw->AddCircleFilled(center,radius,ImGui::GetColorU32(ImVec4(1,1,1,1.f-Smooth((t-.58f)/.42f))),10);
    }
    const float rippleWidth=(13.f+104.f*t)*scale;
    const float rippleHeight=(1.3f+3.f*t)*scale;
    for(int i=0;i<=28;++i) {
        const float angle=IM_PI+IM_PI*i/28.f;
        draw->PathLineTo(impact+ImVec2(std::cos(angle)*rippleWidth,std::sin(angle)*rippleHeight));
    }
    draw->PathStroke(ImGui::GetColorU32(ImVec4(1,1,1,(1.f-t)*.9f)),0,1.2f*scale);
}
inline void Wave(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,float progress) {
    const float t=ImClamp(progress,0.f,1.f);
    if(t<=0.f) return;
    const float amplitude=size.y*.19f*std::sin(IM_PI*t);
    const float level=lo.y+size.y*(1.f-t);
    const auto surface=[&](float x,float phase,float strength) {
        const float u=(x-lo.x)/size.x;
        return level+amplitude*strength*(std::sin(u*IM_PI*3.f-t*IM_PI*2.f+phase)+
            .28f*std::sin(u*IM_PI*5.f+t*IM_PI*2.8f+phase));
    };
    const auto background=ImGui::GetColorU32(ImVec4(.86f,.80f,1.f,.40f));
    const auto foreground=ImGui::GetColorU32(ImVec4(1,1,1,1));
    const auto base=lo.y+size.y+2.f*scale;
    for(int i=0;i<40;++i) {
        const float x0=lo.x+size.x*i/40.f,x1=lo.x+size.x*(i+1)/40.f;
        Quad(draw,{x0,surface(x0,1.6f,.95f)},{x1,surface(x1,1.6f,.95f)},{x1,base},{x0,base},background);
        Quad(draw,{x0,surface(x0,0.f,1.f)},{x1,surface(x1,0.f,1.f)},{x1,base},{x0,base},foreground);
    }
}
inline void Check(ImDrawList* draw,ImVec2 center,float scale,float progress) {
    const float t=ImClamp(progress,0.f,1.f);
    if(t<=0.f) return;
    const float artScale=2.2f*scale;
    const auto a=center+ImVec2(-12.f,0)*artScale;
    const auto b=center+ImVec2(-4.f,8.f)*artScale;
    const auto c=center+ImVec2(12.f,-8.f)*artScale;
    const float first=std::sqrt(ImLengthSqr(b-a)),second=std::sqrt(ImLengthSqr(c-b));
    const float distance=t*(first+second);
    draw->PathLineTo(a);
    draw->PathLineTo(ImLerp(a,b,ImMin(distance/first,1.f)));
    if(distance>first) draw->PathLineTo(ImLerp(b,c,(distance-first)/second));
    draw->PathStroke(ImGui::GetColorU32(ImVec4(.025f,.025f,.035f,1.f)),0,3.8f*artScale);
}
}

inline void DrawAuthenticateContents(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,
                                      const ButtonFrame& frame,ImFont* body,ImFont* icons) {
    if(!draw || size.x<=0.f || size.y<=0.f) return;
    if(!body) body=ImGui::GetFont();
    if(!icons) icons=body;
    const auto center=lo+size*.5f;
    const float textSize=17.f*scale,baseIcon=40.f*scale;
    const auto label=body->CalcTextSizeA(textSize,FLT_MAX,0.f,"Authenticate");
    const float startX=lo.x+(size.x-label.x-35.f*scale)*.5f+9.5f*scale;
    const auto iconCenter=ImLerp(ImVec2(startX,center.y),center,button_art::Smooth(frame.iconMove));
    if(frame.stage==ButtonStage::Idle || frame.stage==ButtonStage::LabelFade)
        button_art::Text(draw,body,textSize,{startX+25.5f*scale,lo.y+(size.y-label.y)*.5f},
            ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,ImClamp(frame.labelOpacity,0.f,1.f))),"Authenticate");
    if(frame.stage<=ButtonStage::Spin)
        button_art::SignIn(draw,icons,iconCenter,baseIcon*frame.iconScale,frame.iconRotation,1.f);
    if(frame.stage==ButtonStage::Droplet) {
        const float morph=button_art::Smooth(frame.dropletMorph);
        button_art::SignIn(draw,icons,center,baseIcon*frame.iconScale,0.f,1.f-morph,morph);
    }
    if(frame.stage>=ButtonStage::Droplet && frame.stage<=ButtonStage::Check) {
        auto* fluid=button_art::FluidDraw(draw);
        if(frame.stage==ButtonStage::Droplet || frame.stage==ButtonStage::Hover || frame.stage==ButtonStage::Fall) {
            const float morph=frame.stage==ButtonStage::Droplet?button_art::Smooth(frame.dropletMorph):1.f;
            const float fall=frame.stage==ButtonStage::Fall?ImClamp(frame.fall,0.f,1.f):0.f;
            const float y=ImLerp(center.y,lo.y+size.y-6.f*scale,fall*fall);
            button_art::Droplet(fluid,{center.x,y},scale*ImLerp(.15f,1.f,morph),morph);
        }
        if(frame.stage==ButtonStage::Splash) button_art::Splash(fluid,lo,size,scale,frame.splash);
        if(frame.stage==ButtonStage::Wave || frame.stage==ButtonStage::Check)
            button_art::Wave(fluid,lo,size,scale,frame.stage==ButtonStage::Check?1.f:frame.wave);
        button_art::ClipFluid(draw,fluid,lo,size);
    }
    if(frame.stage==ButtonStage::Check) button_art::Check(draw,center,scale,frame.check);
}
}
