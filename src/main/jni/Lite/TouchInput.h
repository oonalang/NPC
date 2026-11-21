#pragma once
#include "../ImGui/TouchScroll.h"
#include <cmath>

namespace lite_touch {
enum class Phase { Began, Moved, Stationary, Ended, Canceled };
struct Sample { int finger; ImVec2 position; Phase phase; };
class Input {
    bool held=false, clear=false, blocked=false;
    int finger=0;
    ImVec2 source{},display{};
    void Release(ImGuiIO& io, bool cancel) {
        if(cancel) {
            custom::touch::Cancel();
            ImGui::ClearActiveID();
            io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
        }
        io.AddMouseButtonEvent(0,false);
        held=false;
        clear=true;
    }
public:
    static void Configure(ImGuiIO& io) { io.ConfigInputTrickleEventQueue=false; }
    void Update(ImGuiIO& io, const Sample* sample, ImVec2 resolution) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        if(!sample) {
            blocked=false;
            if(held) Release(io,false);
            else if(clear && !io.MouseDown[0]) {
                io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
                clear=false;
            }
            return;
        }
        if(blocked) return;
        const bool valid=std::isfinite(resolution.x) && std::isfinite(resolution.y) &&
            std::isfinite(io.DisplaySize.x) && std::isfinite(io.DisplaySize.y) &&
            std::isfinite(sample->position.x) && std::isfinite(sample->position.y) &&
            resolution.x>0.f && resolution.y>0.f && io.DisplaySize.x>0.f && io.DisplaySize.y>0.f;
        if(!valid || (held && (finger!=sample->finger || source.x!=resolution.x || source.y!=resolution.y ||
            display.x!=io.DisplaySize.x || display.y!=io.DisplaySize.y)) || sample->phase==Phase::Canceled) {
            Release(io,true);
            blocked=true;
            return;
        }
        const ImVec2 point(sample->position.x*(io.DisplaySize.x/resolution.x),
            (resolution.y-sample->position.y)*(io.DisplaySize.y/resolution.y));
        if(!std::isfinite(point.x) || !std::isfinite(point.y)) {
            Release(io,true); blocked=true; return;
        }
        if(sample->phase==Phase::Ended) {
            if(held) { io.AddMousePosEvent(point.x,point.y); Release(io,false); }
            return;
        }
        if(!held) { finger=sample->finger;source=resolution;display=io.DisplaySize; }
        io.AddMousePosEvent(point.x,point.y);
        io.AddMouseButtonEvent(0,true);
        held=true;
        clear=false;
    }
};
}
