#pragma once

#include "TriggerRange.h"
#include "TriggerRuntime.h"
#include <cmath>

namespace PhantomTrigger {

struct NativeCameraRay { Point3 origin, direction; };
static_assert(sizeof(NativeCameraRay) == 24, "ARM64 camera Ray return layout");
static_assert(sizeof(Point3) == 12, "ARM64 Vector3 HFA argument layout");

inline bool FiniteCameraPoint(const Point3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

template<class Camera>
bool SampleNativeViewRay(Camera& camera, ViewRay& ray,
                         float& normalizedX, float& normalizedY, bool sampleViewport) noexcept {
    normalizedX = normalizedY = 0.5f;
    ray.maxDistance = 0.f;
    if (!camera.IsValid()) return false;
    const float nearPlane = camera.NearPlane();
    const float farPlane = camera.FarPlane();
    const float distance = VisibleTraceDistance(nearPlane,farPlane);
    if (distance <= 0.f) return false;
    const Point3 center{0.5f,0.5f,0};
    const auto native = camera.ViewportRay(center);
    if (!FiniteCameraPoint(native.origin) || !FiniteCameraPoint(native.direction)) return false;
    const double lengthSquared = static_cast<double>(native.direction.x) * native.direction.x +
                                 static_cast<double>(native.direction.y) * native.direction.y +
                                 static_cast<double>(native.direction.z) * native.direction.z;
    if (!std::isfinite(lengthSquared) || lengthSquared < 0.5 || lengthSquared > 1.5) return false;
    if (sampleViewport) {
        const int width = camera.Width();
        const int height = camera.Height();
        if (width <= 0 || height <= 0) return false;
        const auto screen = camera.ScreenPoint(center);
        if (!FiniteCameraPoint(screen)) return false;
        normalizedX = screen.x / static_cast<float>(width);
        normalizedY = 1.f - screen.y / static_cast<float>(height);
        if (!std::isfinite(normalizedX) || !std::isfinite(normalizedY) ||
            normalizedX < 0 || normalizedX > 1 || normalizedY < 0 || normalizedY > 1) return false;
    }
    const float inverseLength = static_cast<float>(1.0 / std::sqrt(lengthSquared));
    ray.origin = native.origin;
    ray.maxDistance = distance;
    ray.direction = {native.direction.x * inverseLength,
                     native.direction.y * inverseLength,
                     native.direction.z * inverseLength};
    return true;
}
}
