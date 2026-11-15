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
static bool Box(Point center, Point extent, Point start, Point end, HitResult& result) {
    const float origin[]{start.x,start.y,start.z},finish[]{end.x,end.y,end.z};
    const float middle[]{center.x,center.y,center.z},half[]{extent.x,extent.y,extent.z};
    float near=0,far=1;int axis=0;float sign=0;
    for (int i=0;i<3;++i) {
        const float delta=finish[i]-origin[i],lo=middle[i]-half[i],hi=middle[i]+half[i];
        if (std::fabs(delta)<1e-8f) {if(origin[i]<lo || origin[i]>hi) return false;continue;}
        float a=(lo-origin[i])/delta,b=(hi-origin[i])/delta;
        const float n=delta>0?-1.f:1.f;
        if(a>b) std::swap(a,b);
        if(a>near) {near=a;axis=i;sign=n;}
        far=std::min(far,b);if(near>far) return false;
    }
    result.location={start.x+(end.x-start.x)*near,start.y+(end.y-start.y)*near,start.z+(end.z-start.z)*near};
    result.normal={axis==0?sign:0,axis==1?sign:0,axis==2?sign:0};
    return true;
}
static bool HorizontalCapsule(Point center,float radius,float halfLength,Point start,Point end,HitResult& result) {
    assert(Near(start.y,end.y));
    const Point closest{center.x,std::clamp(start.y,center.y-halfLength,center.y+halfLength),center.z};
    return Sphere(closest,radius,start,end,result);
}

static void Geometry() {
    int collider=0;
    for (const auto center:{Point{0,0,0},Point{12,-3,4},Point{-31,12,8}})
        for (float scale:{1.25f,2.f,3.5f,5.f})
            for (float x:{-.95f,-.35f,0.f,.35f,.95f,1.4f}) {
                const Point start{center.x+x,center.y,center.z-6},end{center.x+x,center.y,center.z+6};
                HitResult actual{},expected{};actual.distance=FLT_MAX;actual.collider=&collider;
                const bool expanded=ScaledRay(center,start,end,scale,actual,[&](Point a,Point b,HitResult& hit){return Sphere(center,.35f,a,b,hit);});
                const bool reference=Sphere(center,.35f*scale,start,end,expected);
                assert(expanded==reference);
                if(expanded) {
                    assert(Same(actual.location,expected.location)&&Same(actual.normal,expected.normal));
                    assert(Near(actual.distance,actual.location.z-start.z));
                    assert(actual.material==13&&actual.collider==&collider);
                }
                HitResult box{},largeBox{};box.distance=FLT_MAX;
                const bool small=ScaledRay(center,start,end,scale,box,[&](Point a,Point b,HitResult& hit){return Box(center,{.35f,.5f,.2f},a,b,hit);});
                const bool large=Box(center,{.35f*scale,.5f*scale,.2f*scale},start,end,largeBox);
                assert(small==large);
                if(small) assert(Same(box.location,largeBox.location)&&Same(box.normal,largeBox.normal));
            }
    for(float scale:{1.25f,2.f,5.f}) for(float y:{-2.f,-.6f,0.f,.6f,1.1f}) for(float x:{-.5f,0.f,.15f,.5f}) {
        const Point center{4,-3,7},start{4+x,-3+y,1},end{4+x,-3+y,13};
        HitResult capsule{},large{};capsule.distance=FLT_MAX;
        const bool mapped=ScaledRay(center,start,end,scale,capsule,[&](Point a,Point b,HitResult& hit){return HorizontalCapsule(center,.2f,.6f,a,b,hit);});
        const bool expanded=HorizontalCapsule(center,.2f*scale,.6f*scale,start,end,large);
        assert(mapped==expanded);
        if(mapped) assert(Same(capsule.location,large.location)&&Same(capsule.normal,large.normal));
    }
    HitResult result{{1,2,3},{0,1,0},123,9,&collider};const auto before=result;
    assert(!ScaledRay({0,0,0},{0,0,-6},{0,0,-3},2,result,[&](Point a,Point b,HitResult& hit){return Sphere({0,0,0},.5f,a,b,hit);}));
    assert(std::memcmp(&result,&before,sizeof(result))==0);
    for (const auto point:{Point{100,0,0},Point{0,0,20},Point{0,0,-20},Point{NAN,0,0}}) {
        assert(!ScaledRay({0,0,0},{0,0,-6},{0,0,6},2,result,[&](Point,Point,HitResult& hit){hit.location=point;hit.normal={0,1,0};return true;}));
        assert(std::memcmp(&result,&before,sizeof(result))==0);
    }
    int calls=0;
    assert(ScaledRay({0,0,0},{1,2,3},{3,2,1},1,result,[&](Point a,Point b,HitResult&){++calls;assert(Same(a,{1,2,3})&&Same(b,{3,2,1}));return true;}));
    assert(calls==1);
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
                if(ScaledRay(region.center,start,end,currentTrace->scale,result,[&](Point a,Point b,HitResult& out){return Sphere(region.center,region.radius,a,b,out);})) {group=region.group;return true;}
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
            int calls=0;
            assert(TraceWeapon(own,[&] {++calls;assert(currentTrace&&currentTrace->scale==(calls==1?1.f:scale));return calls==2;}));
            assert(calls==2&&!currentTrace);
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
    std::puts("Hitbox: scaled sphere/box/capsule intersections, world-space impacts, normal head/body priority, miss handling, bounded lists, nested/concurrent scopes and settings passed");
}
