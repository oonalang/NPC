#pragma once
#include "../../ImGui/imgui_internal.h"
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
inline void Quad(ImDrawList* draw,ImVec2 a,ImVec2 b,ImVec2 c,ImVec2 d,ImU32 color) {
    draw->PrimReserve(6,4);
    const auto uv=draw->_Data->TexUvWhitePixel;
    draw->PrimQuadUV(a,b,c,d,uv,uv,uv,uv,color);
}
inline void Fade(ImDrawList* draw,int first,float opacity) {
    if(opacity>=1.f) return;
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& color=draw->VtxBuffer[i].col;
        const auto alpha=static_cast<ImU32>(((color>>IM_COL32_A_SHIFT)&255)*ImClamp(opacity,0.f,1.f));
        color=(color&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);
    }
}
struct Surface {ImVec2 lo,size;float opacity=1.f;};
inline Surface SurfaceAt(ImVec2 lo,ImVec2 size,const ButtonFrame& frame) {
    const float circle=size.y*.72f;
    float amount=0.f,opacity=1.f;
    if(frame.stage>=ButtonStage::Collapse && frame.stage<=ButtonStage::Hover) amount=Smooth(frame.collapse);
    if(frame.stage==ButtonStage::Droplet) opacity=1.f-Smooth(frame.dropletMorph);
    if(frame.stage==ButtonStage::Hover) opacity=0.f;
    if(frame.stage==ButtonStage::Fall) {opacity=Smooth(frame.fall);amount=1.f-opacity;}
    const auto extent=amount>=1.f?ImVec2(circle,circle):amount<=0.f?size:ImLerp(size,ImVec2(circle,circle),amount);
    return {lo+(size-extent)*.5f,extent,opacity};
}
inline ImVec2 DropContour(float turn) {
    static const ImVec2 curves[4][4]{
        {{0,-1},{-.06f,-.50f},{-.76f,-.16f},{-.76f,.24f}},
        {{-.76f,.24f},{-.76f,.76f},{-.40f,1},{0,1}},
        {{0,1},{.40f,1},{.76f,.76f},{.76f,.24f}},
        {{.76f,.24f},{.76f,-.16f},{.06f,-.50f},{0,-1}}
    };
    const float phase=turn*4.f;
    const int segment=ImClamp(static_cast<int>(phase),0,3);
    const float t=ImClamp(phase-segment,0.f,1.f),u=1.f-t;
    const auto* p=curves[segment];
    return p[0]*(u*u*u)+p[1]*(3.f*u*u*t)+p[2]*(3.f*u*t*t)+p[3]*(t*t*t);
}
inline ImVec2 FluidContour(float turn,float morph,float rotation,float radius) {
    const float angle=-IM_PI*.5f-turn*2.f*IM_PI+rotation;
    return ImLerp(ImVec2(std::cos(angle),std::sin(angle)),DropContour(turn),morph)*radius;
}
inline void FluidLoop(ImDrawList* draw,ImVec2 center,float radius,float morph,float rotation) {
    const float blend=Smooth(morph),sweep=ImLerp(.75f,1.f,Smooth(ImClamp(morph/.6f,0.f,1.f)));
    const float hole=.8f*(1.f-blend);
    constexpr int segments=72;
    const float fringe=draw->_FringeScale;
    const auto color=ImGui::GetColorU32(ImVec4(1,1,1,1));
    draw->PrimReserve(segments*18,(segments+1)*4);
    const auto first=draw->_VtxCurrentIdx;
    for(int i=0;i<segments;++i) for(int band=0;band<3;++band) {
        const auto a=first+i*4+band,b=a+4;
        for(const auto index:{a,b,b+1,a,b+1,a+1}) draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
    }
    for(int i=0;i<=segments;++i) {
        const float turn=sweep*i/segments;
        const auto point=FluidContour(turn,blend,rotation,radius);
        const auto tangent=FluidContour(ImMin(1.f,turn+.0005f),blend,rotation,radius)-FluidContour(ImMax(0.f,turn-.0005f),blend,rotation,radius);
        const float inverse=1.f/ImMax(.00001f,std::sqrt(ImLengthSqr(tangent)));
        const ImVec2 normal(-tangent.y*inverse,tangent.x*inverse);
        const auto inner=point*hole;
        const auto alpha=static_cast<ImU32>(((color>>IM_COL32_A_SHIFT)&255)*ImLerp(.30f+.70f*i/segments,1.f,blend));
        const auto solid=(color&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT),clear=color&~IM_COL32_A_MASK;
        for(const auto& vertex:{ImDrawVert{center+point+normal*fringe,draw->_Data->TexUvWhitePixel,clear},
                ImDrawVert{center+point,draw->_Data->TexUvWhitePixel,solid},
                ImDrawVert{center+inner,draw->_Data->TexUvWhitePixel,solid},
                ImDrawVert{center+inner-normal*ImMin(fringe,radius*hole),draw->_Data->TexUvWhitePixel,clear}})
            draw->PrimWriteVtx(vertex.pos,vertex.uv,vertex.col);
    }
    if(sweep<.99999f) for(float turn:{0.f,sweep}) {
        const auto point=FluidContour(turn,blend,rotation,radius);
        draw->AddCircleFilled(center+point*((1.f+hole)*.5f),std::sqrt(ImLengthSqr(point))*(1.f-hole)*.5f,
            ImGui::GetColorU32(ImVec4(1,1,1,ImLerp(turn==0.f?.30f:1.f,1.f,blend))),12);
    }
}
inline void Splash(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,float progress) {
    const float t=ImClamp(progress,0.f,1.f);
    const auto impact=lo+ImVec2(size.x*.5f,size.y-1.5f*scale);
    const float flatten=Smooth(ImClamp(t/.22f,0.f,1.f));
    if(flatten<1.f) {
        const int first=draw->VtxBuffer.Size;
        const float radius=size.y*.26f;
        FluidLoop(draw,impact-ImVec2(0,radius),radius,1.f,0.f);
        for(int i=first;i<draw->VtxBuffer.Size;++i) {
            const auto offset=draw->VtxBuffer[i].pos-impact;
            draw->VtxBuffer[i].pos=impact+ImVec2(offset.x*(1.f+flatten*2.f),offset.y*(1.f-flatten));
        }
        Fade(draw,first,1.f-flatten);
    }
    const float emergence=Smooth(ImClamp(t/.08f,0.f,1.f));
    const float crown=std::sin(IM_PI*ImClamp(t/.75f,0.f,1.f))*19.f*scale;
    const float width=(12.f+49.f*t)*scale;
    const float opacity=(1.f-Smooth((t-.35f)/.65f))*emergence;
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
        draw->AddCircleFilled(center,radius,ImGui::GetColorU32(ImVec4(1,1,1,(1.f-Smooth((t-.58f)/.42f))*emergence)),10);
    }
    const float rippleWidth=(13.f+104.f*t)*scale;
    const float rippleHeight=(1.3f+3.f*t)*scale;
    for(int i=0;i<=28;++i) {
        const float angle=IM_PI+IM_PI*i/28.f;
        draw->PathLineTo(impact+ImVec2(std::cos(angle)*rippleWidth,std::sin(angle)*rippleHeight));
    }
    draw->PathStroke(ImGui::GetColorU32(ImVec4(1,1,1,(1.f-t)*.9f*emergence)),0,1.2f*scale);
}
inline void Wave(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,float progress) {
    const float t=ImClamp(progress,0.f,1.f);
    if(t<=0.f) return;
    const float radius=size.y*.5f;
    const float amplitude=size.y*.19f*std::sin(IM_PI*t);
    const float level=lo.y+size.y*(1.f-t);
    const auto surface=[&](float x,float phase,float strength) {
        const float u=(x-lo.x)/size.x;
        return level+amplitude*strength*(std::sin(u*IM_PI*3.f-t*IM_PI*2.f+phase)+
            .28f*std::sin(u*IM_PI*5.f+t*IM_PI*2.8f+phase));
    };
    constexpr int cap=16,middle=64,segments=cap*2+middle;
    const float cy=lo.y+radius;
    const auto xAt=[&](int index) {
        if(index<=cap) return lo.x+radius*(1.f-std::cos(index*(IM_PI*.5f/cap)));
        if(index>=cap+middle) return lo.x+size.x-radius+radius*std::sin((index-cap-middle)*(IM_PI*.5f/cap));
        return lo.x+radius+(size.x-size.y)*(index-cap)/middle;
    };
    const auto paint=[&](float phase,float strength,ImU32 color) {
        std::array<ImVec2,segments+1> top,bottom;
        for(int i=0;i<=segments;++i) {
            const float x=xAt(i),nearest=ImClamp(x,lo.x+radius,lo.x+size.x-radius);
            const float dx=x-nearest,half=std::sqrt(ImMax(0.f,radius*radius-dx*dx));
            top[i]={x,ImClamp(surface(x,phase,strength),cy-half,cy+half)};
            bottom[i]={x,cy+half};
        }
        draw->PrimReserve(segments*18,(segments+1)*4);
        const auto first=draw->_VtxCurrentIdx;
        for(int i=0;i<segments;++i) for(int band=0;band<3;++band) {
            const auto a=first+i*4+band,b=a+4;
            for(const auto index:{a,b,b+1,a,b+1,a+1}) draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
        }
        const auto clear=color&~IM_COL32_A_MASK;
        for(int i=0;i<=segments;++i) {
            const int before=ImMax(0,i-1),after=ImMin(segments,i+1);
            const auto upper=top[after]-top[before],lower=bottom[after]-bottom[before];
            const float upperLength=ImMax(.00001f,std::sqrt(ImLengthSqr(upper))),lowerLength=ImMax(.00001f,std::sqrt(ImLengthSqr(lower)));
            const ImVec2 upperFringe(upper.y/upperLength,-upper.x/upperLength),lowerFringe(-lower.y/lowerLength,lower.x/lowerLength);
            const auto solid=bottom[i].y-top[i].y>.001f?color:clear;
            for(const auto& vertex:{ImDrawVert{top[i]+upperFringe*draw->_FringeScale,draw->_Data->TexUvWhitePixel,clear},
                    ImDrawVert{top[i],draw->_Data->TexUvWhitePixel,solid},ImDrawVert{bottom[i],draw->_Data->TexUvWhitePixel,solid},
                    ImDrawVert{bottom[i]+lowerFringe*draw->_FringeScale,draw->_Data->TexUvWhitePixel,clear}})
                draw->PrimWriteVtx(vertex.pos,vertex.uv,vertex.col);
        }
    };
    paint(1.6f,.95f,ImGui::GetColorU32(ImVec4(.86f,.80f,1.f,.40f)));
    paint(0.f,1.f,ImGui::GetColorU32(ImVec4(1,1,1,1)));
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
                                      const ButtonFrame& frame,ImFont* body) {
    if(!draw || size.x<=0.f || size.y<=0.f) return;
    if(!body) body=ImGui::GetFont();
    const auto center=lo+size*.5f;
    if(frame.stage==ButtonStage::Idle || frame.stage==ButtonStage::LabelFade) {
        const float textSize=17.f*scale;
        const auto label=body->CalcTextSizeA(textSize,FLT_MAX,0.f,"Authenticate");
        button_art::Text(draw,body,textSize,{center.x-label.x*.5f,lo.y+(size.y-label.y)*.5f},
            ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,ImClamp(frame.labelOpacity,0.f,1.f))),"Authenticate");
    }
    draw->PushClipRect(lo,lo+size,true);
    if(frame.stage>=ButtonStage::Collapse && frame.stage<=ButtonStage::Fall) {
        const float morph=frame.stage<=ButtonStage::Spin?0.f:frame.dropletMorph;
        const float fall=frame.stage==ButtonStage::Fall?ImClamp(frame.fall,0.f,1.f):0.f;
        const float radius=size.y*.26f;
        const float y=ImLerp(center.y,lo.y+size.y-radius-1.5f*scale,fall*fall);
        const int first=draw->VtxBuffer.Size;
        button_art::FluidLoop(draw,{center.x,y},radius,morph,frame.spinnerRotation);
        if(frame.stage==ButtonStage::Collapse) button_art::Fade(draw,first,button_art::Smooth(frame.collapse));
    }
    if(frame.stage==ButtonStage::Splash) button_art::Splash(draw,lo,size,scale,frame.splash);
    if(frame.stage==ButtonStage::Wave || frame.stage==ButtonStage::Check)
        button_art::Wave(draw,lo,size,scale,frame.stage==ButtonStage::Check?1.f:frame.wave);
    draw->PopClipRect();
    if(frame.stage==ButtonStage::Check) button_art::Check(draw,center,scale,frame.check);
}
}
