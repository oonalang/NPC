#include "../src/main/jni/System/Hooks/PickupTweaks.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

static float NativeInterval(void*) { return 0.4f; }

int main() {
    using namespace pickup_tweaks;

    std::array<unsigned char, 256> memory{};
    const float baseHorizontal = 16.0f;
    const float baseVertical = 2.5f;
    std::memcpy(memory.data() + HorizontalDistanceSquare, &baseHorizontal, sizeof(float));
    std::memcpy(memory.data() + VerticalDistance, &baseVertical, sizeof(float));

    origAutoPickUpInterval = &NativeInterval;
    Publish(false, false, 6.0f);
    assert(std::fabs(HookAutoPickUpInterval(nullptr) - 0.4f) < 0.0001f);
    BeginManager(reinterpret_cast<uintptr_t>(memory.data()));
    UpdateManager(memory.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(memory.data()), HorizontalDistanceSquare), 16.0f));

    Publish(true, true, 9.0f);
    assert(std::fabs(HookAutoPickUpInterval(nullptr) - 0.18f) < 0.0001f);
    UpdateManager(memory.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(memory.data()), HorizontalDistanceSquare), 81.0f));
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(memory.data()), VerticalDistance), 2.5f));
    UpdateManager(memory.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(memory.data()), HorizontalDistanceSquare), 81.0f));

    Publish(true, true, 12.0f);
    UpdateManager(memory.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(memory.data()), HorizontalDistanceSquare), 144.0f));

    Publish(false, false, 6.0f);
    UpdateManager(memory.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(memory.data()), HorizontalDistanceSquare), 16.0f));
    assert(std::fabs(HookAutoPickUpInterval(nullptr) - 0.4f) < 0.0001f);

    Publish(true, true, std::numeric_limits<float>::infinity());
    assert(SameDistance(selectedRange.load(), 6.0f));
    Publish(true, true, 900.0f);
    assert(SameDistance(selectedRange.load(), MaximumRange));
    Publish(true, true, -1.0f);
    assert(SameDistance(selectedRange.load(), MinimumRange));

    std::array<unsigned char, 256> newManager{};
    const float newHorizontal = 225.0f;
    const float newVertical = 4.0f;
    std::memcpy(newManager.data() + HorizontalDistanceSquare, &newHorizontal, sizeof(float));
    std::memcpy(newManager.data() + VerticalDistance, &newVertical, sizeof(float));
    BeginManager(reinterpret_cast<uintptr_t>(newManager.data()));
    UpdateManager(newManager.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(newManager.data()), HorizontalDistanceSquare), 225.0f));

    Publish(false, false, 6.0f);
    UpdateManager(newManager.data());
    assert(SameDistance(ReadFloat(reinterpret_cast<uintptr_t>(newManager.data()), HorizontalDistanceSquare), 225.0f));
    return 0;
}
