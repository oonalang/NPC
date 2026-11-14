#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <link.h>

namespace PhantomTrigger {


bool InitializeNativeTrigger(std::uintptr_t base) noexcept;
bool IsVerifiedExecutableAddress(std::uintptr_t address) noexcept;
namespace NativeProfileDetail {
struct CodeCheck {
    const char* failure;
    std::uintptr_t rva;
    std::array<unsigned char, 16> expected;
};
bool MatchesBuildIdNote(const unsigned char* data, std::size_t size) noexcept;

enum class ValidationResult { Ready, ModuleUnavailable, InvalidProfile };

bool RequiresExactRuntimePrefix(std::uintptr_t rva) noexcept;

ValidationResult ValidateModule(std::uintptr_t base, const ElfW(Phdr)* segments,
                               std::size_t segmentCount, const CodeCheck* checks,
                               std::size_t count, const char*& failure) noexcept;

bool ValidateEntrypoints(std::uintptr_t base, const CodeCheck* checks,
                         std::size_t count, const char*& failure) noexcept;
}
}
