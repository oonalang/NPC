#include "NativeProfile.h"
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include <link.h>

namespace PhantomTrigger {
bool NativeProfileDetail::MatchesBuildIdNote(const unsigned char* data,
                                            std::size_t size) noexcept {
    constexpr unsigned char expected[]{0x05,0x6d,0x63,0x90,0xcc,0x71,0x28,0x65};
    if (!data) return false;
    std::size_t offset = 0;
    while (size - offset >= 12) {
        std::uint32_t nameSize = 0, valueSize = 0, type = 0;
        std::memcpy(&nameSize, data + offset, 4);
        std::memcpy(&valueSize, data + offset + 4, 4);
        std::memcpy(&type, data + offset + 8, 4);
        offset += 12;
        const std::uint64_t paddedName = (std::uint64_t{nameSize} + 3) & ~std::uint64_t{3};
        const std::uint64_t paddedValue = (std::uint64_t{valueSize} + 3) & ~std::uint64_t{3};
        if (paddedName > size - offset || paddedValue > size - offset - paddedName)
            return false;
        if (nameSize == 4 && valueSize == sizeof(expected) && type == 3 &&
            std::memcmp(data + offset, "GNU\0", 4) == 0 &&
            std::memcmp(data + offset + paddedName, expected, sizeof(expected)) == 0)
            return true;
        offset += static_cast<std::size_t>(paddedName + paddedValue);
    }
    return false;
}

namespace {
const char* PendingDecodeReason(std::uintptr_t rva) noexcept {
    switch (rva) {
        case 0x530005C: return "Waiting for decoded Screen.get_width (0x530005C)";
        case 0x53000C0: return "Waiting for decoded Screen.get_height (0x53000C0)";
        case 0x531EA00: return "Waiting for decoded Physics.RaycastNonAlloc (0x531EA00)";
        case 0x54AFFE4: return "Waiting for decoded Weapon.CanFire (0x54AFFE4)";
        case 0x5E5FA20: return "Waiting for decoded BaseGame.IsInSmoke (0x5E5FA20)";
        case 0x5E5FBE8: return "Waiting for decoded BaseGame.IsSmokeInMiddle (0x5E5FBE8)";
        case 0x5E5FDB4: return "Waiting for decoded smoke cluster query (0x5E5FDB4)";
        case 0x66F0820: return "Waiting for decoded raycast scratch getter (0x66F0820)";
        case 0x66FE6B4: return "Waiting for decoded detailed target trace (0x66FE6B4)";
        case 0x78AF19C: return "Waiting for decoded collider target lookup (0x78AF19C)";
        default:
            return nullptr;
    }
}
}

bool NativeProfileDetail::RequiresExactRuntimePrefix(std::uintptr_t rva) noexcept {
    return PendingDecodeReason(rva) == nullptr;
}

namespace {
using NativeProfileDetail::CodeCheck;
using NativeProfileDetail::ValidationResult;

bool Range(const ElfW(Phdr)* segments, std::size_t count, std::uintptr_t rva,
           std::size_t size, unsigned permissions) noexcept {
    if (!segments) return false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto& segment = segments[i];
        if (segment.p_type == PT_LOAD && (segment.p_flags & permissions) == permissions &&
            rva >= segment.p_vaddr && rva - segment.p_vaddr <= segment.p_memsz &&
            size <= segment.p_memsz - (rva - segment.p_vaddr)) return true;
    }
    return false;
}

#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
struct Validation {
    std::uintptr_t base;
    const CodeCheck* requested;
    std::size_t count;
    ValidationResult result = ValidationResult::ModuleUnavailable;
    const char* failure = "Waiting for the game library loader";
};
int Validate(dl_phdr_info* info, std::size_t, void* argument) noexcept {
    auto& state = *static_cast<Validation*>(argument);
    if (!info || !info->dlpi_name || info->dlpi_addr != state.base) return 0;
    const auto slash = std::strrchr(info->dlpi_name, '/');
    if (std::strcmp(slash ? slash + 1 : info->dlpi_name, "libunity.so") != 0) return 0;
    state.result = NativeProfileDetail::ValidateModule(state.base, info->dlpi_phdr,
        info->dlpi_phnum, state.requested, state.count, state.failure);
    return 1;
}
#endif
}

NativeProfileDetail::ValidationResult NativeProfileDetail::ValidateModule(
        std::uintptr_t base, const ElfW(Phdr)* segments, std::size_t segmentCount,
        const CodeCheck* requested, std::size_t count, const char*& failure) noexcept {
    failure = "Invalid native profile inputs";
    if (!base || !segments || segmentCount == 0 || !requested || count == 0)
        return ValidationResult::InvalidProfile;
    bool identity = false;
    for (std::size_t i = 0; i < segmentCount; ++i) {
        const auto& note = segments[i];
        if (note.p_type != PT_NOTE || note.p_filesz < 12 || note.p_filesz > 4096 ||
            note.p_vaddr > std::numeric_limits<std::uintptr_t>::max() - base ||
            note.p_filesz > std::numeric_limits<std::uintptr_t>::max() - base - note.p_vaddr ||
            !Range(segments, segmentCount, note.p_vaddr, note.p_filesz, PF_R)) continue;
        const auto* bytes = reinterpret_cast<const unsigned char*>(base + note.p_vaddr);
        if (MatchesBuildIdNote(bytes, note.p_filesz)) identity = true;
    }
    if (!identity) {
        failure = "Unsupported game library build ID";
        return ValidationResult::InvalidProfile;
    }
    const char* waiting = nullptr;
    for (std::size_t i = 0; i < count; ++i) {
        const auto& check = requested[i];
        if (check.rva > std::numeric_limits<std::uintptr_t>::max() - base ||
            check.expected.size() > std::numeric_limits<std::uintptr_t>::max() - base - check.rva ||
            !Range(segments, segmentCount, check.rva, check.expected.size(), PF_R | PF_X)) {
            failure = "Native method is outside readable executable game segments";
            return ValidationResult::InvalidProfile;
        }
        const auto* actual = reinterpret_cast<const void*>(base + check.rva);
        const bool matches = std::memcmp(actual, check.expected.data(), check.expected.size()) == 0;
        if (RequiresExactRuntimePrefix(check.rva)) {
            if (!matches) {
                failure = check.failure;
                return ValidationResult::InvalidProfile;
            }
        } else if (matches && !waiting) {
            waiting = PendingDecodeReason(check.rva);
        }
    }
    if (waiting) {
        failure = waiting;
        return ValidationResult::ModuleUnavailable;
    }
    failure = nullptr;
    return ValidationResult::Ready;
}

bool NativeProfileDetail::ValidateEntrypoints(std::uintptr_t base,
        const CodeCheck* requested, std::size_t count, const char*& failure) noexcept {
    failure = "Native profile verification unavailable";
#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
    if (!base || !requested || count == 0) return false;
    Validation validation{base,requested,count};
    dl_iterate_phdr(Validate, &validation);
    failure = validation.failure;
    return validation.result == ValidationResult::Ready;
#else
    (void)base; (void)requested; (void)count;
    return false;
#endif
}
}
