#pragma once
#include <cmath>

namespace anime_preview {
class PreviewClock {
    double previous_=0,elapsed_=0;
    int frame_=-1;
    bool ready_=false;
public:
    struct Tick {double seconds;bool reset;};
    Tick Update(double now,int frame,bool ready) {
        if(!std::isfinite(now)||now<0) now=0;
        if(frame==frame_&&ready==ready_&&now>=previous_) return {elapsed_,false};
        const bool reset=frame_<0||frame<frame_||frame>frame_+1||now<previous_||now-previous_>.5||ready!=ready_;
        if(reset) elapsed_=0;
        else if(ready) elapsed_+=now-previous_;
        previous_=now;frame_=frame;ready_=ready;
        return {elapsed_,reset};
    }
};
}
