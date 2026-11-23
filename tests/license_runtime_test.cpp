#include "lease_test_support.h"
#include "../src/main/jni/Security/LicenseRuntime.h"
#include <condition_variable>
#include <cstdio>
#include <thread>

LeaseSigner signer;
std::mutex mutex;
std::condition_variable wake;
bool blocked=false,released=false;
license::Response Forged(const license::Request& request) {
    auto payload=signer.Payload(request);
    payload.replace(payload.rfind('|')+1,std::string::npos,PanelLicense::kKeyId);
    return {license::Failure::None,payload,signer.Sign(payload),"OK"};
}
license::Response Rejected(const license::Request&) {return {license::Failure::Rejected,{}, {},"Rejected"};}
license::Response Transient(const license::Request&) {return {license::Failure::Transient,{}, {},"Unavailable"};}
license::Response Stale(const license::Request& request) {
    std::unique_lock<std::mutex> lock(mutex);blocked=true;wake.notify_all();wake.wait(lock,[]{return released;});
    return Forged(request);
}
license::Response Osn(const license::Request& request) {
    license::Response response;
    response.failure=license::Failure::None;
    response.osn=true;
    response.osnToken="302637905aaaf3166cbabe99db50bd9d";
    response.osnIssued=request.wall;
    response.osnLicenseUntil=request.wall+3600;
    return response;
}
int main() {
    using namespace license;
    isLogin=false;
    assert(!Allows(Capability::Menu)&&!Allows(Capability::Features));
    assert(Authenticate("key","device-12345678",Forged)!="OK");
    assert(!Allows(Capability::Menu)&&!Allows(Capability::Features));
    assert(Authenticate("key","device-12345678",Transient)!="OK"&&!Allows(Capability::Menu));
    assert(Authenticate("key","device-12345678",Rejected)!="OK"&&!Allows(Capability::Menu));
    assert(Authenticate("key","device-12345678",nullptr)!="OK"&&!Allows(Capability::Menu));
    std::string result;
    std::thread worker([&]{result=Authenticate("old-key","device-12345678",Stale);});
    {std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[]{return blocked;});}
    assert(Authenticate("new-key","device-12345678",Rejected)!="OK");
    {std::lock_guard<std::mutex> lock(mutex);released=true;}wake.notify_all();worker.join();
    assert(result=="Authentication superseded"&&!Allows(Capability::Menu)&&!Allows(Capability::Features));
    assert(Authenticate("key","device-12345678",Osn)=="OK");
    assert(Allows(Capability::Menu)&&Allows(Capability::Features)&&Allows(Capability::Premium));
    assert(LicenseUntil()>0);
    std::puts("Production license runtime: forged, missing, transient and superseded responses rejected; verified OSN login accepted");
}
