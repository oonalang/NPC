#pragma once
#include "State.h"
namespace kill_cosmetics {
template<class Json> inline void Save(Json& json) {json["KillCosmetics"]={{"Automatic",configured.automatic}};}
template<class Json> inline void Load(const Json& json) {
    configured={};
    const auto section=json.find("KillCosmetics");
    if(section!=json.end() && section->is_object()) {
        const auto automatic=section->find("Automatic");
        if(automatic!=section->end() && automatic->is_boolean())configured.automatic=automatic->template get<bool>();
    }
    Publish(configured);
}
}
