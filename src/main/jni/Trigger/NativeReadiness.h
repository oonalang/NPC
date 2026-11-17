#pragma once

#include <cstdint>
#include <cstring>
#include <limits>

namespace PhantomTrigger {

struct NativeReadinessBinding {
    std::uintptr_t entry = 0;
    const void* metadata = nullptr;

    bool IsVerified() const noexcept { return entry != 0; }
    bool Sample(std::uintptr_t weapon) const noexcept {
        if (!weapon || !entry) return false;
        using CanFire = bool(*)(std::uintptr_t, const void*);
        return reinterpret_cast<CanFire>(entry)(weapon, metadata);
    }
};

template<class VerifyExecutable>
NativeReadinessBinding BindNativeReadiness(std::uintptr_t klass,
                                         VerifyExecutable verifyExecutable) noexcept {
    constexpr std::uintptr_t EntryOffset = 0x11F0;
    constexpr std::uintptr_t MetadataOffset = 0x11F8;
    if (!klass ||
        klass > std::numeric_limits<std::uintptr_t>::max() - MetadataOffset - sizeof(void*))
        return {};

    std::uintptr_t entry = 0;
    const void* metadata = nullptr;
    std::memcpy(&entry, reinterpret_cast<const void*>(klass + EntryOffset), sizeof(entry));
    std::memcpy(&metadata, reinterpret_cast<const void*>(klass + MetadataOffset), sizeof(metadata));
    if (!verifyExecutable(entry)) return {};

    return {entry, metadata};
}

}
