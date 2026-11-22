#include "../src/main/jni/Trigger/WorldQuery.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace PhantomTrigger;
namespace {
struct Fixture {
    RayContacts contacts{{{100,18.f}}};
    std::size_t count=1;
    bool complete=true,blocked=true;
    float nearest=18.f;
    int fastCalls=0,contactCalls=0,classifications=0;
    bool prefer=false;
    ContactKind Classify(std::uintptr_t collider) {
        ++classifications;
        return collider==500?ContactKind::BulletIgnored:collider==100?ContactKind::Character:collider==200?ContactKind::Glass:
            collider==300?ContactKind::Unavailable:ContactKind::Solid;
    }
    Occlusion Query(float distance=20.f) {
        return QueryFreshWorld(distance,prefer,
            [&](float& out) {++fastCalls;out=nearest;return blocked;},
            [&](RayContacts& out,std::size_t& size) {++contactCalls;out=contacts;size=count;return complete;},
            [&](std::uintptr_t collider) {return Classify(collider);});
    }
    void ResetCounts() {fastCalls=contactCalls=classifications=0;}
};
}
int main() {
    Fixture f;
    f.blocked=false;
    assert(f.Query()==Occlusion::Clear && f.fastCalls==1 && !f.contactCalls && !f.prefer);
    f.blocked=true;f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && f.fastCalls==1 && f.contactCalls==1 && f.prefer);
    f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && !f.fastCalls && f.contactCalls==1);
    f.contacts={{{400,5.f},{100,18.f}}};f.count=2;f.ResetCounts();
    assert(f.Query()==Occlusion::Blocked && !f.fastCalls && f.contactCalls==1 && f.classifications==1);
    f.contacts={{{300,5.f},{100,18.f}}};f.ResetCounts();
    assert(f.Query()==Occlusion::Unavailable && !f.fastCalls && f.contactCalls==1);
    f.contacts={{{200,5.f},{100,18.f}}};f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && !f.fastCalls && f.contactCalls==1 && f.classifications==2);
    f.contacts={{{500,.6096f}}};f.count=1;f.ResetCounts();
    assert(f.Query(27.6363f)==Occlusion::Clear && !f.fastCalls && f.contactCalls==1);
    f.contacts={{{500,.6096f},{400,12.f}}};f.count=2;f.ResetCounts();
    assert(f.Query(27.6363f)==Occlusion::Blocked && !f.fastCalls && f.contactCalls==1);
    f.contacts={{{500,.6096f},{200,3.f},{100,18.f}}};f.count=3;f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && !f.fastCalls && f.contactCalls==1);
    for(int material=-32;material<=256;++material)
        assert(IsBulletIgnoredWorldMaterial(material)==(material==11 || material==13 || material==15));
    assert(!IsBulletIgnoredWorldMaterial(std::numeric_limits<int>::min()) &&
           !IsBulletIgnoredWorldMaterial(std::numeric_limits<int>::max()));
    assert(!IsBulletIgnoredWorldMaterial(15,101) && !IsBulletIgnoredWorldMaterial(11,101) &&
           !IsBulletIgnoredWorldMaterial(13,101));
    f.count=0;f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && !f.fastCalls && f.contactCalls==1 && !f.prefer);
    f.blocked=false;f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && f.fastCalls==1 && !f.contactCalls);
    f.prefer=true;f.complete=false;f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && f.fastCalls==1 && f.contactCalls==1 && !f.prefer);
    f.prefer=true;f.blocked=true;f.ResetCounts();
    assert(f.Query()==Occlusion::Unavailable && f.fastCalls==1 && f.contactCalls==1 && !f.prefer);
    f.prefer=true;f.nearest=20.f;f.ResetCounts();
    assert(f.Query()==Occlusion::Clear && f.fastCalls==1 && f.contactCalls==1 && !f.prefer);
    f.prefer=true;f.nearest=-1.f;f.ResetCounts();
    assert(f.Query()==Occlusion::Unavailable && f.fastCalls==1 && f.contactCalls==1 && !f.prefer);
    f.complete=true;f.prefer=true;f.count=MaxGlassContacts+1;f.ResetCounts();
    assert(f.Query()==Occlusion::Unavailable && !f.fastCalls && f.contactCalls==1 && !f.prefer);
    f.prefer=true;f.count=2;f.contacts={{{100,18.f},{400,std::numeric_limits<float>::quiet_NaN()}}};f.ResetCounts();
    assert(f.Query()==Occlusion::Unavailable && !f.fastCalls && f.contactCalls==1 && !f.classifications);
    f.ResetCounts();
    for(float bad:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        assert(f.Query(bad)==Occlusion::Unavailable && !f.fastCalls && !f.contactCalls);
    WorldQueryPreference preference;
    ContextIdentity context{1,2,3,4,5};
    assert(!preference.For(context));preference.For(context)=true;assert(preference.For(context));
    for(unsigned field=0;field<5;++field) {
        auto changed=context;
        if(field==0)++changed.match;
        if(field==1)++changed.pawn;
        if(field==2)++changed.weapon;
        if(field==3)++changed.component;
        if(field==4)++changed.inputMode;
        assert(!preference.For(changed));preference.For(changed)=true;
        assert(!preference.For(context));preference.For(context)=true;
    }
    unsigned equivalents=0;
    for(unsigned code=0;code<4096;++code) {
        Fixture sample;sample.prefer=true;
        sample.count=code%5;
        unsigned bits=code;
        for(std::size_t index=0;index<sample.count;++index) {
            sample.contacts[index]={100+(bits%4)*100,2.f+static_cast<float>(index)*4.f};bits/=4;
        }
        sample.blocked=sample.count!=0;
        sample.nearest=sample.count?sample.contacts[0].distance:0.f;
        auto expectedContacts=sample.contacts;
        const auto expected=ResolveWorldOcclusion(20.f,sample.blocked,sample.nearest,
            [&](RayContacts& out,std::size_t& size) {out=expectedContacts;size=sample.count;return true;},
            [&](std::uintptr_t collider) {return sample.Classify(collider);});
        sample.ResetCounts();
        assert(sample.Query()==expected && sample.fastCalls==0 && sample.contactCalls==1);
        ++equivalents;
    }
    Fixture stable;
    for(int update=0;update<120;++update) assert(stable.Query()==Occlusion::Clear);
    assert(stable.fastCalls==1 && stable.contactCalls==120);
    stable.contacts={{{400,1.f}}};stable.count=1;
    assert(stable.Query()==Occlusion::Blocked);
    std::printf("Fresh world queries: %u policy equivalence scenarios; fresh cover/glass/unknown/empty transitions; invalid data, context reset and failed-query bounds passed\n",equivalents);
    std::puts("Material 15 at 0.6096 m before a 27.6363 m target clears; a real wall behind it still blocks; native bullet-mask bounds passed");
    std::puts("Stable contact path: 121 world queries over 120 updates versus the previous 240; every update samples fresh collision data");
}
