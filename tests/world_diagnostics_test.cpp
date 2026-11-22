#include "../src/main/jni/Trigger/WorldDiagnostics.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <limits>
#include <thread>
#include <type_traits>

int main() {
    using namespace PhantomTrigger;
    static_assert(sizeof(WorldDiagnostic)<=64);
    static_assert(std::is_trivially_copyable<WorldDiagnostic>::value);
    const auto initial=GetWorldDiagnostic();
    assert(!initial.sequence && !initial.timeMillis);
    char text[384]="unchanged";
    assert(!FormatWorldDiagnostic(text,sizeof(text),initial,1000) && !text[0]);
    WorldDiagnostic sample;
    sample.result=Occlusion::Blocked;sample.reason=WorldProbeReason::Solid;
    sample.owner=WorldProbeOwner::World;
    sample.targetDistance=37.125f;sample.worldDistance=17.25f;sample.contactDistance=17.25f;
    sample.material=6;sample.group=-1;sample.contacts=3;sample.timeMillis=1000;
    sample.fastQueries=1;sample.contactQueries=1;sample.elapsedMicros=317;
    PublishWorldDiagnostic(sample);
    auto value=GetWorldDiagnostic();
    assert(value.sequence==1 && value.timeMillis==1000 && value.contacts==3);
    assert(FormatWorldDiagnostic(text,sizeof(text),value,1016));
    assert(std::strstr(text,"Solid collider reported") && std::strstr(text,"Body 37.1250 | world 17.2500"));
    assert(std::strstr(text,"Contact 17.2500 | world/unmapped") && std::strstr(text,"Material 6 | group -1 | hits 3"));
    assert(std::strstr(text,"16 ms old | #1"));
    assert(std::strstr(text,"World 317 us | rays 1+1"));
    assert(FormatWorldDiagnostic(text,sizeof(text),value,1000+WorldDiagnosticFreshnessMillis));
    assert(std::strstr(text,"Body 37.1250"));
    assert(FormatWorldDiagnostic(text,sizeof(text),value,1001+WorldDiagnosticFreshnessMillis));
    assert(std::strstr(text,"No fresh world ray") && !std::strstr(text,"Solid collider reported"));
    assert(FormatWorldDiagnostic(text,sizeof(text),value,999));
    assert(std::strstr(text,"clock changed") && !std::strstr(text,"Solid collider reported"));
    value.result=Occlusion::Clear;
    assert(!FormatWorldDiagnostic(text,sizeof(text),value,1016) && !text[0]);
    value.result=Occlusion::Unavailable;value.reason=WorldProbeReason::ContactLimit;
    assert(FormatWorldDiagnostic(text,sizeof(text),value,1016));
    assert(std::strstr(text,"Contact buffer limit reached"));
    assert(!FormatWorldDiagnostic(nullptr,32,value,1016));
    assert(!FormatWorldDiagnostic(text,0,value,1016));
    for(std::size_t capacity=1;capacity<=sizeof(text);++capacity) {
        std::array<char,400> guarded;guarded.fill('#');
        assert(FormatWorldDiagnostic(guarded.data()+1,capacity,value,1016));
        assert(guarded.front()=='#' && guarded[capacity+1]=='#');
        assert(std::memchr(guarded.data()+1,'\0',capacity));
    }
    sample.timeMillis=0;
    const auto before=WorldDiagnosticClock();
    PublishWorldDiagnostic(sample);
    value=GetWorldDiagnostic();
    assert(value.sequence==2 && value.timeMillis>=before && value.timeMillis<=WorldDiagnosticClock());
    assert(FormatWorldDiagnostic(text,sizeof(text),value,WorldDiagnosticClock()));
    std::atomic<bool> start{false},done{false};
    std::thread writer([&] {
        while(!start.load(std::memory_order_acquire)) std::this_thread::yield();
        for(int index=1;index<=20000;++index) {
            WorldDiagnostic next;
            next.targetDistance=static_cast<float>(index);next.worldDistance=next.targetDistance*.5f;
            next.contactDistance=next.worldDistance;next.material=index;next.contacts=static_cast<std::uint32_t>(index);
            next.result=Occlusion::Blocked;next.reason=WorldProbeReason::Solid;next.timeMillis=1000;
            next.elapsedMicros=static_cast<std::uint32_t>(index);next.fastQueries=static_cast<std::uint8_t>(index%2);
            PublishWorldDiagnostic(next);
        }
        done.store(true,std::memory_order_release);
    });
    std::thread reader([&] {
        std::uint64_t sequence=0;
        start.store(true,std::memory_order_release);
        do {
            const auto current=GetWorldDiagnostic();
            assert(current.sequence>=sequence);sequence=current.sequence;
            if(current.sequence<=2) continue;
            assert(current.targetDistance==static_cast<float>(current.material));
            assert(current.worldDistance==current.targetDistance*.5f && current.contactDistance==current.worldDistance);
            assert(current.contacts==static_cast<std::uint32_t>(current.material));
            assert(current.elapsedMicros==current.contacts && current.fastQueries==current.material%2);
        } while(!done.load(std::memory_order_acquire));
    });
    writer.join();reader.join();
    value=GetWorldDiagnostic();
    assert(value.sequence==20002 && value.material==20000);
    std::printf("World diagnostics: %zu-byte coherent snapshot; freshness/clock, bounded formatting and 20,000 concurrent publications passed\n",sizeof(value));
}
