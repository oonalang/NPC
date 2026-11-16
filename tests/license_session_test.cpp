#include "lease_test_support.h"
#include <cstdio>
#include <thread>

using namespace license;
namespace {
Session* active=nullptr;
std::uint64_t tick=100000;
int originals=0,handlers=0;
int Original(int value,int& seen){++originals;seen=value;return value;}
int Handler(int value,int& seen){++handlers;seen=value*2;return value*2;}
int (*originalSlot)(int,int&)=Original;
void (*originalVoid)(int&)=nullptr;
void VoidHandler(int& value){++value;}
Request Begin(Session& session,char nonce='a',bool refresh=false,std::uint64_t at=100000) {
    return session.Begin("test-key","device-12345678",std::string(64,nonce),1900000000,at,refresh);
}
std::string Change(std::string payload,std::size_t field,const std::string& value) {
    std::size_t begin=0;
    for(std::size_t i=0;i<field;++i) {begin=payload.find('|',begin)+1;assert(begin);}
    const auto end=payload.find('|',begin);
    payload.replace(begin,end==std::string::npos?end:end-begin,value);return payload;
}
void ClaimsAndSignatures(const LeaseSigner& signer) {
    Session session(signer.publicKey.c_str(),signer.keyId.c_str());
    const auto request=Begin(session);
    const auto payload=signer.Payload(request);const auto signature=signer.Sign(payload);
    Claims claims;
    const auto verify=[&](const std::string& text,const std::string& sig,std::uint64_t wall=1900000000,std::uint64_t now=100001) {
        return Verify(request,text,sig,wall,now,claims,signer.publicKey.c_str(),signer.keyId.c_str());
    };
    assert(verify(payload,signature)&&claims.capabilities==3&&claims.until==1900000180);
    for(const auto& mutation:std::initializer_list<std::pair<std::size_t,std::string>>{
        {0,"v1"},{1,"OTHER"},{2,"2"},{3,"other-key"},{4,"other-device"},{5,std::string(64,'b')},
        {6,"1900000031"},{6,"01899999999"},{6,"18446744073709551616"},
        {7,"1900000000"},{7,"1900000181"},{8,"1900000179"},{8,"18446744073709551615"},{9,"0"},{9,"4"},{9,"03"},{10,std::string(64,'b')}}) {
        const auto changed=Change(payload,mutation.first,mutation.second);
        assert(!verify(changed,signer.Sign(changed)));
    }
    assert(!verify(payload+"|extra",signer.Sign(payload+"|extra")));
    auto corrupt=signature;corrupt[10]=corrupt[10]=='A'?'B':'A';assert(!verify(payload,corrupt));
    assert(!verify(payload,signature+'='));assert(!verify(payload,""));
    LeaseSigner other;assert(!verify(payload,other.Sign(payload)));
    assert(!verify(payload,signature,1900000180));
    assert(!verify(payload,signature,1900000000,130001));
    assert(!verify(payload,signature,1900000000,99999));
    assert(!verify(payload,signature,0));
    assert(!verify(payload,signature,UINT64_MAX));
    auto wrongNonce=request;wrongNonce.nonce=std::string(64,'b');
    assert(!Verify(wrongNonce,payload,signature,1900000000,100001,claims,signer.publicKey.c_str(),signer.keyId.c_str()));
}
void GateAndRevocation(const LeaseSigner& signer) {
    Session session(signer.publicKey.c_str(),signer.keyId.c_str());active=&session;
    using Hook=Guard<Handler,&originalSlot>;
    bool patchedLoginFlag=true;
    int seen=0;
    assert(patchedLoginFlag&&!session.Allows(Capability::Menu,tick));
    assert(Hook::Invoke(7,seen)==7&&seen==7&&originals==1&&handlers==0);
    Guard<VoidHandler,&originalVoid>::Invoke(seen);assert(seen==7);
    const auto request=Begin(session);const auto payload=signer.Payload(request);
    assert(session.Accept(request,payload,signer.Sign(payload),request.wall,tick));
    assert(Hook::Invoke(8,seen)==16&&seen==16&&handlers==1);
    originalSlot=nullptr;assert(Hook::Invoke(9,seen)==0&&handlers==1);originalSlot=Original;
    assert(!session.Accept(request,payload,signer.Sign(payload),request.wall,tick));
    assert(session.Allows(Capability::Features,tick));
    const auto transient=Begin(session,'b',true,110000);session.Reject(transient,true);
    assert(session.Allows(Capability::Features,279999));
    assert(!session.Allows(Capability::Features,280000));
    tick=280000;assert(Hook::Invoke(9,seen)==9&&handlers==1);
    tick=120000;
    const auto denied=Begin(session,'c',true,120000);session.Reject(denied);
    assert(!session.Allows(Capability::Features,tick));
    const auto menu=Begin(session,'d');const auto menuPayload=signer.Payload(menu,1);
    assert(session.Accept(menu,menuPayload,signer.Sign(menuPayload),menu.wall,tick));
    assert(session.Allows(Capability::Menu,tick)&&!session.Allows(Capability::Features,tick));
    assert(Hook::Invoke(10,seen)==10&&handlers==1);
    active=nullptr;tick=100000;
}
void StaleResponses(const LeaseSigner& signer) {
    Session session(signer.publicKey.c_str(),signer.keyId.c_str());
    const auto first=Begin(session,'a');const auto old=signer.Payload(first);
    const auto second=Begin(session,'b');const auto current=signer.Payload(second);
    assert(session.Accept(second,current,signer.Sign(current),second.wall,100001));
    session.Reject(first);assert(session.Allows(Capability::Features,100001));
    assert(!session.Accept(first,old,signer.Sign(old),first.wall,100001));
    assert(session.Allows(Capability::Features,100001));
    const auto replay=Begin(session,'c',true);
    assert(!session.Accept(replay,current,signer.Sign(current),replay.wall,100001));
    assert(!session.Allows(Capability::Menu,100001));
    const auto base=Begin(session,'d');const auto accepted=signer.Payload(base);
    assert(session.Accept(base,accepted,signer.Sign(accepted),base.wall,100001));
    for(int i=0;i<100;++i) {const auto next=Begin(session,'e',true,100010+i);session.Reject(next,true);}
    assert(!session.Allows(Capability::Features,280000));
    assert(!session.Allows(Capability::Features,0));
    assert(!session.Allows(static_cast<Capability>(4),100001));
    assert(!session.Begin("", "device-12345678",std::string(64,'a'),1900000000,100000).generation);
    assert(!session.Allows(Capability::Features,100001));
}
void ConcurrentReads(const LeaseSigner& signer) {
    Session session(signer.publicKey.c_str(),signer.keyId.c_str());
    std::atomic<bool> stop{false};std::atomic<unsigned> reads{0};
    std::thread reader([&]{while(!stop.load()){session.Allows(Capability::Features,100001);++reads;}});
    for(int i=0;i<20;++i) {
        const auto request=Begin(session,char('a'+i%6));const auto payload=signer.Payload(request);
        assert(session.Accept(request,payload,signer.Sign(payload),request.wall,100001));
    }
    stop=true;reader.join();assert(reads.load()>0);
}
}
namespace license {bool Allows(Capability capability) noexcept {return active&&active->Allows(capability,tick);}}
int main() {
    LeaseSigner signer;
    ClaimsAndSignatures(signer);GateAndRevocation(signer);StaleResponses(signer);ConcurrentReads(signer);
    std::puts("License sessions: forged/misbound/replayed/legacy grants rejected; menu-flag bypass, expiry, revocation, stale workers and native guard forwarding covered");
}
