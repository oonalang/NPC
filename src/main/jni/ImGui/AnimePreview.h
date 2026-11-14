#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace anime_preview {
struct Rig {
    ImVec2 contentMin{.155f,.053f},contentMax{.845f,.96f};
    ImVec2 head{.5f,.172f},neck{.5f,.236f},chest{.5f,.309f},hip{.5f,.50f};
    ImVec2 leftEye{.478f,.168f},rightEye{.531f,.168f};
    std::array<ImVec2,2> shoulder{{{.401f,.255f},{.599f,.255f}}};
    std::array<ImVec2,2> elbow{{{.318f,.36f},{.682f,.36f}}};
    std::array<ImVec2,2> hand{{{.213f,.52f},{.787f,.52f}}};
    std::array<ImVec2,2> knee{{{.443f,.703f},{.557f,.703f}}};
    std::array<ImVec2,2> ankle{{{.447f,.915f},{.553f,.915f}}};
};
struct Bounds {
    ImVec2 min,max,head,neck,chest,hip,knee,feet;
    std::array<ImVec2,2> shoulder,elbow,hand,knees,ankle;
    float height=0.f,jump=0.f;
    bool excited=false;
};
struct Spring {
    float position=0.f,velocity=0.f;
    void Reset() { position=velocity=0.f; }
    void Step(float target,float seconds,float frequency,float damping,float limit) {
        const int steps=ImClamp(static_cast<int>(std::ceil(seconds/.004f)),1,24);
        const float dt=seconds/steps;
        for(int i=0;i<steps;++i) {
            velocity+=((target-position)*frequency*frequency-2.f*damping*frequency*velocity)*dt;
            position=ImClamp(position+velocity*dt,-limit,limit);
            velocity=ImClamp(velocity,-.8f,.8f);
        }
    }
};
struct Pose {
    float bodyY=0.f,sway=0.f,squash=1.f,cheer=0.f,blink=0.f;
    float chestY=0.f,hairX=0.f,hairY=0.f,skirtX=0.f;
    float shadow=1.f;
    bool excited=false;
};
struct Motion {
    Spring chest,hairX,hairY,skirt;
    double last=-1.;
};
inline float Smooth(float t) { t=ImClamp(t,0.f,1.f);return t*t*(3.f-2.f*t); }
inline float Window(float t,float lo,float hi,float edge) {
    return Smooth((t-lo)/edge)*(1.f-Smooth((t-(hi-edge))/edge));
}
inline Pose Sample(double time,float delta,Motion& state) {
    Pose pose;
    if(!std::isfinite(time)) time=0.;
    const double elapsed=std::max(0.,time);
    const float breathePhase=static_cast<float>(std::fmod(elapsed*1.8,6.283185307179586));
    const float swayPhase=static_cast<float>(std::fmod(elapsed*.85,6.283185307179586));
    const float hairPhase=static_cast<float>(std::fmod(elapsed*1.2,6.283185307179586));
    const float cycle=static_cast<float>(std::fmod(elapsed,7.2));
    const float anticipation=cycle>=4.35f && cycle<4.57f?Smooth((cycle-4.35f)/.22f):0.f;
    const float flight=(cycle-4.57f)/.72f;
    const bool airborne=flight>=0.f && flight<=1.f;
    const float landing=cycle>5.29f && cycle<5.52f?std::sin((cycle-5.29f)/.23f*IM_PI):0.f;
    const float lift=airborne?4.f*.115f*flight*(1.f-flight):0.f;
    pose.bodyY=-lift+std::sin(breathePhase)*.0018f;
    pose.sway=std::sin(swayPhase)*.007f;
    pose.cheer=airborne?std::sin(IM_PI*flight):anticipation*.15f;
    pose.squash=1.f-.045f*anticipation+.018f*pose.cheer-.038f*landing;
    pose.excited=airborne || anticipation>0.f || landing>.1f;
    pose.shadow=1.f-lift*3.6f;
    const float blinkCycle=static_cast<float>(std::fmod(elapsed,3.87));
    const float blink=blinkCycle>=3.67f?std::sin((blinkCycle-3.67f)/.20f*IM_PI):0.f;
    pose.blink=ImClamp(std::max(blink,airborne?.72f*pose.cheer:0.f),0.f,1.f);
    const bool discontinuity=state.last<0. || time<state.last || time-state.last>.20;
    float dt=std::isfinite(delta)?ImClamp(delta,0.f,.05f):0.f;
    if(discontinuity) {
        state.chest.Reset();state.hairX.Reset();state.hairY.Reset();state.skirt.Reset();
        dt=0.f;
    }
    if(time==state.last) dt=0.f;
    const float velocity=(airborne?-4.f*.115f*(1.f-2.f*flight)/.72f:0.f)+std::cos(breathePhase)*.00324f;
    const float acceleration=(airborne?8.f*.115f/(.72f*.72f):0.f)-std::sin(breathePhase)*.005832f;
    if(dt>0.f) {
        const auto crossed=[&](double event) {
            return std::floor((time-event)/7.2)>std::floor((state.last-event)/7.2);
        };
        if(crossed(4.57) || crossed(5.29)) state.chest.velocity+=.15f;
        state.chest.Step(-acceleration*.0026f,dt,22.f,.33f,.018f);
        state.hairY.Step(-velocity*.043f,dt,15.f,.42f,.032f);
        state.hairX.Step(std::sin(hairPhase)*.009f+velocity*.012f,dt,11.f,.50f,.025f);
        state.skirt.Step(-velocity*.018f,dt,17.f,.48f,.020f);
    }
    pose.chestY=state.chest.position;
    pose.hairX=state.hairX.position;
    pose.hairY=state.hairY.position;
    pose.skirtX=state.skirt.position;
    state.last=time;
    return pose;
}
struct VertexWeight {
    ImVec2 uv;
    float chest=0.f,hair=0.f,skirt=0.f,arm=0.f,side=0.f;
    std::array<float,2> eye{};
};
struct Artwork {
    ImTextureID texture=nullptr;
    int width=1024,height=1536;
    Rig rig;
    bool rebuild=true;
};
inline Artwork artwork;
inline Motion motion;
inline std::vector<VertexWeight> weights;
inline constexpr int columns=32,rows=112;
inline void SetArtwork(ImTextureID texture,int width,int height) {
    artwork.texture=texture;
    artwork.width=std::max(1,width);artwork.height=std::max(1,height);
}
inline void SetRig(const Rig& rig) { artwork.rig=rig;artwork.rebuild=true; }
inline float Gaussian(float value,float center,float spread) {
    const float d=(value-center)/spread;
    return std::exp(-d*d*.5f);
}
inline void BuildWeights() {
    if(!artwork.rebuild && weights.size()==static_cast<std::size_t>((columns+1)*(rows+1))) return;
    weights.clear();weights.reserve((columns+1)*(rows+1));
    const auto& rig=artwork.rig;
    for(int y=0;y<=rows;++y) for(int x=0;x<=columns;++x) {
        VertexWeight w;
        w.uv={static_cast<float>(x)/columns,static_cast<float>(y)/rows};
        w.chest=Gaussian(w.uv.x,rig.chest.x,.105f)*Gaussian(w.uv.y,rig.chest.y,.040f);
        const float outside=Smooth((std::fabs(w.uv.x-rig.head.x)-.075f)/.10f);
        w.hair=outside*Window(w.uv.y,.015f,.41f,.08f);
        w.skirt=Window(w.uv.y,.355f,.64f,.12f)*Smooth(std::fabs(w.uv.x-.5f)/.22f);
        w.side=w.uv.x<.5f?-1.f:1.f;
        w.arm=Smooth((std::fabs(w.uv.x-.5f)-.12f)/.11f)*Window(w.uv.y,.23f,.52f,.095f);
        for(int i=0;i<2;++i) {
            const auto eye=i==0?rig.leftEye:rig.rightEye;
            w.eye[i]=Gaussian(w.uv.x,eye.x,.029f)*Gaussian(w.uv.y,eye.y,.011f);
        }
        weights.push_back(w);
    }
    artwork.rebuild=false;
}
inline VertexWeight WeightAt(ImVec2 uv) {
    VertexWeight w;w.uv=uv;
    const auto& rig=artwork.rig;
    w.chest=Gaussian(uv.x,rig.chest.x,.105f)*Gaussian(uv.y,rig.chest.y,.040f);
    w.hair=Smooth((std::fabs(uv.x-rig.head.x)-.075f)/.10f)*Window(uv.y,.015f,.41f,.08f);
    w.skirt=Window(uv.y,.355f,.64f,.12f)*Smooth(std::fabs(uv.x-.5f)/.22f);
    w.side=uv.x<.5f?-1.f:1.f;
    w.arm=Smooth((std::fabs(uv.x-.5f)-.12f)/.11f)*Window(uv.y,.23f,.52f,.095f);
    for(int i=0;i<2;++i) {
        const auto eye=i==0?rig.leftEye:rig.rightEye;
        w.eye[i]=Gaussian(uv.x,eye.x,.029f)*Gaussian(uv.y,eye.y,.011f);
    }
    return w;
}
struct Transform {
    ImVec2 imageSize,origin;
    float height=0.f;
    Pose pose;
    ImVec2 Apply(const VertexWeight& w) const {
        auto uv=w.uv;
        const auto& rig=artwork.rig;
        uv.y+=pose.chestY*w.chest;
        uv.x+=pose.hairX*w.hair+pose.skirtX*w.skirt*w.side;
        uv.y+=pose.hairY*w.hair;
        const auto shoulder=rig.shoulder[w.side<0?0:1];
        const auto arm=uv-shoulder;
        const float angle=-w.side*pose.cheer*.20f;
        const float sine=std::sin(angle),cosine=std::cos(angle);
        uv+=ImVec2(arm.x*cosine-arm.y*sine-arm.x,arm.x*sine+arm.y*cosine-arm.y)*w.arm;
        for(int i=0;i<2;++i) {
            const auto eye=i==0?rig.leftEye:rig.rightEye;
            uv.y-=(w.uv.y-eye.y)*pose.blink*.88f*w.eye[i];
        }
        const float horizontal=1.f/std::sqrt(pose.squash);
        return origin+ImVec2((uv.x-.5f)*imageSize.x*horizontal+pose.sway*height,
            (uv.y-rig.contentMax.y)*imageSize.y*pose.squash+pose.bodyY*height);
    }
    ImVec2 Apply(ImVec2 uv) const { return Apply(WeightAt(uv)); }
};
inline void Silhouette(ImDrawList* draw,const Transform& transform) {
    const auto P=[&](float x,float y) { return transform.Apply(ImVec2(x,y)); };
    const auto skin=IM_COL32(245,216,207,255),hair=IM_COL32(171,144,215,255);
    const auto dress=IM_COL32(112,82,154,255),shade=IM_COL32(67,50,95,255);
    draw->AddLine(P(.43f,.59f),P(.39f,.96f),shade,transform.height*.046f);
    draw->AddLine(P(.56f,.59f),P(.57f,.96f),shade,transform.height*.046f);
    const ImVec2 skirt[]{P(.41f,.34f),P(.59f,.34f),P(.68f,.64f),P(.32f,.64f)};
    draw->AddConvexPolyFilled(skirt,4,dress);
    draw->AddLine(P(.395f,.19f),P(.285f,.39f),dress,transform.height*.052f);
    draw->AddLine(P(.285f,.39f),P(.23f,.47f),skin,transform.height*.025f);
    draw->AddLine(P(.605f,.19f),P(.715f,.39f),dress,transform.height*.052f);
    draw->AddLine(P(.715f,.39f),P(.77f,.47f),skin,transform.height*.025f);
    const ImVec2 torso[]{P(.395f,.18f),P(.605f,.18f),P(.59f,.35f),P(.41f,.35f)};
    draw->AddConvexPolyFilled(torso,4,dress);
    draw->AddBezierCubic(P(.43f,.07f),P(.29f,.18f),P(.33f,.29f),P(.29f,.39f),hair,transform.height*.045f,20);
    draw->AddBezierCubic(P(.56f,.07f),P(.67f,.18f),P(.64f,.29f),P(.70f,.39f),hair,transform.height*.045f,20);
    draw->AddCircleFilled(P(.493f,.104f),transform.height*.065f,hair,28);
    draw->AddCircleFilled(P(.493f,.116f),transform.height*.052f,skin,28);
    for(const auto eye : {artwork.rig.leftEye,artwork.rig.rightEye}) {
        const auto p=P(eye.x,eye.y+.012f);
        draw->AddLine(p-ImVec2(3,0),p+ImVec2(3,0),shade,1.5f);
    }
}
inline Bounds Draw(ImDrawList* draw,ImVec2 viewMin,ImVec2 viewMax,double time,float delta) {
    Bounds bounds;
    if(!draw || viewMax.x<=viewMin.x || viewMax.y<=viewMin.y) return bounds;
    BuildWeights();
    const auto& rig=artwork.rig;
    const float contentHeight=ImMax(.1f,rig.contentMax.y-rig.contentMin.y);
    const float aspect=static_cast<float>(artwork.width)/artwork.height/contentHeight;
    const float height=ImMin((viewMax.y-viewMin.y)*.80f,(viewMax.x-viewMin.x)*.90f/aspect);
    const float floor=viewMax.y-12.f;
    const auto pose=Sample(time,delta,motion);
    Transform transform{{height*aspect,height/contentHeight},{(viewMin.x+viewMax.x)*.5f,floor},height,pose};
    const auto shadow=ImGui::GetColorU32(ImVec4(.28f,.20f,.42f,.20f*pose.shadow));
    const auto shadowCenter=ImVec2(transform.origin.x+pose.sway*height,floor+3.f);
    for(int i=0;i<32;++i) {
        const float angle=IM_PI*2.f*i/32.f;
        draw->PathLineTo(shadowCenter+ImVec2(std::cos(angle)*height*.16f*pose.shadow,std::sin(angle)*height*.018f));
    }
    draw->PathFillConvex(shadow);
    draw->PushClipRect(viewMin,viewMax,true);
    if(artwork.texture) {
        draw->PushTextureID(artwork.texture);
        const int vertexCount=(columns+1)*(rows+1),indexCount=columns*rows*6;
        draw->PrimReserve(indexCount,vertexCount);
        const auto first=draw->_VtxCurrentIdx;
        for(int y=0;y<rows;++y) for(int x=0;x<columns;++x) {
            const auto a=first+y*(columns+1)+x,b=a+1,c=a+columns+2,d=a+columns+1;
            for(const auto index : {a,b,c,a,c,d}) draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
        }
        const auto color=ImGui::GetColorU32(ImVec4(1,1,1,1));
        for(const auto& w : weights) draw->PrimWriteVtx(transform.Apply(w),w.uv,color);
        draw->PopTextureID();
    } else Silhouette(draw,transform);
    draw->PopClipRect();
    bounds.min={FLT_MAX,FLT_MAX};bounds.max={-FLT_MAX,-FLT_MAX};
    for(const auto& w : weights) if(w.uv.x>=rig.contentMin.x && w.uv.x<=rig.contentMax.x &&
        w.uv.y>=rig.contentMin.y && w.uv.y<=rig.contentMax.y) {
        const auto p=transform.Apply(w);
        bounds.min=ImMin(bounds.min,p);bounds.max=ImMax(bounds.max,p);
    }
    bounds.head=transform.Apply(rig.head);bounds.neck=transform.Apply(rig.neck);
    bounds.chest=transform.Apply(rig.chest);bounds.hip=transform.Apply(rig.hip);
    for(int i=0;i<2;++i) {
        bounds.shoulder[i]=transform.Apply(rig.shoulder[i]);bounds.elbow[i]=transform.Apply(rig.elbow[i]);
        bounds.hand[i]=transform.Apply(rig.hand[i]);bounds.knees[i]=transform.Apply(rig.knee[i]);
        bounds.ankle[i]=transform.Apply(rig.ankle[i]);
    }
    bounds.knee=(bounds.knees[0]+bounds.knees[1])*.5f;
    bounds.feet=transform.Apply(ImVec2(.5f,rig.contentMax.y));
    bounds.height=height;bounds.jump=-pose.bodyY*height;bounds.excited=pose.excited;
    return bounds;
}
}
