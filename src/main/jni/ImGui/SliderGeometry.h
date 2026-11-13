#pragma once
#include "imgui_internal.h"

namespace custom {
struct SliderGeometry {
    float start,end,fill,center,halfWidth;
};
inline SliderGeometry MakeSliderGeometry(float left,float right,float halfWidth,float ratio) {
    ratio=ImClamp(ratio,0.f,1.f);
    const float width=ImMax(0.f,right-left);
    halfWidth=ImMin(ImMax(0.f,halfWidth),width*.5f);
    return {left,right,left+width*ratio,ImLerp(left+halfWidth,right-halfWidth,ratio),halfWidth};
}
}
