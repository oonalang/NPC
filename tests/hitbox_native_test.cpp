#include "../src/main/jni/System/Hooks/Hitbox.cpp"
#include "../src/main/jni/Trigger/NativeDriver.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include "license_gate_stub.h"

using namespace hitbox;

namespace {
template<class Buffer, class Value>
void Put(Buffer& buffer, std::size_t offset, Value value) {
    assert(offset + sizeof(value) <= buffer.size());
    std::memcpy(buffer.data() + offset, &value, sizeof(value));
}
bool Near(float a, float b) { return std::fabs(a - b) < .00001f; }
bool Same(Point a, Point b) { return Near(a.x,b.x) && Near(a.y,b.y) && Near(a.z,b.z); }

struct Fixture {
    alignas(8) std::array<unsigned char,0x560> local{},target{},other{};
    alignas(8) std::array<unsigned char,0x190> match{};
    alignas(8) std::array<unsigned char,0x40> fire{};
    alignas(8) std::array<unsigned char,0x60> collider{},unselected{};
    alignas(8) std::array<unsigned char,0x30> enemies{},enemyItems{},regions{},regionItems{};
    HitResult hit{};
    Point start{3.22f,4.f,0.f},direction{0.f,0.f,1.f};
    const Point center{3.f,4.f,10.f};
    const Rotation rotation{.1f,.2f,.3f,.9f};
    const void* weaponInfo = reinterpret_cast<void*>(0x1234);
    const void* avatarInfo = reinterpret_cast<void*>(0x5678);
    const void* capsuleInfo = reinterpret_cast<void*>(0x9abc);
    void* parent = reinterpret_cast<void*>(1);
    bool parentAlive = true, nested = false, throwMath = false, sphere = false, obb = false;
    float fraction = 0.f;
    int weaponCalls = 0, avatarCalls = 0, mathCalls = 0;
    int type = 1;
    void* queryTarget = target.data();
    void* selected = collider.data();
    std::vector<float> scales;

    Fixture() {
        Put(local,0x550,std::uint8_t{1});Put(target,0x550,std::uint8_t{1});
        Put(fire,0x28,local.data());Put(match,0x178,enemies.data());
        Put(enemies,0x10,enemyItems.data());Put(enemies,0x18,1);
        Put(enemyItems,0x18,std::uint64_t{1});Put(enemyItems,0x20,target.data());
        Put(target,0xa8,regions.data());Put(regions,0x10,regionItems.data());Put(regions,0x18,1);
        Put(regionItems,0x18,std::uint64_t{1});Put(regionItems,0x20,collider.data());
        for(auto* shape : {&collider,&unselected}) {
            Put(*shape,0x20,std::uint16_t{3});Put(*shape,0x40,parent);Put(*shape,0x50,std::uint8_t{1});
        }
    }
    bool Shot(bool instant = false) {
        weaponCalls=avatarCalls=mathCalls=0;scales.clear();
        hit={{0,0,0},{0,0,0},1000.f,0,nullptr};
        const bool result=(instant?Instant:Base)(fire.data(),type,queryTarget,selected,start,direction,&hit,weaponInfo);
        assert(!currentTrace && primitiveScale==1.f);
        return result;
    }
};
Fixture* active = nullptr;

void* Local(void* receiver, const void* info) {
    assert(!receiver && !info);return active->local.data();
}
void* Match(void* receiver, const void* info) {
    assert(!receiver && !info);return active->match.data();
}
bool MathOriginal(void* receiver, Point center, Rotation rotation, float axis, float radius,
                  Point start, Point end, HitResult* output, const void* info) {
    auto& f=*active;++f.mathCalls;
    assert(receiver==f.parent && info==f.capsuleInfo && output==&f.hit);
    assert(Same(center,f.center) && std::memcmp(&rotation,&f.rotation,sizeof(rotation))==0);
    assert(Same(start,f.start) && Same(end,{f.start.x,f.start.y,f.start.z+20.f}));
    assert(primitiveScale==1.f && Near(axis/.8f,radius/.2f));
    f.scales.push_back(radius/.2f);
    if(f.throwMath) throw 7;
    const float dx=start.x-center.x;
    if(std::fabs(dx)>radius) return false;
    const float z=center.z-std::sqrt(radius*radius-dx*dx);
    output->location={start.x,start.y,z};
    output->normal={dx/radius,0.f,(z-center.z)/radius};
    output->distance=z-start.z;
    output->material=13;output->collider=f.selected;
    return true;
}
bool AvatarOriginal(void* receiver, void* collider, Point start, Point end, HitResult* output, const void* info) {
    auto& f=*active;++f.avatarCalls;
    assert(receiver==nullptr && collider==f.selected && info==f.avatarInfo && output==&f.hit);
    if(!f.parentAlive || !Field<void*>(collider,0x40)) return false;
    return Capsule(f.parent,f.center,f.rotation,.8f,.2f,start,end,output,f.capsuleInfo);
}
bool SphereMathOriginal(void* receiver, Point center, float radius, Point start, Point direction,
                        float length, HitResult* output, const void* info) {
    auto& f=*active;++f.mathCalls;
    assert(receiver==f.parent && info==f.capsuleInfo && output==&f.hit && Same(center,f.center));
    assert(Same(start,f.start)&&Same(direction,{0,0,1})&&length==20.f&&primitiveScale==1.f);
    f.scales.push_back(radius/.2f);
    const float dx=start.x-center.x;
    if(std::fabs(dx)>radius) return false;
    const float z=center.z-std::sqrt(radius*radius-dx*dx);
    output->location={start.x,start.y,z};output->normal={dx/radius,0.f,(z-center.z)/radius};
    output->distance=z-start.z;output->material=13;output->collider=f.selected;
    return true;
}
bool AvatarSphereOriginal(void* receiver, void* collider, Point start, Point end, HitResult* output, const void* info) {
    auto& f=*active;++f.avatarCalls;
    assert(receiver==nullptr&&collider==f.selected&&info==f.avatarInfo&&output==&f.hit);
    assert(Same(end,{start.x,start.y,start.z+20.f}));
    if(!f.parentAlive||!Field<void*>(collider,0x40)) return false;
    return Sphere(f.parent,f.center,.2f,start,{0,0,1},20.f,output,f.capsuleInfo);
}
bool AabbOriginal(void* receiver,Point center,Point extent,Point start,Point end,float* fraction,const void* info) {
    auto& f=*active;++f.mathCalls;
    assert(receiver==f.parent&&info==f.capsuleInfo&&fraction==&f.fraction&&Same(center,f.center));
    assert(Same(start,f.start)&&Same(end,{f.start.x,f.start.y,f.start.z+20.f})&&primitiveScale==1.f);
    assert(Near(extent.x/.2f,extent.y/.4f)&&Near(extent.x,extent.z));f.scales.push_back(extent.x/.2f);
    const float a[]{start.x,start.y,start.z},b[]{end.x,end.y,end.z},c[]{center.x,center.y,center.z},half[]{extent.x,extent.y,extent.z};
    float near=0.f,far=1.f;
    for(int i=0;i<3;++i) {
        const float d=b[i]-a[i];
        if(std::fabs(d)<.00001f) {if(a[i]<c[i]-half[i]||a[i]>c[i]+half[i]) return false;continue;}
        float lo=(c[i]-half[i]-a[i])/d,hi=(c[i]+half[i]-a[i])/d;
        if(lo>hi) std::swap(lo,hi);
        near=std::max(near,lo);far=std::min(far,hi);if(near>far) return false;
    }
    *fraction=near;return true;
}
bool AvatarObbOriginal(void* receiver,void* collider,Point start,Point end,HitResult* output,const void* info) {
    auto& f=*active;++f.avatarCalls;
    assert(receiver==nullptr&&collider==f.selected&&info==f.avatarInfo&&output==&f.hit);
    if(!f.parentAlive||!Field<void*>(collider,0x40)) return false;
    if(!Aabb(f.parent,f.center,{.2f,.4f,.2f},start,end,&f.fraction,f.capsuleInfo)) return false;
    output->location={start.x,start.y,start.z+(end.z-start.z)*f.fraction};output->normal={0,0,-1};
    output->distance=output->location.z-start.z;output->material=13;output->collider=f.selected;return true;
}
bool WeaponOriginal(void* receiver, int type, void* target, void* collider,
                    Point start, Point direction, void* impact, const void* info) {
    auto& f=*active;++f.weaponCalls;
    assert(receiver==f.fire.data() && target==f.queryTarget && collider==f.selected && type==f.type);
    assert(impact==&f.hit && info==f.weaponInfo && Same(start,f.start) && Same(direction,f.direction));
    if(f.nested) {
        f.nested=false;
        return Instant(receiver,type,target,collider,start,direction,impact,info);
    }
    return (f.obb?AvatarObb:f.sphere?AvatarSphere:Avatar)(nullptr,collider,start,{start.x,start.y,start.z+20.f},static_cast<HitResult*>(impact),f.avatarInfo);
}
void Bind(Fixture& fixture) {
    active=&fixture;available=true;
    originalBase=originalInstant=WeaponOriginal;
    originalAvatarCapsule=AvatarOriginal;originalCapsule=MathOriginal;
    originalAvatarSphere=AvatarSphereOriginal;originalSphere=SphereMathOriginal;sphereAvailable=true;
    originalAvatarObb=AvatarObbOriginal;originalAabb=AabbOriginal;obbAvailable=true;
    localPawn=Local;matchGame=Match;Publish(true,1.21f);
}
void NearMissAndPriority() {
    Fixture f;Bind(f);
    for(bool instant : {false,true}) {
        assert(f.Shot(instant) && f.weaponCalls==2 && f.avatarCalls==2 && f.mathCalls==2);
        assert(Near(f.scales[0],1.f) && Near(f.scales[1],1.21f));
        assert(f.hit.collider==f.collider.data() && f.hit.material==13 && Same(f.hit.location,{f.start.x,4.f,f.hit.distance}));
        assert(Field<std::uint16_t>(f.collider.data(),0x20)==3);
        f.start.x=3.1f;Put(f.collider,0x20,std::uint16_t{1});
        assert(f.Shot(instant) && f.weaponCalls==1 && f.mathCalls==1 && Near(f.scales[0],1.f));
        assert(Field<std::uint16_t>(f.collider.data(),0x20)==1);
        f.start.x=3.22f;Put(f.collider,0x20,std::uint16_t{3});
    }
    f.start.x=5.f;
    assert(!f.Shot() && f.weaponCalls==2 && f.mathCalls==2 && f.hit.collider==nullptr);
    f.start.x=3.22f;f.nested=true;
    assert(f.Shot() && f.weaponCalls==3 && f.mathCalls==2 && Near(f.scales.back(),1.21f));
}
void InvalidParentsAndUnselectedRegions() {
    Fixture f;Bind(f);
    f.parentAlive=false;
    assert(!f.Shot() && f.weaponCalls==2 && f.avatarCalls==2 && f.mathCalls==0);
    f.parentAlive=true;Put(f.collider,0x40,static_cast<void*>(nullptr));
    assert(!f.Shot() && f.mathCalls==0);
    Put(f.collider,0x40,f.parent);
    for(std::uint16_t group : {0,6,32}) {
        Put(f.collider,0x20,group);
        assert(!f.Shot() && f.mathCalls==2 && Near(f.scales[1],1.f));
    }
    Put(f.collider,0x20,std::uint16_t{3});Put(f.collider,0x50,std::uint8_t{0});
    assert(!f.Shot() && Near(f.scales[1],1.f));
    Put(f.collider,0x50,std::uint8_t{1});f.selected=f.unselected.data();
    assert(!f.Shot() && Near(f.scales[1],1.f));
    f.selected=f.collider.data();
    assert(f.Shot() && Near(f.scales[1],1.21f));
}
void ScopeGating() {
    Fixture f;Bind(f);
    const auto stock=[&] {assert(!f.Shot() && f.weaponCalls==1 && f.mathCalls==1 && Near(f.scales[0],1.f));};
    Publish(false,1.21f);stock();Publish(true,1.f);stock();Publish(true,1.21f);
    available=false;stock();available=true;
    license_test::allowed=false;stock();license_test::allowed=true;
    Put(f.fire,0x28,f.other.data());stock();Put(f.fire,0x28,f.local.data());
    Put(f.local,0x550,std::uint8_t{0});stock();Put(f.local,0x550,std::uint8_t{1});
    Put(f.target,0x550,std::uint8_t{0});stock();Put(f.target,0x550,std::uint8_t{1});
    Put(f.enemyItems,0x20,f.other.data());stock();Put(f.enemyItems,0x20,f.target.data());
    f.queryTarget=f.local.data();stock();f.queryTarget=f.target.data();
    f.type=2;stock();f.type=1;
    localPawn=nullptr;stock();localPawn=Local;
    matchGame=nullptr;stock();matchGame=Match;
    Put(f.regions,0x18,129);stock();Put(f.regions,0x18,1);
    assert(f.Shot());
    f.throwMath=true;
    try {f.Shot();assert(false);} catch(int value) {assert(value==7);}
    assert(!currentTrace && primitiveScale==1.f);
    f.throwMath=false;assert(f.Shot());
    TraceContext context;context.count=1;context.colliders[0]=f.collider.data();context.scale=1.21f;
    {
        TraceScope scope(context);
        available=false;
        const Point end{f.start.x,f.start.y,f.start.z+20.f};
        assert(!Avatar(nullptr,f.selected,f.start,end,&f.hit,f.avatarInfo));
        assert(Near(f.scales.back(),1.f));
        available=true;
    }
}
void SphereSizingAndHeadGroups() {
    Fixture f;Bind(f);f.sphere=true;Put(f.collider,0x20,std::uint16_t{1});
    for(bool instant:{false,true}) {
        for(float scale:{1.f,1.21f,2.f,5.f}) {
            Publish(true,scale);f.start.x=3.22f;
            assert(f.Shot(instant)==(scale>1.f));
            assert(f.weaponCalls==(scale>1.f?2:1));
            assert(Near(f.scales.back(),scale));
            assert(Field<std::uint16_t>(f.collider.data(),0x20)==1);
            f.start.x=3.1f;assert(f.Shot(instant)&&f.weaponCalls==1&&Near(f.scales[0],1.f));
        }
    }
    f.start.x=3.5f;Publish(true,1.21f);assert(!f.Shot());
    Publish(true,3.f);assert(f.Shot()&&Near(f.scales.back(),3.f));
    f.start.x=3.22f;Publish(true,1.21f);sphereAvailable=false;
    assert(!f.Shot()&&Near(f.scales.back(),1.f));
    f.sphere=false;assert(f.Shot());f.sphere=true;sphereAvailable=true;
    f.parentAlive=false;assert(!f.Shot()&&f.mathCalls==0);f.parentAlive=true;
    Put(f.collider,0x20,std::uint16_t{6});assert(!f.Shot()&&Near(f.scales.back(),1.f));
    Put(f.collider,0x20,std::uint16_t{1});Put(f.collider,0x50,std::uint8_t{0});
    assert(!f.Shot()&&Near(f.scales.back(),1.f));Put(f.collider,0x50,std::uint8_t{1});
    f.selected=f.unselected.data();assert(!f.Shot()&&Near(f.scales.back(),1.f));f.selected=f.collider.data();
    license_test::allowed=false;assert(!f.Shot()&&f.weaponCalls==1);license_test::allowed=true;
    assert(f.Shot()&&f.hit.collider==f.collider.data());
}
void ObbSizingAndHeadGroups() {
    Fixture f;Bind(f);f.obb=true;Put(f.collider,0x20,std::uint16_t{1});
    for(float scale:{1.f,1.21f,2.f,5.f}) {
        Publish(true,scale);f.start.x=3.22f;assert(f.Shot()==(scale>1.f));
        assert(Near(f.scales.back(),scale)&&Field<std::uint16_t>(f.collider.data(),0x20)==1);
        f.start.x=3.1f;assert(f.Shot()&&f.weaponCalls==1&&Near(f.scales[0],1.f));
    }
    f.start.x=3.5f;Publish(true,1.21f);assert(!f.Shot());Publish(true,3.f);assert(f.Shot());
    f.start.x=3.22f;Publish(true,1.21f);obbAvailable=false;assert(!f.Shot()&&Near(f.scales.back(),1.f));
    f.obb=false;assert(f.Shot());f.obb=true;obbAvailable=true;
    f.parentAlive=false;assert(!f.Shot()&&f.mathCalls==0);f.parentAlive=true;
    Put(f.collider,0x20,std::uint16_t{6});assert(!f.Shot()&&Near(f.scales.back(),1.f));
    Put(f.collider,0x20,std::uint16_t{1});license_test::allowed=false;assert(!f.Shot()&&f.weaponCalls==1);license_test::allowed=true;
}
bool AlreadyScaledSphere(void*,Point,float radius,Point,Point,float,HitResult*,const void*) {
    assert(Near(radius,.242f)&&primitiveScale==1.f);return true;
}
bool CapsuleEndSphere(void* receiver,Point center,Rotation,float axis,float radius,
                      Point start,Point,HitResult* output,const void* info) {
    assert(Near(axis,.968f)&&Near(radius,.242f)&&primitiveScale==1.f);
    return Sphere(receiver,center,radius,start,{0,0,1},20.f,output,info);
}
void NestedSphereIsNotScaledTwice() {
    sphereAvailable=true;originalSphere=AlreadyScaledSphere;originalCapsule=CapsuleEndSphere;
    PrimitiveScope scope(1.21f);
    assert(Capsule(nullptr,{0,0,0},{0,0,0,1},.8f,.2f,{0,0,-5},{0,0,5},nullptr,nullptr));
    assert(Near(primitiveScale,1.21f));
}
int nestedCalls=0;
bool NestedMath(void* receiver, Point center, Rotation rotation, float axis, float radius,
                Point start, Point end, HitResult* output, const void* info) {
    assert(primitiveScale==1.f && Near(axis,.968f) && Near(radius,.242f));
    if(++nestedCalls==1) assert(Capsule(receiver,center,rotation,axis,radius,start,end,output,info));
    return true;
}
void PrimitiveReentry() {
    originalCapsule=NestedMath;
    {
        PrimitiveScope scope(1.21f);
        assert(Capsule(nullptr,{1,2,3},{0,0,0,1},.8f,.2f,{1,2,0},{1,2,6},nullptr,nullptr));
        assert(nestedCalls==2 && Near(primitiveScale,1.21f));
    }
    assert(primitiveScale==1.f);
}
}

extern "C" int dl_iterate_phdr(int(*)(dl_phdr_info*,std::size_t,void*),void*) {return 0;}
namespace PhantomTrigger {
bool InstallNativeDriver(std::uintptr_t) noexcept {return false;}
void NativeDriverFailure() noexcept {}
void NativeDriverPending() noexcept {}
}

int main() {
    NearMissAndPriority();InvalidParentsAndUnselectedRegions();ScopeGating();PrimitiveReentry();SphereSizingAndHeadGroups();ObbSizingAndHeadGroups();NestedSphereIsNotScaledTwice();
    std::puts("Hitbox native callbacks: capsule/OBB/sphere slider scaling, head-group preservation, nested end-sphere scaling, 1.21x side shots, direct-hit priority, invalid parents, unchanged rays/results/metadata, ownership gating and nested scope restoration passed (stubbed game methods)");
}
