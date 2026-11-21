#pragma once
#include "../ImGui/imgui_internal.h"
#include <cmath>

namespace lite_restore {
inline constexpr float lineWidth=120.f,lineHeight=6.f,bottomGap=10.f;
inline constexpr float hitPadding=72.f,minimumHitHeight=44.f;
inline constexpr float swipeDistance=48.f,minimumDistance=32.f,maximumDistance=64.f;
inline constexpr float directionTolerance=12.f,horizontalRatio=.75f;
struct Geometry {
    ImVec2 display{};
    ImRect line{},hit{};
    float travel=swipeDistance,slop=directionTolerance;
};
inline Geometry Fit(ImVec2 display,float scale) {
    Geometry result;
    if(!std::isfinite(display.x) || !std::isfinite(display.y) || display.x<=16.f || display.y<=2.f) return result;
    scale=std::isfinite(scale)?ImClamp(scale,.5f,1.45f):1.f;
    result.display=display;
    const float width=ImMin(lineWidth*scale,display.x-16.f);
    const float bottom=display.y-bottomGap*scale;
    result.line={{(display.x-width)*.5f,bottom-lineHeight*scale},{(display.x+width)*.5f,bottom}};
    const float hitWidth=ImMin(width+hitPadding*scale,ImMax(1.f,display.x-8.f));
    const float hitHeight=ImMax(minimumHitHeight,28.f*scale);
    result.hit={{(display.x-hitWidth)*.5f,ImMax(0.f,display.y-hitHeight-2.f)},
        {(display.x+hitWidth)*.5f,display.y-2.f}};
    result.travel=ImClamp(swipeDistance*scale,minimumDistance,maximumDistance);
    result.slop=ImMax(10.f,directionTolerance*scale);
    return result;
}
class Swipe {
    enum class Phase { Idle,Tracking,Rejected,Consumed };
    Phase phase=Phase::Idle;
    ImVec2 start{},display{};
    float progress=0.f;
public:
    bool Captures() const { return phase!=Phase::Idle; }
    float Progress() const { return progress; }
    bool Update(bool hidden,const Geometry& geometry,ImVec2 position,bool pressed,bool down,bool released) {
        if(!hidden) {
            if(phase!=Phase::Consumed || !down) { phase=Phase::Idle;progress=0.f; }
            return false;
        }
        const bool valid=std::isfinite(position.x) && std::isfinite(position.y) &&
            ImGui::IsMousePosValid(&position) && geometry.display.x>0.f && geometry.display.y>0.f;
        if(pressed) {
            phase=valid && geometry.hit.Contains(position)?Phase::Tracking:Phase::Idle;
            start=position;display=geometry.display;progress=0.f;
        }
        bool reveal=false;
        if(phase==Phase::Tracking) {
            const float horizontal=std::fabs(position.x-start.x),up=start.y-position.y;
            if(!valid || display.x!=geometry.display.x || display.y!=geometry.display.y ||
                up < -geometry.slop || (horizontal>geometry.slop && horizontal>ImMax(0.f,up)*horizontalRatio)) {
                phase=Phase::Rejected;progress=0.f;
            } else {
                progress=ImClamp(up/geometry.travel,0.f,1.f);
                if((down || released) && up>=geometry.travel && horizontal<=up*horizontalRatio) {
                    phase=Phase::Consumed;reveal=true;
                }
            }
        }
        if(!down) { phase=Phase::Idle;progress=0.f; }
        return reveal;
    }
};
}
