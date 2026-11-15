#include "../src/main/jni/System/Hooks/SessionProtection.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace session_protection;
static constexpr Identity first{1,2,1},second{1,2,2};
static constexpr Preference off{false,0},on{true,1},released{false,2};

static void NoImplicitRequests() {
    Controller value;
    assert(!value.Step(first,true,off,0).valid&&value.Status()==State::Off);
    assert(!value.Step(first,false,on,0).valid&&value.Status()==State::WaitingForLogin);
    assert(!value.Step({},true,on,0).valid);
    auto request=value.Step(first,true,on,1);assert(request.valid&&request.value&&!value.HasBaseline());
    value.Complete(request,Outcome::Queued,1);
    assert(value.Status()==State::Requested&&value.NeedsAttention());
    for(std::uint64_t now:{2,1000,100000}) assert(!value.Step(first,true,on,now).valid);
    request=value.Step(first,true,released,100001);assert(request.valid&&!request.value);
    value.Complete(request,Outcome::Queued,100001);
    assert(!value.NeedsAttention()&&value.Status()==State::Restored);
    assert(!value.Step(first,true,released,100002).valid&&value.Status()==State::Off);
}
static void StockBehavior() {
    Controller value;value.ObserveGameRequest(true);
    auto request=value.Step(first,true,on,0);assert(request.valid&&request.value);
    value.Complete(request,Outcome::Queued,0);
    assert(!value.NeedsAttention());
    assert(!value.Step(first,true,released,1).valid&&value.Status()==State::Off);
    Controller changing;changing.ObserveGameRequest(false);
    request=changing.Step(first,true,on,0);changing.Complete(request,Outcome::Queued,0);
    changing.ObserveGameRequest(true);
    changing.NaturalResult(on,true,Outcome::Queued,1);
    assert(!changing.Step(first,true,released,2).valid);
    Controller natural;natural.ObserveGameRequest(false);
    request=natural.Step(first,true,on,0);
    natural.ObserveGameRequest(false);
    natural.NaturalResult(on,true,Outcome::Queued,1);
    natural.Complete(request,Outcome::Rejected,2);
    assert(natural.Status()==State::Requested&&!natural.Step(first,true,on,10).valid);
    natural.ObserveGameRequest(false);
    natural.NaturalResult(released,false,Outcome::Queued,11);
    assert(!natural.NeedsAttention()&&!natural.Step(first,true,released,12).valid);
}
static void BoundedFailures() {
    Controller value;value.ObserveGameRequest(false);
    auto request=value.Step(first,true,on,0);assert(request.valid);
    value.Complete(request,Outcome::Rejected,0);
    assert(!value.Step(first,true,on,999).valid);
    request=value.Step(first,true,on,1000);assert(request.valid);
    value.Complete(request,Outcome::Rejected,1000);
    assert(!value.Step(first,true,on,3999).valid);
    request=value.Step(first,true,on,4000);assert(request.valid);
    value.Complete(request,Outcome::Rejected,4000);
    assert(value.Status()==State::Failed);
    for(auto now:{4001u,10000u,1000000u}) assert(!value.Step(first,true,on,now).valid);
    assert(!value.Step(first,true,released,1000001).valid);
    request=value.Step(first,true,{true,3},1000002);assert(request.valid);
    value.Complete(request,Outcome::Queued,1000002);assert(value.Status()==State::Requested);
    Controller uncertain;uncertain.ObserveGameRequest(false);
    request=uncertain.Step(first,true,on,0);uncertain.Complete(request,Outcome::Unconfirmed,0);
    assert(uncertain.Status()==State::Unconfirmed&&uncertain.NeedsAttention());
    assert(!uncertain.Step(first,true,on,100000).valid);
    request=uncertain.Step(first,true,released,100001);assert(request.valid&&!request.value);
    uncertain.Complete(request,Outcome::Unconfirmed,100001);
    assert(uncertain.Status()==State::Unconfirmed&&uncertain.NeedsAttention());
    assert(!uncertain.Step(first,true,released,100002).valid);
}
static void SessionBoundaries() {
    Controller value;value.ObserveGameRequest(false);
    const auto old=value.Step(first,true,on,0);assert(old.valid);
    value.Disconnected(true);
    value.Complete(old,Outcome::Queued,1);
    assert(value.Status()==State::WaitingForLogin&&!value.Active());
    const auto current=value.Step(second,true,on,2);assert(current.valid&&current.value);
    value.Complete(old,Outcome::Rejected,3);
    value.Complete(current,Outcome::Queued,3);
    assert(value.Status()==State::Requested);
    assert(!value.Step(second,false,on,4).valid&&!value.Active());
    auto request=value.Step({8,9,3},true,released,5);assert(!request.valid);
    request=value.Step({8,9,3},true,{true,3},6);assert(request.valid);
    value.Complete(request,Outcome::Queued,6);
    value.ObserveGameRequest(true);value.NaturalResult({true,3},true,Outcome::Queued,7);
    assert(!value.Step({8,9,3},true,{false,4},8).valid);
    value.Disconnected(true);
    request=value.Step({8,9,4},true,{true,5},9);assert(request.valid&&request.value&&!value.HasBaseline());
    value.Complete(request,Outcome::Queued,9);
    request=value.Step({8,9,4},true,{false,6},10);assert(request.valid&&!request.value);
}
static void PreferenceAndLabels() {
    preference=0;
    std::thread writer([] {for(int i=0;i<10000;++i) {Publish(true);Publish(false);}});
    std::uint64_t previous=0;
    for(int i=0;i<20000;++i) {
        const auto setting=Read();assert(setting.revision>=previous);
        assert(setting.enabled==bool(setting.revision&1));previous=setting.revision;
    }
    writer.join();Publish(false);const auto before=Read();Publish(false);assert(Read().revision==before.revision);
    backend=Backend::Ready;
    for(auto state:{State::Off,State::WaitingForLogin,State::Pending,State::Requested,State::Restored,State::Unconfirmed,State::Failed}) {
        reported=state;assert(StatusText()&&std::strlen(StatusText())>0);
        assert(std::strstr(StatusText(),"Protected")==nullptr);
    }
    reported=State::Requested;assert(std::strstr(StatusText(),"server decides"));
}
int main() {
    NoImplicitRequests();StockBehavior();BoundedFailures();SessionBoundaries();PreferenceAndLabels();
    std::puts("Session protection: opt-in requests, stock restoration, bounded retries, unknown outcomes, reconnects, stale work and atomic preferences passed");
}
