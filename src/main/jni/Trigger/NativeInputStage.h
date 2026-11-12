#pragma once
#include "TriggerCore.h"
#include <cstdint>
#include <cstring>
#include <limits>

namespace PhantomTrigger {

inline bool IsSupportedPreparation(std::uintptr_t base, std::uintptr_t entry) noexcept {
    if (!base || entry < base) return false;
    const auto rva = entry - base;
    // Weapon and WeaponCODSniper. Charged/specialized handlers remain excluded.
    return rva == 0x54A373C || rva == 0xC38D790;
}

inline FireMode PreparedFireMode(bool handler, bool readiness, bool ammo,
                                 bool burst, bool single, bool release) noexcept {
    if (!handler || !readiness || !ammo || burst) return FireMode::Unsupported;
    // The native release consumer uses the prepared release/up fields, not the
    // weapon's ordinary single-shot flag, to decide whether this input fires.
    if (release) return FireMode::SingleRelease;
    return single ? FireMode::Single : FireMode::Automatic;
}

template<class Setter>
void MergePreparedInput(void* param, void* result, const Intent& intent, Setter set) noexcept {
    if (!param || intent.passthrough) return;
    set(0xBA32E68, param, intent.held);
    set(0xBA32E30, param, intent.down);
    std::memcpy(static_cast<unsigned char*>(param) + 0x1A, &intent.up, sizeof(bool));
    set(0xBA32EE0, param, intent.down);
    set(0xBA32EF0, param, intent.up);
    // UpdateFireInput copied these edges before preparation. Refresh them at
    // the post-preparation seam so its consumer sees our current input too.
    if (result) {
        std::memcpy(static_cast<unsigned char*>(result) + 0x15,
                    static_cast<unsigned char*>(param) + 0x24, sizeof(bool));
        std::memcpy(static_cast<unsigned char*>(result) + 0x16,
                    static_cast<unsigned char*>(param) + 0x25, sizeof(bool));
    }
}
}
