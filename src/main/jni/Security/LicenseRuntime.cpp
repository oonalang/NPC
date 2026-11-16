#include "LicenseRuntime.h"
#include <openssl/rand.h>
#include <condition_variable>
#include <ctime>
#include <thread>

namespace license {
std::uint64_t BootMillis() noexcept {
    timespec now{};
    if (clock_gettime(CLOCK_BOOTTIME,&now) != 0 || now.tv_sec < 0) return 0;
    return std::uint64_t(now.tv_sec)*1000 + std::uint64_t(now.tv_nsec)/1000000;
}
namespace {
std::string Nonce() {
    std::array<unsigned char,32> bytes{};
    if (RAND_bytes(bytes.data(),bytes.size()) != 1) return {};
    constexpr char hex[] = "0123456789abcdef";
    std::string result(64,'0');
    for (std::size_t i=0;i<bytes.size();++i) {result[i*2]=hex[bytes[i]>>4];result[i*2+1]=hex[bytes[i]&15];}
    return result;
}
struct Service {
    Session session;
    std::mutex mutex;
    std::condition_variable wake;
    std::string key,serial,error;
    Transport transport = nullptr;
    std::uint64_t revision = 0;
    bool stopping = false;
    std::thread worker;

    ~Service() {
        {std::lock_guard<std::mutex> lock(mutex);stopping=true;}
        wake.notify_all();if(worker.joinable()) worker.join();
    }
    Request Begin(const std::string& nextKey, const std::string& nextSerial, bool refresh) {
        const auto wall = std::time(nullptr);
        return session.Begin(nextKey,nextSerial,Nonce(),wall>0?std::uint64_t(wall):0,BootMillis(),refresh);
    }
    bool Consume(const Request& request, const Response& response) {
        if(response.failure != Failure::None) {
            session.Reject(request,response.failure==Failure::Transient);
            error=response.error.empty()?"License request failed":response.error;return false;
        }
        const auto wall=std::time(nullptr);
        if(!session.Accept(request,response.payload,response.signature,wall>0?std::uint64_t(wall):0,BootMillis())) {
            error="License response verification failed";return false;
        }
        error.clear();return true;
    }
    void Run() {
        std::unique_lock<std::mutex> lock(mutex);
        while(!stopping) {
            wake.wait(lock,[&]{return stopping||(!key.empty()&&transport);});
            if(stopping) break;
            const auto expected=revision;
            auto delay=refreshSeconds;
            while(!stopping && revision==expected && !key.empty()) {
                if(wake.wait_for(lock,std::chrono::seconds(delay),[&]{return stopping||revision!=expected;})) break;
                const auto request=Begin(key,serial,true);
                const auto fetch=transport;
                lock.unlock();
                const auto response=request.generation?fetch(request):Response{Failure::Rejected,{}, {},"Unable to create secure request"};
                lock.lock();
                if(stopping||revision!=expected) break;
                const bool accepted=Consume(request,response);
                delay=accepted?refreshSeconds:retrySeconds;
                if(!accepted&&response.failure!=Failure::Transient) {key.clear();serial.clear();}
            }
        }
    }
};
Service& Engine() {static Service service;return service;}
}
bool Allows(Capability capability) noexcept {return Engine().session.Allows(capability,BootMillis());}
std::uint64_t LicenseUntil() noexcept {return Engine().session.LicenseUntil();}
std::string LastError() {auto& service=Engine();std::lock_guard<std::mutex> lock(service.mutex);return service.error;}
std::string Authenticate(const std::string& key, const std::string& serial, Transport transport) {
    auto& service=Engine();
    std::unique_lock<std::mutex> lock(service.mutex);
    service.key.clear();service.serial.clear();++service.revision;service.wake.notify_all();
    const auto request=service.Begin(key,serial,false);
    const auto expected=service.revision;
    lock.unlock();
    const auto response=request.generation&&transport?transport(request):Response{Failure::Rejected,{}, {},"Unable to create secure request"};
    lock.lock();
    if(expected!=service.revision) return "Authentication superseded";
    if(!service.Consume(request,response)) return service.error;
    service.key=key;service.serial=serial;service.transport=transport;
    if(!service.worker.joinable()) service.worker=std::thread([&service]{service.Run();});
    service.wake.notify_all();
    return "OK";
}
}
