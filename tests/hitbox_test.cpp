#include "../src/main/jni/System/Hooks/Hitbox.h"
#include "../src/main/jni/System/Hooks/HitboxProfile.h"
#include <cassert>
#include <cfloat>
#include <cstdio>
#include <limits>
#include <thread>

using namespace hitbox;

static bool Near(float a, float b) { return std::fabs(a - b) < .0002f; }
static bool Same(Point a, Point b) { return Near(a.x,b.x) && Near(a.y,b.y) && Near(a.z,b.z); }
static bool Sphere(Point center, float radius, Point start, Point end, HitResult& result) {
    const double dx = double(end.x)-start.x, dy = double(end.y)-start.y, dz = double(end.z)-start.z;
    const double ox = double(start.x)-center.x, oy = double(start.y)-center.y, oz = double(start.z)-center.z;
    const double a = dx*dx+dy*dy+dz*dz, b = 2*(dx*ox+dy*oy+dz*oz), c = ox*ox+oy*oy+oz*oz-double(radius)*radius;
    const double discriminant = b*b-4*a*c;
    if (a <= 0 || discriminant < 0) return false;
    double t = (-b-std::sqrt(discriminant))/(2*a);
    if (t < 0) t = (-b+std::sqrt(discriminant))/(2*a);
    if (t < 0 || t > 1) return false;
    result.location = {float(start.x+dx*t),float(start.y+dy*t),float(start.z+dz*t)};
    result.normal = {(result.location.x-center.x)/radius,(result.location.y-center.y)/radius,(result.location.z-center.z)/radius};
    result.material = 13;
    return true;
}
static bool HorizontalCapsule(Point center,float radius,float halfLength,Point start,Point end,HitResult& result) {
    assert(Near(start.y,end.y));
    const Point closest{center.x,std::clamp(start.y,center.y-halfLength,center.y+halfLength),center.z};
    return Sphere(closest,radius,start,end,result);
}

static void Geometry() {
    for (float scale : {1.f,1.21f,1.25f,2.f,5.f})
        for (float y : {-2.f,-.6f,0.f,.6f,1.1f})
            for (float x : {-.5f,0.f,.15f,.22f,.5f}) {
                const Point center{4,-3,7},start{4+x,-3+y,1},end{4+x,-3+y,13};
                float axisLength=1.2f,radius=.2f;
                assert(ExpandCapsule(scale,axisLength,radius)==(scale>1.f));
                HitResult actual{},expected{};
                const bool hit=HorizontalCapsule(center,radius,axisLength*.5f,start,end,actual);
                const bool reference=HorizontalCapsule(center,.2f*scale,.6f*scale,start,end,expected);
                assert(hit==reference);
                if(hit) assert(Same(actual.location,expected.location)&&Same(actual.normal,expected.normal));
            }
    for(float value : {NAN,INFINITY,-INFINITY,-1.f,0.f,1.f}) {
        float axis=1.2f,radius=.2f;
        assert(!ExpandCapsule(value,axis,radius)&&axis==1.2f&&radius==.2f);
    }
    for(float bad : {NAN,INFINITY,-INFINITY,-1.f}) {
        float axis=bad,radius=.2f;
        assert(!ExpandCapsule(1.21f,axis,radius)&&radius==.2f);
        axis=1.2f;radius=bad;
        assert(!ExpandCapsule(1.21f,axis,radius)&&axis==1.2f);
    }
    float axis=0.f,radius=.2f;
    assert(ExpandCapsule(1.21f,axis,radius)&&axis==0.f&&Near(radius,.242f));
    axis=1.f;radius=0.f;assert(!ExpandCapsule(1.21f,axis,radius)&&axis==1.f);
    axis=FLT_MAX;radius=1.f;assert(!ExpandCapsule(5.f,axis,radius)&&axis==FLT_MAX&&radius==1.f);
    axis=1.f;radius=FLT_MAX;assert(!ExpandCapsule(5.f,axis,radius)&&axis==1.f&&radius==FLT_MAX);
    for(float scale:{1.f,1.21f,2.f,5.f}) {
        float sphere=.2f;assert(ExpandSphere(scale,sphere)==(scale>1.f));assert(Near(sphere,.2f*scale));
    }
    for(float invalid:{0.f,-1.f,NAN,INFINITY,FLT_MAX}) {
        float sphere=invalid;assert(!ExpandSphere(5.f,sphere));
        assert((std::isnan(sphere)&&std::isnan(invalid))||sphere==invalid);
    }
    for(float scale:{1.f,1.21f,2.f,5.f}) {
        Point extent{.2f,.4f,.3f};assert(ExpandExtents(scale,extent)==(scale>1.f));
        assert(Same(extent,{.2f*scale,.4f*scale,.3f*scale}));
    }
    for(Point invalid:{Point{NAN,1,1},Point{-1,1,1},Point{1,INFINITY,1},Point{FLT_MAX,1,1}}) {
        const Point previous=invalid;assert(!ExpandExtents(5.f,invalid));assert(std::memcmp(&invalid,&previous,sizeof(invalid))==0);
    }
    assert(primitiveScale==1.f);
    {
        PrimitiveScope outer(1.21f);assert(Near(primitiveScale,1.21f));
        {PrimitiveScope neutral(1.f);assert(primitiveScale==1.f);}
        assert(Near(primitiveScale,1.21f));
        try {PrimitiveScope inner(3.f);throw 1;} catch(int) {}
        assert(Near(primitiveScale,1.21f));
    }
    assert(primitiveScale==1.f);
}

static void ClassificationAndScope() {
    struct Region {Point center;float radius;int group;};
    const std::array<Region,2> regions{{{{0,0,0},.45f,3},{{0,1.4f,0},.2f,1}}};
    TraceContext context;context.count=2;context.scale=2;
    context.colliders[0]=&regions[0];context.colliders[1]=&regions[1];
    const auto trace=[&](Point start,int expected,int expectedCalls) {
        int calls=0,group=0;HitResult result{};result.distance=FLT_MAX;
        const bool hit=TraceWeapon(context,[&] {
            ++calls;assert(currentTrace);
            const Point end{start.x,start.y,6};
            for(const auto& region:regions) {
                assert(currentTrace->Contains(&region));
                float axis=0.f,radius=region.radius;
                ExpandCapsule(currentTrace->scale,axis,radius);
                if(Sphere(region.center,radius,start,end,result)) {group=region.group;return true;}
            }
            return false;
        });
        assert(hit==(expected!=0)&&group==expected&&calls==expectedCalls&&!currentTrace);
    };
    trace({0,1.4f,-6},1,1);
    trace({0,0,-6},3,1);
    trace({.65f,0,-6},3,2);
    trace({.28f,1.4f,-6},1,2);
    trace({2,0,-6},0,2);
    for(unsigned group=0;group<35;++group)
        assert(BodyGroup(group)==(group>=1&&group<=31&&group!=6));
    assert(!context.Contains(nullptr)&&!context.Contains(&context));
    TraceContext outer=context;outer.scale=3;
    {
        TraceScope scope(outer);int calls=0;
        assert(TraceWeapon(context,[&]{++calls;assert(currentTrace==&outer);return true;}));assert(calls==1);
        {TraceScope inner(context);assert(currentTrace==&context);}
        assert(currentTrace==&outer);
    }
    assert(!currentTrace);
    try {TraceScope scope(outer);throw 1;}catch(int) {}
    assert(!currentTrace);
    const auto worker=[&](float scale) {
        auto own=context;own.scale=scale;
        for(int i=0;i<10000;++i) {
            PrimitiveScope primitive(scale);
            int calls=0;
            assert(TraceWeapon(own,[&] {++calls;assert(currentTrace&&currentTrace->scale==(calls==1?1.f:scale));return calls==2;}));
            assert(calls==2&&!currentTrace&&primitiveScale==scale);
        }
    };
    std::thread a(worker,2.f),b(worker,4.f);a.join();b.join();
}

static void ListsAndSettings() {
    alignas(8) std::array<unsigned char,0x30> list{};
    alignas(8) std::array<unsigned char,0x40> items{};
    auto put=[](auto& bytes,std::size_t offset,auto value){std::memcpy(bytes.data()+offset,&value,sizeof(value));};
    const void* pointer=items.data();put(list,0x10,pointer);put(list,0x18,2);put(items,0x18,std::uint64_t{2});
    const void* first=list.data();const void* second=items.data();put(items,0x20,first);put(items,0x28,second);
    std::array<const void*,2> output{};std::size_t count=99;
    assert(CopyList(list.data(),output.data(),output.size(),count)&&count==2&&output[0]==first&&output[1]==second);
    assert(!CopyList(list.data(),output.data(),1,count)&&count==0);
    for(int size:{-1,0,129,INT32_MAX}) {put(list,0x18,size);assert(!CopyList(list.data(),output.data(),output.size(),count)&&count==0);}
    put(list,0x18,2);put(items,0x18,std::uint64_t{1});assert(!CopyList(list.data(),output.data(),output.size(),count));
    put(items,0x18,std::uint64_t{4097});assert(!CopyList(list.data(),output.data(),output.size(),count));
    assert(!CopyList(nullptr,output.data(),2,count)&&count==0);
    for(float value:{-10.f,0.f,1.f,1.25f,5.f,999.f,NAN,INFINITY,-INFINITY}) {
        Publish(true,value);const auto options=Read();
        assert(options.enabled&&Near(options.scale,NormalizeScale(value)));
        Publish(false,value);assert(!Read().enabled);
    }
    assert(!StackEntry(encoded[0].bytes.data())&&!StackEntry(encoded[1].bytes.data())&&!StackEntry(nullptr));
    assert(StackEntry(anchors[0].bytes.data())&&StackEntry(anchors[2].bytes.data()));
    std::array<unsigned char,16> entry{};const std::uint32_t str=0xf81e0ffe;std::memcpy(entry.data(),&str,4);assert(StackEntry(entry.data()));
    const std::uint32_t bti=0xd503245f;std::memcpy(entry.data(),&bti,4);std::memcpy(entry.data()+4,anchors[0].bytes.data(),4);assert(StackEntry(entry.data()));
}

int main() {
    Geometry();ClassificationAndScope();ListsAndSettings();
    std::puts("Hitbox: capsule dimensions, invalid geometry, normal head/body priority, miss handling, bounded lists, nested/concurrent scopes and settings passed");
}
