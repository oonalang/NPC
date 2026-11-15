#pragma once
#include <algorithm>
#include <cmath>
namespace anime_preview {
struct OrbitControl {
    bool enabled=false,pending=false,dragging=false;
    float yaw=0,startX=0,startY=0,startYaw=0;
    void Toggle() {enabled=!enabled;yaw=0;pending=dragging=false;}
    void Update(bool pressed,bool down,bool hovered,float x,float y,float width,bool canceled=false) {
        if(!enabled||!down||canceled) {pending=dragging=false;return;}
        if(pressed&&hovered) {pending=true;dragging=false;startX=x;startY=y;startYaw=yaw;}
        if(!pending&&!dragging) return;
        const float dx=x-startX,dy=y-startY;
        if(pending&&dx*dx+dy*dy>=49) {
            pending=false;
            if(std::fabs(dy)>=std::fabs(dx)) return;
            dragging=true;
        }
        if(dragging) yaw=std::max(-1.570796327f,std::min(1.570796327f,startYaw+dx*2.4f/std::max(1.f,width)));
    }
};
}
