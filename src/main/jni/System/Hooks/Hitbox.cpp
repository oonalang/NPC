#include "Hitbox.h"
#include "../../Security/Access.h"
#include "HitboxProfile.h"
#include "../../Trigger/NativeProfile.h"
#include <mutex>

#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
#include "FrameRate.h"

namespace hitbox {
namespace {
using Metadata = const void*;
using WeaponTrace = bool(*)(void*, int, void*, void*, Point, Point, void*, Metadata);
using AvatarCapsule = bool(*)(void*, void*, Point, Point, HitResult*, Metadata);
using AabbRay = bool(*)(void*, Point, Point, Point, Point, float*, Metadata);
using SphereRay = bool(*)(void*, Point, float, Point, Point, float, HitResult*, Metadata);
using CapsuleRay = bool(*)(void*, Point, Rotation, float, float, Point, Point, HitResult*, Metadata);
using Getter = void*(*)(void*, Metadata);
WeaponTrace originalBase = nullptr, originalInstant = nullptr;
AvatarCapsule originalAvatarCapsule = nullptr;
CapsuleRay originalCapsule = nullptr;
AvatarCapsule originalAvatarSphere = nullptr;
SphereRay originalSphere = nullptr;
AvatarCapsule originalAvatarObb = nullptr;
AabbRay originalAabb = nullptr;
Getter localPawn = nullptr, matchGame = nullptr;

struct ListHeader {
    const void* list = nullptr;
    const void* items = nullptr;
    int count = 0;
};

bool ReadListHeader(const void* list, ListHeader& header) noexcept {
    header.list = list;
    header.items = Field<void*>(list, 0x10);
    header.count = Field<int>(list, 0x18);
    return header.list && header.items && header.count > 0;
}

bool SameList(const ListHeader& left, const ListHeader& right) noexcept {
    return left.list == right.list && left.items == right.items &&
           left.count == right.count;
}

bool ScanEnemy(void* target, ListHeader& enemies, std::size_t& index) noexcept {
    const auto* match = matchGame ? matchGame(nullptr, nullptr) : nullptr;
    if (!match) return false;
    std::array<const void*, 512> list{};
    std::size_t count = 0;
    const auto* source = Field<void*>(match, 0x178);
    if (!CopyList(source, list.data(), list.size(), count) || !ReadListHeader(source, enemies))
        return false;
    const auto found = std::find(list.begin(), list.begin() + count, target);
    if (found == list.begin() + count) return false;
    index = static_cast<std::size_t>(found - list.begin());
    return true;
}

// A single fired round can reach the trace hook more than once (base and
// instant paths, penetration passes). The enemy scan and the collider copy are
// cached per (weapon instance, target) and re-validated on every use: the same
// slot must still hold the target, the list headers must be unchanged and the
// target must still be alive. Any change - a different pawn, a rebuilt list, a
// death - falls back to a full read, so a stale answer cannot reach the next
// bullet or the next frame.
struct Lookup {
    void* instance = nullptr;
    void* target = nullptr;
    ListHeader enemies{};
    std::size_t enemyIndex = 0;
    ListHeader regions{};
    std::array<const void*, maximumColliders> colliders{};
    std::size_t count = 0;
    bool collidersReady = false;
};
thread_local Lookup lookup;
thread_local bool fieldScaling = false;

bool EnemyCached(void* instance, void* target) noexcept {
    if (!localPawn || !matchGame) return false;
    const auto* local = localPawn(nullptr, nullptr);
    if (!local || target == local || Field<void*>(instance, 0x28) != local ||
        Field<unsigned char>(local, 0x550) != 1) return false;
    if (Field<unsigned char>(target, 0x550) != 1) return false;
    const auto* match = matchGame ? matchGame(nullptr, nullptr) : nullptr;
    ListHeader current{};
    if (match && lookup.instance == instance && lookup.target == target &&
        lookup.enemies.count > 0 && lookup.enemyIndex < std::size_t(lookup.enemies.count) &&
        ReadListHeader(Field<void*>(match, 0x178), current) &&
        SameList(current, lookup.enemies) &&
        Field<const void*>(lookup.enemies.items,
                           0x20 + lookup.enemyIndex * sizeof(void*)) == target)
        return true;
    lookup = Lookup{};
    if (!ScanEnemy(target, lookup.enemies, lookup.enemyIndex)) return false;
    lookup.instance = instance;
    lookup.target = target;
    return true;
}

bool CollidersCached(void* target, TraceContext& context) noexcept {
    const auto* list = static_cast<const void*>(Field<void*>(target, 0xa8));
    ListHeader header{};
    if (lookup.target == target && lookup.collidersReady && lookup.count > 0 &&
        ReadListHeader(list, header) && SameList(header, lookup.regions) &&
        lookup.count == std::size_t(header.count)) {
        context.count = lookup.count;
        std::copy(lookup.colliders.begin(), lookup.colliders.begin() + lookup.count,
                  context.colliders.begin());
        return true;
    }
    lookup.collidersReady = false;
    if (!CopyList(list, context.colliders.data(), context.colliders.size(), context.count) ||
        !ReadListHeader(list, header))
        return false;
    lookup.regions = header;
    lookup.count = context.count;
    std::copy(context.colliders.begin(), context.colliders.begin() + context.count,
              lookup.colliders.begin());
    lookup.collidersReady = true;
    return true;
}

// The hitbox a shot tests against carries its own size at +0x14. This inflates
// that field on every hitbox of the traced pawn for the duration of one trace
// and writes the exact previous values back immediately afterwards, so the
// pawn is restored before the next shot or frame whatever the trace returns or
// throws. Scaling the stored fields is what the reference mod does: the size
// applies to the first trace and to every region, instead of a second attempt
// that only runs after a miss.
struct HitboxField {
    void* hitbox = nullptr;
    float size = 0.f;
};
class HitboxFields {
    std::array<HitboxField, maximumColliders> fields_{};
    std::size_t count_ = 0;
public:
    HitboxFields() = default;
    HitboxFields(const HitboxFields&) = delete;
    HitboxFields& operator=(const HitboxFields&) = delete;
    ~HitboxFields() { Restore(); }
    bool Apply(void* target, float scale) noexcept {
        Restore();
        if (!target) return false;
        std::array<const void*, maximumColliders> list{};
        std::size_t count = 0;
        if (!CopyList(Field<void*>(target, 0xa8), list.data(), list.size(), count)) return false;
        for (std::size_t i = 0; i < count; ++i) {
            auto* hitbox = const_cast<void*>(list[i]);
            if (!hitbox) continue;
            const float size = Field<float>(hitbox, 0x14);
            const float expanded = size * scale;
            if (!std::isfinite(size) || size <= 0.f || !std::isfinite(expanded)) continue;
            std::memcpy(static_cast<unsigned char*>(hitbox) + 0x14, &expanded, sizeof(expanded));
            fields_[count_++] = {hitbox, size};
        }
        if (count_ > 0) fieldScaling = true;
        return count_ > 0;
    }
    void Restore() noexcept {
        for (std::size_t i = 0; i < count_; ++i)
            std::memcpy(static_cast<unsigned char*>(fields_[i].hitbox) + 0x14,
                        &fields_[i].size, sizeof(float));
        count_ = 0;
        fieldScaling = false;
    }
};

bool Weapon(WeaponTrace original, void* instance, int type, void* target, void* collider,
            Point start, Point direction, void* impact, Metadata info) {
    if (!original) return false;
    const auto invoke = [&] { return original(instance, type, target, collider, start, direction, impact, info); };
    const auto options = Read();
    if (!license::Allows(license::Capability::Features) || currentTrace || !available.load(std::memory_order_acquire) || !options.enabled ||
        options.scale <= 1.f || !instance || !target || !impact || type != 1)
        return invoke();
    // A trace that is already running under inflated fields must not inflate
    // them again; it simply runs.
    if (fieldScaling) return invoke();
    HitboxFields fields;
    if (fields.Apply(target, options.scale)) return invoke();
    if (!EnemyCached(instance, target)) return invoke();
    TraceContext context;
    context.scale = options.scale;
    if (!CollidersCached(target, context)) return invoke();
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
bool Capsule(void* instance, Point center, Rotation rotation, float axisLength, float radius,
             Point start, Point end, HitResult* result, Metadata info) {
    if (!originalCapsule) return false;
    ExpandCapsule(primitiveScale, axisLength, radius);
    PrimitiveScope neutral(1.f);
    return originalCapsule(instance, center, rotation, axisLength, radius, start, end, result, info);
}
bool Avatar(void* instance, void* collider, Point start, Point end, HitResult* result, Metadata info) {
    if (!originalAvatarCapsule) return false;
    float scale = 1.f;
    if (available.load(std::memory_order_acquire) && currentTrace && currentTrace->Contains(collider) &&
        BodyGroup(Field<std::uint16_t>(collider, 0x20)) && Field<unsigned char>(collider, 0x50) == 1)
        scale = currentTrace->scale;
    PrimitiveScope scope(scale);
    return originalAvatarCapsule(instance, collider, start, end, result, info);
}
bool Sphere(void* instance, Point origin, float radius, Point start, Point direction, float length,
            HitResult* result, Metadata info) {
    if (!originalSphere) return false;
    if (sphereAvailable.load(std::memory_order_acquire)) ExpandSphere(primitiveScale, radius);
    PrimitiveScope neutral(1.f);
    return originalSphere(instance, origin, radius, start, direction, length, result, info);
}
bool AvatarSphere(void* instance, void* collider, Point start, Point end, HitResult* result, Metadata info) {
    if (!originalAvatarSphere) return false;
    float scale = 1.f;
    if (available.load(std::memory_order_acquire) && sphereAvailable.load(std::memory_order_acquire) &&
        currentTrace && currentTrace->Contains(collider) && BodyGroup(Field<std::uint16_t>(collider, 0x20)) &&
        Field<unsigned char>(collider, 0x50) == 1) scale = currentTrace->scale;
    PrimitiveScope scope(scale);
    return originalAvatarSphere(instance, collider, start, end, result, info);
}
bool Aabb(void* instance, Point center, Point extent, Point start, Point end, float* hit, Metadata info) {
    if (!originalAabb) return false;
    if (obbAvailable.load(std::memory_order_acquire)) ExpandExtents(primitiveScale,extent);
    PrimitiveScope neutral(1.f);
    return originalAabb(instance,center,extent,start,end,hit,info);
}
bool AvatarObb(void* instance, void* collider, Point start, Point end, HitResult* result, Metadata info) {
    if (!originalAvatarObb) return false;
    float scale = 1.f;
    if (available.load(std::memory_order_acquire) && obbAvailable.load(std::memory_order_acquire) &&
        currentTrace && currentTrace->Contains(collider) && BodyGroup(Field<std::uint16_t>(collider,0x20)) &&
        Field<unsigned char>(collider,0x50) == 1) scale = currentTrace->scale;
    PrimitiveScope scope(scale);
    return originalAvatarObb(instance,collider,start,end,result,info);
}
void UpgradeObb(std::uintptr_t base, HookInstaller hook) noexcept {
    if (obbAvailable.load() || obbStatus.load() != Status::Waiting) return;
    for (const auto& entry : obbAnchors) {
        if (!frame_rate::Executable(base + entry.rva,entry.bytes.size()) ||
            std::memcmp(reinterpret_cast<const void*>(base + entry.rva),entry.bytes.data(),entry.bytes.size()) != 0) {
            obbStatus = Status::Unsupported;
            return;
        }
    }
    hook(reinterpret_cast<void*>(base + 0x66FC548),reinterpret_cast<void*>(AvatarObb),reinterpret_cast<void**>(&originalAvatarObb));
    if (!originalAvatarObb) {obbStatus = Status::HookFailed;return;}
    hook(reinterpret_cast<void*>(base + 0x66F82A8),reinterpret_cast<void*>(Aabb),reinterpret_cast<void**>(&originalAabb));
    if (!originalAabb) {obbStatus = Status::HookFailed;return;}
    obbStatus = Status::Ready;
    obbAvailable.store(true,std::memory_order_release);
}
void UpgradeSphere(std::uintptr_t base, HookInstaller hook) noexcept {
    if (sphereAvailable.load() || sphereStatus.load() != Status::Waiting) return;
    for (const auto& entry : sphereAnchors) {
        if (!frame_rate::Executable(base + entry.rva, entry.bytes.size()) ||
            std::memcmp(reinterpret_cast<const void*>(base + entry.rva),entry.bytes.data(),entry.bytes.size()) != 0) {
            sphereStatus = Status::Unsupported;
            return;
        }
    }
    const auto& entry = sphereEncoded[0];
    if (!frame_rate::Executable(base + entry.rva, entry.bytes.size())) {
        sphereStatus = Status::Unsupported;
        return;
    }
    const auto* bytes = reinterpret_cast<const unsigned char*>(base + entry.rva);
    if (std::memcmp(bytes,entry.bytes.data(),entry.bytes.size()) == 0 || !StackEntry(bytes)) return;
    hook(reinterpret_cast<void*>(base + entry.rva),reinterpret_cast<void*>(AvatarSphere),reinterpret_cast<void**>(&originalAvatarSphere));
    if (!originalAvatarSphere) {sphereStatus = Status::HookFailed;return;}
    hook(reinterpret_cast<void*>(base + 0x66F286C),reinterpret_cast<void*>(Sphere),reinterpret_cast<void**>(&originalSphere));
    if (!originalSphere) {sphereStatus = Status::HookFailed;return;}
    sphereStatus = Status::Ready;
    sphereAvailable.store(true,std::memory_order_release);
}

}

bool Install(std::uintptr_t base, HookInstaller hook) noexcept {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (!base || !hook) return false;
    if (available.load()) {UpgradeObb(base,hook);UpgradeSphere(base,hook);return true;}
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
    localPawn = reinterpret_cast<Getter>(base + 0x8012814);
    matchGame = reinterpret_cast<Getter>(base + 0x801191C);
    struct Hook { std::uintptr_t rva; void* replacement; void** original; };
    const Hook hooks[]{
        {0x66F2A78, reinterpret_cast<void*>(Capsule), reinterpret_cast<void**>(&originalCapsule)},
        {0x66FCA40, reinterpret_cast<void*>(Avatar), reinterpret_cast<void**>(&originalAvatarCapsule)},
        {0xC2BE0C8, reinterpret_cast<void*>(Base), reinterpret_cast<void**>(&originalBase)},
        {0xD52E8E4, reinterpret_cast<void*>(Instant), reinterpret_cast<void**>(&originalInstant)}
    };
    for (const auto& target : hooks) {
        hook(reinterpret_cast<void*>(base + target.rva), target.replacement, target.original);
        if (!*target.original) { status = Status::HookFailed; return false; }
    }
    status = Status::Ready;
    available.store(true, std::memory_order_release);
    UpgradeObb(base,hook);
    UpgradeSphere(base,hook);
    return true;
}
}
#else
namespace hitbox {
bool Install(std::uintptr_t, HookInstaller) noexcept { return false; }
}
#endif
