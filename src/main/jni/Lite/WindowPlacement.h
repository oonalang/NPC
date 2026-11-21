#pragma once
#include "../ImGui/imgui_internal.h"

namespace lite_placement {
class Placement {
    ImVec2 position{},start{},origin{},display{};
    bool moved=false,tracking=false,dragging=false;
    int frame=-1;
public:
    bool Tracking() const { return tracking; }
    ImVec2 Position(ImVec2 screen,ImVec2 size) const {
        return ImClamp(moved?position:(screen-size)*.5f,ImVec2(0,0),ImMax(ImVec2(0,0),screen-size));
    }
    void Update(int currentFrame,bool enabled,ImVec2 screen,ImVec2 size,ImRect header,
        ImVec2 pointer,bool pressed,bool down,float threshold) {
        if(frame==currentFrame) return;
        frame=currentFrame;
        if(!enabled || !down || !ImGui::IsMousePosValid(&pointer)) { tracking=dragging=false;return; }
        if(pressed) {
            tracking=header.Contains(pointer);dragging=false;start=pointer;origin=Position(screen,size);display=screen;
        }
        if(!tracking) return;
        if(screen.x!=display.x || screen.y!=display.y) { tracking=false;return; }
        const auto delta=pointer-start;
        if(ImLengthSqr(delta)>=threshold*threshold || dragging) {
            position=ImClamp(origin+delta,ImVec2(0,0),ImMax(ImVec2(0,0),screen-size));moved=dragging=true;
        }
    }
};
}
