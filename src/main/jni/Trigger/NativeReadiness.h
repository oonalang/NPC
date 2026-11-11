#pragma once

#include <cstdint>
#include <cstring>
#include <limits>

namespace PhantomTrigger {

struct NativeReadiness {
    bool handlerVerified = false;
    bool shotReady = false;
};

template<class VerifyExecutable>
NativeReadiness QueryNativeReadiness(std::uintptr_t weapon, std::uintptr_t klass,
                                    VerifyExecutable verifyExecutable) noexcept {
    // InventorySlot.StartFire uses slot 274: native code at 0xC0E66E0 loads
    // the function at +0x11F0 and its MethodInfo at +0x11F8 before dispatch.
    constexpr std::uintptr_t EntryOffset = 0x11F0;
    constexpr std::uintptr_t MetadataOffset = 0x11F8;
    if (!weapon || !klass ||
        klass > std::numeric_limits<std::uintptr_t>::max() - MetadataOffset - sizeof(void*))
        return {};

    std::uintptr_t entry = 0;
    const void* metadata = nullptr;
    std::memcpy(&entry, reinterpret_cast<const void*>(klass + EntryOffset), sizeof(entry));
    std::memcpy(&metadata, reinterpret_cast<const void*>(klass + MetadataOffset), sizeof(metadata));
    if (!verifyExecutable(entry)) return {};

    using CanFire = bool(*)(std::uintptr_t, const void*);
    return {true, reinterpret_cast<CanFire>(entry)(weapon, metadata)};
}

}
