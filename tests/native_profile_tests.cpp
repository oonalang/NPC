#include "../src/main/jni/Trigger/NativeProfile.h"
#include "../src/main/jni/Trigger/TriggerCompatibility.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sys/mman.h>

using namespace PhantomTrigger;
using NativeProfileDetail::CodeCheck;
using NativeProfileDetail::ValidationResult;

namespace {
#include "../src/main/jni/Trigger/NativeProfileChecks.inc"
int assertions = 0;
int installs = 0;
int failures = 0;
int pending = 0;
int blockedOpens = 0;
bool loaderReady = true;
dl_phdr_info loaded{};

void Require(bool condition, const char* message) {
    ++assertions;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Module {
    static constexpr std::size_t size = 0xC91A000;
    unsigned char* bytes;
    std::array<ElfW(Phdr), 3> segments{};
    Module() : bytes(static_cast<unsigned char*>(mmap(nullptr, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0))) {
        Require(bytes != MAP_FAILED, "synthetic module mapping available");
        segments[0].p_type = PT_LOAD;
        segments[0].p_flags = PF_R;
        segments[0].p_memsz = segments[0].p_filesz = 0x1000;
        segments[1].p_type = PT_LOAD;
        segments[1].p_flags = PF_R | PF_X;
        segments[1].p_vaddr = 0x1000;
        segments[1].p_memsz = segments[1].p_filesz = size - 0x1000;
        segments[2].p_type = PT_NOTE;
        segments[2].p_vaddr = 0x100;
        segments[2].p_memsz = segments[2].p_filesz = 24;
        constexpr unsigned char note[]{4,0,0,0,8,0,0,0,3,0,0,0,
            'G','N','U',0,0x05,0x6d,0x63,0x90,0xcc,0x71,0x28,0x65};
        std::memcpy(bytes + 0x100, note, sizeof(note));
        for (const auto& check : checks)
            std::memcpy(bytes + check.rva, check.expected.data(), check.expected.size());
        loaded.dlpi_addr = base();
        loaded.dlpi_name = "/data/app/game/lib/arm64/libunity.so";
        loaded.dlpi_phdr = segments.data();
        loaded.dlpi_phnum = segments.size();
    }
    ~Module() { munmap(bytes, size); }
    std::uintptr_t base() const { return reinterpret_cast<std::uintptr_t>(bytes); }
    ValidationResult Validate(const char*& failure) const {
        return NativeProfileDetail::ValidateModule(base(), segments.data(), segments.size(),
            checks, sizeof(checks) / sizeof(checks[0]), failure);
    }
};

void NoteParsing(Module& module) {
    const auto note = module.bytes + 0x100;
    Require(NativeProfileDetail::MatchesBuildIdNote(note, 24), "pinned GNU build ID matches");
    Require(!NativeProfileDetail::MatchesBuildIdNote(note, 23), "truncated GNU note rejected");
    std::array<unsigned char, 40> combined{};
    combined[0] = 1; // A one-byte unknown name, padded to four bytes.
    combined[8] = 1;
    combined[12] = 'X';
    std::memcpy(combined.data() + 16, note, 24);
    Require(NativeProfileDetail::MatchesBuildIdNote(combined.data(), combined.size()),
            "GNU build ID can follow another aligned note");
    combined[0] = combined[1] = combined[2] = combined[3] = 255;
    Require(!NativeProfileDetail::MatchesBuildIdNote(combined.data(), combined.size()),
            "oversized note lengths rejected without overflow");
}

void ValidationPolicy(Module& module) {
    const char* reason = nullptr;
    Require(module.Validate(reason) == ValidationResult::ModuleUnavailable && reason,
            "unchanged encoded disk bodies keep initialization pending");
    module.bytes[0xC913748] ^= 1;
    Require(module.Validate(reason) == ValidationResult::InvalidProfile,
            "stable hook mismatch remains permanent while encoded bodies are pending");
    module.bytes[0xC913748] ^= 1;
    std::size_t encoded = 0;
    for (const auto& check : checks) {
        if (!NativeProfileDetail::RequiresExactRuntimePrefix(check.rva)) {
            ++encoded;
            constexpr unsigned char decoded[]{0xfd,0x7b,0xbf,0xa9,0xfd,0x03,0x00,0x91,
                                               0x1f,0x20,0x03,0xd5,0xc0,0x03,0x5f,0xd6};
            std::memcpy(module.bytes + check.rva, decoded, sizeof(decoded));
        }
    }
    Require(encoded == 8, "exactly eight known disk-only prefixes classified");
    Require(module.Validate(reason) == ValidationResult::Ready,
            "loader-decoded bodies do not reject an otherwise matching build");
    constexpr std::uintptr_t hooks[]{0xC913748,0xC913FA4,0xC916D64,0xC917C78,0xC917D68};
    for (auto rva : hooks) {
        module.bytes[rva] ^= 1;
        Require(module.Validate(reason) == ValidationResult::InvalidProfile && reason,
                "each changed live hook anchor is rejected");
        module.bytes[rva] ^= 1;
    }
    module.bytes[0x110] ^= 1;
    Require(module.Validate(reason) == ValidationResult::InvalidProfile && reason,
            "unknown build ID rejected even when all hooks match");
    module.bytes[0x110] ^= 1;
    module.segments[1].p_flags = PF_X;
    Require(module.Validate(reason) == ValidationResult::InvalidProfile,
            "unreadable executable segment rejected before copying");
    module.segments[1].p_flags = PF_R;
    Require(module.Validate(reason) == ValidationResult::InvalidProfile,
            "readable nonexecutable method range rejected");
    module.segments[1].p_flags = PF_R | PF_X;
    const auto originalSize = module.segments[1].p_memsz;
    module.segments[1].p_memsz = 0x100;
    Require(module.Validate(reason) == ValidationResult::InvalidProfile,
            "truncated executable range rejected");
    module.segments[1].p_memsz = originalSize;
    Require(NativeProfileDetail::RequiresExactRuntimePrefix(0x123456),
            "unlisted methods retain exact runtime checks by default");
    Require(NativeProfileDetail::ValidateModule(std::numeric_limits<std::uintptr_t>::max() - 8,
            module.segments.data(), module.segments.size(), checks, 1, reason) ==
            ValidationResult::InvalidProfile, "overflowing module addresses rejected");
}

void ReadableMappings(Module& module) {
    std::array<unsigned char, 16> actual{};
    constexpr std::uintptr_t anchor = 0xC913748;
    Require(CompatibilityDetail::ReadSelf(module.base() + anchor, actual.data(), actual.size()),
            "loaded module reads succeed while /proc/self/mem is denied");
    Require(std::equal(actual.begin(), actual.end(), module.bytes + anchor),
            "loaded module read copies the requested bytes");
    Require(blockedOpens == 0, "mapped reads never need the proc fallback");
    module.segments[1].p_flags = PF_X;
    Require(!CompatibilityDetail::ReadSelf(module.base() + anchor, actual.data(), actual.size()) &&
            blockedOpens == 0, "unreadable mapped range cannot use proc fallback");
    module.segments[1].p_flags = PF_R | PF_X;
    Require(!CompatibilityDetail::ReadSelf(module.base() + Module::size - 4,
            actual.data(), actual.size()) && blockedOpens == 0,
            "read crossing mapped segment boundary is rejected");
    Require(!CompatibilityDetail::ReadSelf(1, actual.data(), actual.size()) && blockedOpens == 1,
            "denied nonmodule fallback fails cleanly");
    const auto report = CaptureLoadedCompatibility();
    Require(report.moduleFound && report.arm64 && report.samples[5].prefixMatches,
            "compatibility report still compares stable live prefixes");
    Require(report.samples[6].readable && !report.samples[6].hasExpectedPrefix,
            "encoded smoke prefix is captured without a false live mismatch");
}

void RetryAndPublish(Module& module) {
    const auto address = module.base() + 0x54AFFE4;
    Require(!IsVerifiedExecutableAddress(address), "no executable cache before validation");
    loaderReady = false;
    Require(!InitializeNativeTrigger(module.base()) && pending == 1 && installs == 0 && failures == 0,
            "missing loader entry remains pending and permits retry");
    Require(!IsVerifiedExecutableAddress(address), "pending initialization publishes no ranges");
    loaderReady = true;
    const auto encoded = std::find_if(std::begin(checks), std::end(checks), [](const CodeCheck& check) {
        return check.rva == 0x530005C;
    });
    std::array<unsigned char, 16> decoded{};
    std::memcpy(decoded.data(), module.bytes + encoded->rva, decoded.size());
    std::memcpy(module.bytes + encoded->rva, encoded->expected.data(), encoded->expected.size());
    Require(!InitializeNativeTrigger(module.base()) && pending == 2 && installs == 0 && failures == 0,
            "mapped but still encoded method remains pending without hook installation");
    Require(!IsVerifiedExecutableAddress(address), "encoded pending state publishes no ranges");
    std::memcpy(module.bytes + encoded->rva, decoded.data(), decoded.size());
    Require(InitializeNativeTrigger(module.base()) && installs == 1 && failures == 0,
            "retry installs once after the loader becomes available");
    Require(IsVerifiedExecutableAddress(address), "validated decoded method is executable");
    Require(!IsVerifiedExecutableAddress(0) && !IsVerifiedExecutableAddress(address + 1),
            "null and unaligned virtual methods rejected");
    Require(!IsVerifiedExecutableAddress(module.base() + 0x100),
            "read-only data cannot be called as a virtual method");
    Require(IsVerifiedExecutableAddress(module.base() + Module::size - 4) &&
            !IsVerifiedExecutableAddress(module.base() + Module::size),
            "executable cache respects complete instruction boundaries");
    Require(InitializeNativeTrigger(module.base()) && installs == 1,
            "completed installation is idempotent");
}

void PermanentFailure(Module& module) {
    module.bytes[0xC913748] ^= 1;
    Require(InitializeNativeTrigger(module.base()) && failures == 1 && installs == 0,
            "changed stable hook produces a terminal failure");
    Require(!IsVerifiedExecutableAddress(module.base() + 0xC913748),
            "rejected module publishes no executable ranges");
    module.bytes[0xC913748] ^= 1;
    Require(InitializeNativeTrigger(module.base()) && failures == 1 && installs == 0,
            "permanent failure never retries hook installation");
}
}

extern "C" int dl_iterate_phdr(int (*callback)(dl_phdr_info*, std::size_t, void*), void* data) {
    return loaderReady ? callback(&loaded, sizeof(loaded), data) : 0;
}

extern "C" int __wrap_open(const char*, int, ...) {
    ++blockedOpens;
    errno = EACCES;
    return -1;
}

namespace PhantomTrigger {
bool InstallNativeDriver(std::uintptr_t) noexcept { ++installs; return true; }
void NativeDriverFailure(const char*) noexcept { ++failures; }
void NativeDriverPending(const char*) noexcept { ++pending; }
}

int main(int argc, char** argv) {
    Module module;
    if (argc == 2 && std::strcmp(argv[1], "--permanent") == 0) {
        PermanentFailure(module);
    } else {
        NoteParsing(module);
        ValidationPolicy(module);
        ReadableMappings(module);
        RetryAndPublish(module);
    }
    std::cout << "Passed " << assertions << " native profile checks\n";
}
