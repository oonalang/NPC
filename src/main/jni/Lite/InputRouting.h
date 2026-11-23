#pragma once
#include "TouchInput.h"
#include <array>
#include <mutex>

namespace lite_routing {
inline constexpr std::size_t maximumRegions=16;
struct Regions {
    std::array<ImRect,maximumRegions> rectangles{};
    std::size_t count=0;
};
inline Regions VisibleRegions() {
    Regions result;
    const auto display=ImGui::GetIO().DisplaySize;
    if(display.x<=0 || display.y<=0) return result;
    for(const auto* window:GImGui->Windows) {
        if(!window->Active || window->Hidden || window->IsFallbackWindow ||
            (window->Flags&(ImGuiWindowFlags_ChildWindow|ImGuiWindowFlags_NoMouseInputs))) continue;
        const auto lo=ImClamp(window->Pos,ImVec2(0,0),display);
        const auto hi=ImClamp(window->Pos+window->Size,ImVec2(0,0),display);
        if(lo.x>=hi.x || lo.y>=hi.y) continue;
        result.rectangles[result.count++]={{lo.x/display.x,lo.y/display.y},{hi.x/display.x,hi.y/display.y}};
        if(result.count==maximumRegions) break;
    }
    return result;
}
class Router {
    std::mutex mutex;
    Regions regions;
    int finger=0;
    bool known=false,ui=false,ended=false;
public:
    void Publish(const Regions& value) {
        std::lock_guard<std::mutex> lock(mutex);regions=value;
        regions.count=ImMin(regions.count,maximumRegions);
    }
    int Filter(int count,int id,ImVec2 normalized,lite_touch::Phase phase) {
        std::lock_guard<std::mutex> lock(mutex);
        if(count<=0) { known=ui=ended=false;return 0; }
        if(!known || id!=finger || (ended && phase==lite_touch::Phase::Began)) {
            ui=false;finger=id;known=true;ended=false;
            if(std::isfinite(normalized.x) && std::isfinite(normalized.y))
                for(std::size_t index=0;index<regions.count;++index)
                    if(regions.rectangles[index].Contains(normalized)) { ui=true;break; }
        }
        if(phase==lite_touch::Phase::Ended || phase==lite_touch::Phase::Canceled) ended=true;
        return ui?0:count;
    }
};
}
