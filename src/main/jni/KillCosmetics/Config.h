#pragma once
#include "State.h"
namespace kill_cosmetics {
template<class Json> inline void Save(Json& json) {
    for(std::size_t i=0;i<categoryCount;++i)
        json["KillCosmetics"][labels[i]]=Valid(static_cast<Kind>(i),configured.ids[i])?configured.ids[i]:0;
}
template<class Json> inline void Load(const Json& json) {
    configured={};
    const auto section=json.find("KillCosmetics");
    if(section!=json.end() && section->is_object()) for(std::size_t i=0;i<categoryCount;++i) {
        const auto value=section->find(labels[i]);
        if(value==section->end() || !value->is_number_integer()) continue;
        if(value->is_number_unsigned()) {
            const auto number=value->template get<std::uint64_t>();
            if(number<=UINT32_MAX && Valid(static_cast<Kind>(i),static_cast<std::uint32_t>(number))) configured.ids[i]=static_cast<std::uint32_t>(number);
        } else {
            const auto number=value->template get<std::int64_t>();
            if(number>0 && number<=UINT32_MAX && Valid(static_cast<Kind>(i),static_cast<std::uint32_t>(number))) configured.ids[i]=static_cast<std::uint32_t>(number);
        }
    }
    Publish(configured);
}
}
