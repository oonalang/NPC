#include "TriggerCompatibility.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <link.h>
#include <unistd.h>

namespace PhantomTrigger {
namespace {
struct Probe {
    const char* name;
    std::uintptr_t rva;
    std::array<unsigned char, 8> expected;
    bool compare;
};
constexpr Probe probes[] = {
    {"Burst getter", 0x5474344, {0x00,0xb0,0x48,0x39,0xc0,0x03,0x5f,0xd6}, true},
    {"Fire-input setter", 0xBA32E30, {0x08,0x60,0x40,0x39,0x1f,0x01,0x00,0x71}, true},
    {"Fire-held getter", 0xBA32E60, {0x00,0x70,0x40,0x39,0xc0,0x03,0x5f,0xd6}, true},
    {"Shot acknowledgement", 0x54748B8, {0x00,0x98,0x41,0xb9,0xc0,0x03,0x5f,0xd6}, true},
    {"Body trace", 0x66F17EC, {0xff,0x43,0x02,0xd1,0xee,0x1b,0x00,0xfd}, true},
    {"Input update", 0xC913748, {0xe8,0x0f,0x1d,0xfc,0xfe,0x57,0x01,0xa9}, true},
    // This is an encoded disk prefix; capture live bytes without comparing
    // them to the loader's on-disk representation.
    {"Smoke query", 0x5E5FBE8, {0xf1,0x19,0x19,0xad,0x19,0x08,0x59,0xe0}, false}
};
static_assert(sizeof(probes) / sizeof(probes[0]) == 7, "report probe count");

bool ExecutableRange(const dl_phdr_info& info, std::uintptr_t rva,
                     std::size_t size) noexcept {
    for (std::size_t i = 0; i < info.dlpi_phnum; ++i) {
        const auto& segment = info.dlpi_phdr[i];
        if (segment.p_type != PT_LOAD || (segment.p_flags & (PF_R | PF_X)) != (PF_R | PF_X))
            continue;
        if (rva >= segment.p_vaddr && rva - segment.p_vaddr <= segment.p_memsz &&
            size <= segment.p_memsz - (rva - segment.p_vaddr)) return true;
    }
    return false;
}

int Capture(dl_phdr_info* info, std::size_t, void* destination) noexcept {
    if (!info || !info->dlpi_name) return 0;
    const char* slash = std::strrchr(info->dlpi_name, '/');
    const char* name = slash ? slash + 1 : info->dlpi_name;
    if (std::strcmp(name, "libunity.so") != 0) return 0;
    auto& report = *static_cast<CompatibilityReport*>(destination);
    report.moduleFound = true;
    for (std::size_t i = 0; i < report.samples.size(); ++i) {
        auto& sample = report.samples[i];
        const auto& probe = probes[i];
        sample.name = probe.name;
        sample.rva = probe.rva;
        sample.hasExpectedPrefix = probe.compare;
        if (!ExecutableRange(*info, probe.rva, sample.bytes.size()) ||
            probe.rva > std::numeric_limits<std::uintptr_t>::max() - info->dlpi_addr ||
            sample.bytes.size() > std::numeric_limits<std::uintptr_t>::max() - info->dlpi_addr - probe.rva)
            continue;
        // The callback already owns a validated module mapping. Avoid nested
        // loader enumeration and do not depend on /proc/self/mem permissions.
        std::memcpy(sample.bytes.data(), reinterpret_cast<const void*>(info->dlpi_addr + probe.rva),
                    sample.bytes.size());
        sample.readable = true;
        sample.prefixMatches = sample.readable && probe.compare &&
            std::memcmp(sample.bytes.data(), probe.expected.data(), probe.expected.size()) == 0;
    }
    return 1;
}

struct SelfRead {
    std::uintptr_t address;
    void* output;
    std::size_t size;
    bool mapped = false;
    bool copied = false;
};

int CopyReadableModule(dl_phdr_info* info, std::size_t, void* argument) noexcept {
    if (!info || !info->dlpi_phdr) return 0;
    auto& request = *static_cast<SelfRead*>(argument);
    if (request.address < info->dlpi_addr) return 0;
    const auto relative = request.address - info->dlpi_addr;
    for (std::size_t i = 0; i < info->dlpi_phnum; ++i) {
        const auto& segment = info->dlpi_phdr[i];
        if (segment.p_type != PT_LOAD || relative < segment.p_vaddr ||
            relative - segment.p_vaddr >= segment.p_memsz) continue;
        request.mapped = true;
        if ((segment.p_flags & PF_R) != 0 &&
            request.size <= segment.p_memsz - (relative - segment.p_vaddr)) {
            // The loader keeps this module mapped throughout the callback.
            std::memcpy(request.output, reinterpret_cast<const void*>(request.address), request.size);
            request.copied = true;
        }
        // An unreadable or truncated module range must never use the /proc
        // fallback to circumvent the segment's declared access permissions.
        return 1;
    }
    return 0;
}
}

bool CompatibilityDetail::ReadSelf(std::uintptr_t address, void* output,
                                   std::size_t size) noexcept {
    if (!address || !output || size == 0 || size > 256 ||
        size > std::numeric_limits<std::uintptr_t>::max() - address) return false;
    SelfRead request{address, output, size};
    dl_iterate_phdr(CopyReadableModule, &request);
    if (request.mapped) return request.copied;

    // Nonmodule memory retains the bounded diagnostic fallback. Module reads
    // above work even when Android denies opening /proc/self/mem.
    if (address > static_cast<std::uintptr_t>(std::numeric_limits<off_t>::max())) return false;
    const int descriptor = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) return false;
    const auto copied = pread(descriptor, output, size, static_cast<off_t>(address));
    close(descriptor);
    return copied == static_cast<ssize_t>(size);
}

CompatibilityReport CaptureLoadedCompatibility() noexcept {
    CompatibilityReport report;
#if defined(__aarch64__)
    report.arm64 = true;
    dl_iterate_phdr(Capture, &report);
#else

    (void)&Capture;
#endif
    return report;
}

void FormatCompatibility(const CompatibilityReport& report, char* output,
                         std::size_t capacity) noexcept {
    if (!output || capacity == 0) return;
    std::size_t used = 0;
    auto append = [&](const char* format, auto... args) {
        if (used >= capacity - 1) return;
        const int count = std::snprintf(output + used, capacity - used, format, args...);
        if (count < 0) return;
        const auto written = static_cast<std::size_t>(count);
        used += written < capacity - used ? written : capacity - used - 1;
    };
    output[0] = '\0';
    append("Read-only trigger check v1\nARM64: %s; game library: %s\n",
           report.arm64 ? "yes" : "no", report.moduleFound ? "loaded" : "not found");
    if (report.moduleFound) {
        for (const auto& sample : report.samples) {
            append("%s @ %llx: ", sample.name, static_cast<unsigned long long>(sample.rva));
            if (!sample.readable) { append("unreadable\n"); continue; }
            for (auto byte : sample.bytes) append("%02x", static_cast<unsigned>(byte));
            if (sample.hasExpectedPrefix)
                append(" [%s]", sample.prefixMatches ? "prefix matches" : "prefix differs");
            else
                append(" [runtime bytes; disk prefix may be encoded]");
            append("\n");
        }
    }
    append("This report does not enable automatic firing.");
}
}
