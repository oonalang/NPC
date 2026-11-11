#include "../src/main/jni/Trigger/TriggerDotStyle.h"
#include <cassert>
#include <limits>

int main() {
    using TriggerDotStyle::Appearance;
    Appearance defaults;
    assert(defaults.radius == 3.0f);
    assert(defaults.outline == 1.0f);
    assert(defaults.opacity == 1.0f);

    Appearance changed;
    changed.radius = 99.0f;
    changed.outline = -2.0f;
    changed.opacity = std::numeric_limits<float>::quiet_NaN();
    changed.color[0] = -1.0f;
    changed.color[1] = 2.0f;
    changed.color[2] = std::numeric_limits<float>::infinity();
    TriggerDotStyle::Normalize(changed);

    assert(changed.radius == 12.0f);
    assert(changed.outline == 0.0f);
    assert(changed.opacity == 1.0f);
    assert(changed.color[0] == 0.0f);
    assert(changed.color[1] == 1.0f);
    assert(changed.color[2] == 1.0f);
    return 0;
}
