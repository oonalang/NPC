#pragma once

#include "TriggerRuntime.h"
#include <cstdint>

namespace PhantomTrigger {

bool InstallNativeDriver(std::uintptr_t base) noexcept;
RuntimeStatus NativeStatus() noexcept;
const char* NativeStatusText() noexcept;
FireMode NativeDetectedMode() noexcept;
bool NativeViewportCenter(float& x, float& y) noexcept;

void NativeDriverFailure(const char* staticReason) noexcept;
void NativeDriverPending(const char* staticReason) noexcept;


void NotifyNativeShot(std::uintptr_t component) noexcept;
}
