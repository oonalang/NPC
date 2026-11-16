#include "SessionProtection.h"
#include "../../Security/Access.h"
#include "SessionProtectionProfile.h"
#include "../../Trigger/TriggerCompatibility.h"
#include <chrono>
#include <cstring>
#include <mutex>

#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
namespace session_protection {
namespace {
using Metadata = const void*;
using ForbidFunction = void(*)(void*, bool, Metadata);
using SendFunction = bool(*)(void*, void*, std::uint32_t, std::uint64_t, Metadata);
using TickFunction = void(*)(void*, float, Metadata);
using ConnectionFunction = void(*)(void*, Metadata);
using DisconnectFunction = bool(*)(void*, Metadata);
ForbidFunction originalForbid = nullptr;
SendFunction originalSend = nullptr;
TickFunction originalTick = nullptr;
ConnectionFunction originalDisconnect = nullptr, originalReset = nullptr, originalShutdown = nullptr;
DisconnectFunction originalOnDisconnect = nullptr;
Controller controller;
std::recursive_mutex stateMutex;
std::mutex installMutex;
std::uintptr_t library = 0;
std::uint64_t generation = 1;
std::atomic<bool> attention{false};
std::atomic<std::uint64_t> appliedRevision{std::numeric_limits<std::uint64_t>::max()};
thread_local unsigned forwarding = 0, ticking = 0;

Preference NativeOptions() noexcept {
    auto options = Read();
    const bool allowed = license::Allows(license::Capability::Features);
    options.enabled = options.enabled && allowed;
    options.revision = (options.revision << 1) | (allowed ? 1u : 0u);
    return options;
}
std::uint64_t Now() noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
template<class Value> Value Field(const void* object, std::size_t offset) noexcept {
    Value value{};
    if (object) std::memcpy(&value, static_cast<const unsigned char*>(object) + offset, sizeof(value));
    return value;
}
template<class Value> bool Safe(std::uintptr_t address, Value& value) noexcept {
    return PhantomTrigger::CompatibilityDetail::ReadSelf(address, &value, sizeof(value));
}
std::uintptr_t NativeClass(std::uintptr_t slot) noexcept {
    std::uintptr_t variable = 0, klass = 0;
    return library && Safe(library + slot, variable) && variable && Safe(variable, klass) ? klass : 0;
}
bool Live() noexcept {
    const auto identity = controller.Context();
    const auto* handler = reinterpret_cast<const void*>(identity.handler);
    return identity.Valid() && identity.generation == generation &&
        Field<unsigned char>(handler, 0x34) == 1 &&
        Field<std::uintptr_t>(handler, 0x38) == identity.connector;
}
void Report() noexcept {
    reported.store(controller.Status(), std::memory_order_release);
    attention.store(controller.NeedsAttention(), std::memory_order_release);
}

struct Capture {
    bool expected, observed = false, queued = false;
    Capture* previous;
    static thread_local Capture* current;
    explicit Capture(bool value) : expected(value), previous(current) { current = this; ++forwarding; }
    ~Capture() { --forwarding; current = previous; }
    Outcome Result() const noexcept { return !observed ? Outcome::Unconfirmed : queued ? Outcome::Queued : Outcome::Rejected; }
};
thread_local Capture* Capture::current = nullptr;

bool Send(void* instance, void* request, std::uint32_t command, std::uint64_t route, Metadata info) {
    const bool queued = originalSend && originalSend(instance, request, command, route, info);
    auto* capture = Capture::current;
    if (capture && !capture->observed && request && command == 0 && route == 0 &&
        Field<int>(request, 0x18) == int(capture->expected)) {
        const auto klass = NativeClass(requestTypeSlot);
        if (klass && Field<std::uintptr_t>(request, 0) == klass) {
            capture->observed = true;
            capture->queued = queued;
        }
    }
    return queued;
}
void Forbid(void* instance, bool value, Metadata info) {
    if (!originalForbid) return;
    if (!available.load(std::memory_order_acquire) || forwarding) { originalForbid(instance, value, info); return; }
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    const auto setting = NativeOptions();
    const bool live = Live();
    if (live) controller.ObserveGameRequest(value);
    const bool effective = setting.enabled && live ? true : value;
    Capture capture(effective);
    originalForbid(instance, effective, info);
    if (live) controller.NaturalResult(setting, effective, capture.Result(), Now());
    Report();
}
Outcome Request(bool value) noexcept {
    Capture capture(value);
    try { originalForbid(nullptr, value, nullptr); }
    catch (...) { return Outcome::Unconfirmed; }
    return capture.Result();
}
void Tick(void* instance, float deltaTime, Metadata info) {
    if (!originalTick) return;
    if (ticking) { originalTick(instance, deltaTime, info); return; }
    struct Scope { Scope() { ++ticking; } ~Scope() { --ticking; } } scope;
    originalTick(instance, deltaTime, info);
    const auto preference = NativeOptions();
    if (!available.load(std::memory_order_acquire) ||
        (!preference.enabled && !attention.load() && appliedRevision.load() == preference.revision)) return;
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    const auto setting = NativeOptions();
    const auto now = Now();
    const auto connector = Field<std::uintptr_t>(instance, 0x38);
    const Identity identity{reinterpret_cast<std::uintptr_t>(instance), connector, generation};
    const bool authenticated = instance && Field<unsigned char>(instance, 0x34) == 1;
    const auto action = controller.Step(identity, authenticated, setting, now);
    if (action.valid && Live()) {
        const auto outcome = Request(action.value);
        controller.Complete(action, outcome, Now());
    }
    appliedRevision.store(setting.revision, std::memory_order_release);
    Report();
}
void Invalidate() noexcept {
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    ++generation;
    controller.Disconnected(NativeOptions().enabled);
    Report();
}
void Disconnect(void* instance, Metadata info) {
    Invalidate();
    if (originalDisconnect) originalDisconnect(instance, info);
}
bool OnDisconnect(void* instance, Metadata info) {
    Invalidate();
    return originalOnDisconnect ? originalOnDisconnect(instance, info) : false;
}
void Reset(void* instance, Metadata info) {
    Invalidate();
    if (originalReset) originalReset(instance, info);
}
void Shutdown(void* instance, Metadata info) {
    Invalidate();
    if (originalShutdown) originalShutdown(instance, info);
}
}

bool Install(std::uintptr_t base, HookInstaller install) noexcept {
    std::lock_guard<std::mutex> lock(installMutex);
    if (available.load()) return true;
    if (!base || !install || backend == Backend::Unsupported || backend == Backend::HookFailed) return false;
    using ValidationResult = PhantomTrigger::NativeProfileDetail::ValidationResult;
    struct Validation { std::uintptr_t base; ValidationResult result = ValidationResult::ModuleUnavailable; } validation{base};
    dl_iterate_phdr([](dl_phdr_info* module, std::size_t, void* context) {
        auto& state = *static_cast<Validation*>(context);
        if (!module || module->dlpi_addr != state.base || !module->dlpi_name) return 0;
        const auto* slash = std::strrchr(module->dlpi_name, '/');
        if (std::strcmp(slash ? slash + 1 : module->dlpi_name, "libunity.so") != 0) return 0;
        const char* failure = nullptr;
        state.result = PhantomTrigger::NativeProfileDetail::ValidateModule(state.base,
            module->dlpi_phdr, module->dlpi_phnum, profile, std::size(profile), failure);
        return 1;
    }, &validation);
    if (validation.result != ValidationResult::Ready) {
        if (validation.result == ValidationResult::InvalidProfile) backend = Backend::Unsupported;
        return false;
    }
    library = base;
    struct Hook { std::uintptr_t rva; void* replacement; void** original; };
    const Hook hooks[]{
        {0x5B9ADF0,reinterpret_cast<void*>(Send),reinterpret_cast<void**>(&originalSend)},
        {0xC977A24,reinterpret_cast<void*>(Forbid),reinterpret_cast<void**>(&originalForbid)},
        {0xC448DD8,reinterpret_cast<void*>(Disconnect),reinterpret_cast<void**>(&originalDisconnect)},
        {0xC44AA14,reinterpret_cast<void*>(OnDisconnect),reinterpret_cast<void**>(&originalOnDisconnect)},
        {0xC44A988,reinterpret_cast<void*>(Reset),reinterpret_cast<void**>(&originalReset)},
        {0xC44A748,reinterpret_cast<void*>(Shutdown),reinterpret_cast<void**>(&originalShutdown)},
        {0xC444A08,reinterpret_cast<void*>(Tick),reinterpret_cast<void**>(&originalTick)}
    };
    for (const auto& hook : hooks) {
        if (install(reinterpret_cast<void*>(base + hook.rva), hook.replacement, hook.original) != 0 || !*hook.original) {
            backend = Backend::HookFailed;
            return false;
        }
    }
    backend = Backend::Ready;
    available.store(true, std::memory_order_release);
    return true;
}
}
#else
namespace session_protection {
bool Install(std::uintptr_t, HookInstaller) noexcept { return false; }
}
#endif
