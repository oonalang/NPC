#include "Hitbox.h"
#include "HitboxProfile.h"
#include "../../Trigger/NativeProfile.h"
#include <mutex>

#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
#include "FrameRate.h"

namespace hitbox {
namespace {
using Metadata = const void*;
using WeaponTrace = bool(*)(void*, int, void*, void*, Point, Point, void*, Metadata);
using AvatarRay = bool(*)(void*, int, void*, Point, Point, HitResult*, Metadata);
WeaponTrace originalBase = nullptr, originalInstant = nullptr;
AvatarRay originalRay = nullptr;
std::uintptr_t library = 0;

template<class Function> Function At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<Function>(library + rva);
}

bool Enemy(void* instance, void* target) noexcept {
    using Getter = void*(*)(void*, Metadata);
    const auto* local = At<Getter>(0x8012814)(nullptr, nullptr);
    if (!local || target == local || Field<void*>(instance, 0x28) != local ||
        Field<unsigned char>(local, 0x550) != 1) return false;
    const auto* match = At<Getter>(0x801191C)(nullptr, nullptr);
    std::array<const void*, 512> enemies{};
    std::size_t count = 0;
    if (!CopyList(Field<void*>(match, 0x178), enemies.data(), enemies.size(), count) ||
        std::find(enemies.begin(), enemies.begin() + count, target) == enemies.begin() + count)
        return false;
    return Field<unsigned char>(target, 0x550) == 1;
}

bool Weapon(WeaponTrace original, void* instance, int type, void* target, void* collider,
            Point start, Point direction, void* impact, Metadata info) {
    if (!original) return false;
    const auto invoke = [&] { return original(instance, type, target, collider, start, direction, impact, info); };
    const auto options = Read();
    if (currentTrace || !available.load(std::memory_order_acquire) || !options.enabled ||
        options.scale <= 1.f || !instance || !target || !impact || type != 1 || !Enemy(instance, target))
        return invoke();
    TraceContext context;
    context.scale = options.scale;
    if (!CopyList(Field<void*>(target, 0xa8), context.colliders.data(), context.colliders.size(), context.count))
        return invoke();
    return TraceWeapon(context, invoke);
}

bool Base(void* instance, int type, void* target, void* collider,
          Point start, Point direction, void* impact, Metadata info) {
    return Weapon(originalBase, instance, type, target, collider, start, direction, impact, info);
}
bool Instant(void* instance, int type, void* target, void* collider,
             Point start, Point direction, void* impact, Metadata info) {
    return Weapon(originalInstant, instance, type, target, collider, start, direction, impact, info);
}
bool Ray(void* instance, int shape, void* collider, Point start, Point end, HitResult* result, Metadata info) {
    if (!originalRay) return false;
    if (!currentTrace || currentTrace->scale <= 1.f || !result || !currentTrace->Contains(collider) ||
        !BodyGroup(Field<std::uint16_t>(collider, 0x20)) || Field<unsigned char>(collider, 0x50) != 1)
        return originalRay(instance, shape, collider, start, end, result, info);
    const auto* parent = Field<void*>(collider, 0x40);
    const auto position = Field<Point>(collider, 0x24);
    if (!parent || !Finite(position)) return originalRay(instance, shape, collider, start, end, result, info);
    using TransformPoint = Point(*)(const void*, Point, Metadata);
    const auto center = At<TransformPoint>(0x532D29C)(parent, position, nullptr);
    return ScaledRay(center, start, end, currentTrace->scale, *result,
        [&](Point mappedStart, Point mappedEnd, HitResult& hit) {
            return originalRay(instance, shape, collider, mappedStart, mappedEnd, &hit, info);
        });
}
}

bool Install(std::uintptr_t base, HookInstaller hook) noexcept {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (available.load()) return true;
    if (!base || !hook || status.load() == Status::HookFailed || status.load() == Status::Unsupported) return false;
    std::array<PhantomTrigger::NativeProfileDetail::CodeCheck, std::size(anchors)> checks{};
    for (std::size_t i = 0; i < checks.size(); ++i)
        checks[i] = {"Hitbox game profile differs", anchors[i].rva, anchors[i].bytes};
    using ValidationResult = PhantomTrigger::NativeProfileDetail::ValidationResult;
    struct Validation {
        std::uintptr_t base;
        const PhantomTrigger::NativeProfileDetail::CodeCheck* checks;
        std::size_t count;
        ValidationResult result = ValidationResult::ModuleUnavailable;
    } validation{base, checks.data(), checks.size()};
    dl_iterate_phdr([](dl_phdr_info* module, std::size_t, void* context) {
        auto& requested = *static_cast<Validation*>(context);
        if (!module || module->dlpi_addr != requested.base || !module->dlpi_name) return 0;
        const auto* slash = std::strrchr(module->dlpi_name, '/');
        if (std::strcmp(slash ? slash + 1 : module->dlpi_name, "libunity.so") != 0) return 0;
        const char* failure = nullptr;
        requested.result = PhantomTrigger::NativeProfileDetail::ValidateModule(requested.base,
            module->dlpi_phdr, module->dlpi_phnum, requested.checks, requested.count, failure);
        return 1;
    }, &validation);
    if (validation.result != ValidationResult::Ready) {
        if (validation.result == ValidationResult::InvalidProfile) status = Status::Unsupported;
        return false;
    }
    for (const auto& entry : encoded) {
        if (!frame_rate::Executable(base + entry.rva, entry.bytes.size())) {
            status = Status::Unsupported;
            return false;
        }
        const auto* bytes = reinterpret_cast<const unsigned char*>(base + entry.rva);
        if (std::memcmp(bytes, entry.bytes.data(), entry.bytes.size()) == 0) return false;
        if (!StackEntry(bytes)) return false;
    }
    library = base;
    struct Hook { std::uintptr_t rva; void* replacement; void** original; };
    const Hook hooks[]{
        {0x66FDB60, reinterpret_cast<void*>(Ray), reinterpret_cast<void**>(&originalRay)},
        {0xC2BE0C8, reinterpret_cast<void*>(Base), reinterpret_cast<void**>(&originalBase)},
        {0xD52E8E4, reinterpret_cast<void*>(Instant), reinterpret_cast<void**>(&originalInstant)}
    };
    for (const auto& target : hooks) {
        hook(reinterpret_cast<void*>(base + target.rva), target.replacement, target.original);
        if (!*target.original) { status = Status::HookFailed; return false; }
    }
    status = Status::Ready;
    available.store(true, std::memory_order_release);
    return true;
}
}
#else
namespace hitbox {
bool Install(std::uintptr_t, HookInstaller) noexcept { return false; }
}
#endif
