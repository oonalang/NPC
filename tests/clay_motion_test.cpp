#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/ImGui/imgui_internal.h"
#include "../src/main/jni/ImGui/ClayGlass.h"
#include <cassert>

int main() {
    clay::Spring fast, slow;
    for (int i = 0; i < 120; ++i) fast.Step(1.f, 1.f / 120);
    for (int i = 0; i < 30; ++i) slow.Step(1.f, 1.f / 30);
    assert(std::abs(fast.value - 1.f) < .0001f);
    assert(std::abs(fast.value - slow.value) < .0001f);

    fast.Step(0.f, 1.f / 60);
    float previous = fast.value;
    for (int i = 0; i < 120; ++i) {
        fast.Step(0.f, 1.f / 120);
        assert(fast.value <= previous + .00001f && fast.value >= 0.f);
        previous = fast.value;
    }
    fast.Step(1.f, 100.f);
    assert(std::isfinite(fast.value) && fast.value <= 1.f);
    const float before = fast.value;
    fast.Step(0.f, -1.f);
    assert(fast.value == before);
}
