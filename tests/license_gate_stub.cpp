#include "license_gate_stub.h"
#include "../src/main/jni/Security/Access.h"
namespace license_test {std::atomic<bool> allowed{true};}
namespace license {
bool Allows(Capability capability) noexcept {
    const auto bits=static_cast<std::uint32_t>(capability);
    return bits && !(bits & ~allCapabilities) && license_test::allowed.load();
}
}
