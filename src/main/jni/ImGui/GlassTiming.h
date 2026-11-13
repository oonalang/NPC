#pragma once
#include <cmath>

namespace clay {
struct CaptureSchedule {
    double last = -1.0;
    bool Due(double now, bool moving, bool force = false) {
        if (!std::isfinite(now)) return false;
        const double interval = moving ? 1.0 / 20.0 : 1.0 / 30.0;
        if (force || last < 0.0 || now < last || now - last >= interval - 0.000001) {
            last = now;
            return true;
        }
        return false;
    }
    void Reset() { last = -1.0; }
};
}
