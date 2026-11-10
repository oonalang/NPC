#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace PhantomTrigger {


void InitializeNativeTrigger(std::uintptr_t base) noexcept;
namespace NativeProfileDetail {
struct CodeCheck {
    const char* failure;
    std::uintptr_t rva;
    std::array<unsigned char, 16> expected;
};
bool MatchesBuildIdNote(const unsigned char* data, std::size_t size) noexcept;


bool ValidateEntrypoints(std::uintptr_t base, const CodeCheck* checks,
                         std::size_t count, const char*& failure) noexcept;
}
}
