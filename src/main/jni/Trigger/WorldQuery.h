#pragma once
#include "TriggerGlass.h"

namespace PhantomTrigger {
struct WorldQueryPreference {
    ContextIdentity context{};
    bool contacts=false;
    bool& For(const ContextIdentity& next) noexcept {
        if(context!=next) {context=next;contacts=false;}
        return contacts;
    }
};

template<class Fast,class Trace,class Classify>
Occlusion QueryFreshWorld(float distance,bool& preferContacts,Fast fast,Trace trace,Classify classify) noexcept {
    if(!std::isfinite(distance) || distance<=0.f) return Occlusion::Unavailable;
    if(preferContacts) {
        RayContacts contacts{};
        std::size_t count=0;
        if(trace(contacts,count)) {
            if(count>contacts.size()) {preferContacts=false;return Occlusion::Unavailable;}
            preferContacts=count!=0;
            if(!count) return Occlusion::Clear;
            float nearest=contacts[0].distance;
            for(std::size_t index=1;index<count;++index) nearest=std::min(nearest,contacts[index].distance);
            return CheckWorldContacts(contacts,count,{0,nearest},distance,classify);
        }
        preferContacts=false;
        float blockerDistance=0.f;
        const bool blocked=fast(blockerDistance);
        return ResolveWorldOcclusion(distance,blocked,blockerDistance,
            [](RayContacts&,std::size_t&) noexcept {return false;},classify);
    }
    float blockerDistance=0.f;
    const bool blocked=fast(blockerDistance);
    preferContacts=blocked && std::isfinite(blockerDistance) && blockerDistance>=0.f &&
        blockerDistance+ContactTolerance(distance)<distance;
    return ResolveWorldOcclusion(distance,blocked,blockerDistance,trace,classify);
}
}
