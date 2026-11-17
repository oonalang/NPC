#pragma once
#include "SaveConfig.h"
#include "AndroidConfigExchange.h"
#include <dirent.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace device_profiles {

struct Entry { std::string name; };

inline std::string NormalizeName(const std::string& text) {
    std::string out;
    for (unsigned char ch : text) {
        if (out.size() >= 40) break;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_') out += static_cast<char>(ch);
        else if (ch == ' ' && !out.empty() && out.back() != '_') out += '_';
    }
    return out;
}

inline std::string Directory() {
    const std::string base = GetFilesDirPath(VM);
    return base.empty() ? std::string() : base + "/shai_profiles";
}

inline bool EnsureDirectory() {
    const std::string path=Directory();
    return !path.empty() && (mkdir(path.c_str(),0700)==0 || errno==EEXIST);
}

inline std::string File(const std::string& name) {
    if (name.empty() || name != NormalizeName(name)) return {};
    const std::string root=Directory();
    return root.empty() ? std::string() : root + "/" + name + ".json";
}

inline bool Read(const std::string& path, std::string& output) {
    if (path.empty()) return false;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    const auto size=file.tellg();
    if (size<=0 || size>131072) return false;
    file.seekg(0);
    output.resize(static_cast<size_t>(size));
    file.read(&output[0],static_cast<std::streamsize>(output.size()));
    return file.good() || file.gcount()==static_cast<std::streamsize>(output.size());
}

inline bool Write(const std::string& path, const std::string& text) {
    if (path.empty() || text.empty() || text.size()>131072 || !EnsureDirectory()) return false;
    const std::string temp=path+".tmp";
    {
        std::ofstream file(temp,std::ios::binary|std::ios::trunc);
        if (!file) return false;
        file.write(text.data(),static_cast<std::streamsize>(text.size()));
        file.flush();
        if (!file.good()) {file.close();std::remove(temp.c_str());return false;}
    }
    if (std::rename(temp.c_str(),path.c_str())!=0) {std::remove(temp.c_str());return false;}
    return true;
}

inline bool MergeAllowed(nlohmann::json& target, const nlohmann::json& incoming, int depth=0) {
    if (!target.is_object() || !incoming.is_object() || depth>3) return false;
    bool any=false;
    for (auto it=incoming.begin();it!=incoming.end();++it) {
        if (!target.contains(it.key())) continue;
        auto& slot=target[it.key()];
        const auto& value=it.value();
        if (slot.is_object()) {
            if (!value.is_object()) return false;
            if (MergeAllowed(slot,value,depth+1)) any=true;
        } else if (slot.is_boolean()) {
            if (!value.is_boolean()) return false;
            slot=value;any=true;
        } else if (slot.is_number()) {
            if (!value.is_number()) return false;
            const double number=value.get<double>();
            if (!std::isfinite(number) || number < -10000.0 || number > 10000.0) return false;
            if (slot.is_number_integer() && (!value.is_number_integer() || number < 0 || number > 1000)) return false;
            slot=value;any=true;
        } else if (slot.is_array()) {
            if (!value.is_array() || value.size()!=slot.size() || value.size()>4) return false;
            for (size_t i=0;i<value.size();++i) {
                if (!value[i].is_number()) return false;
                const double number=value[i].get<double>();
                if (!std::isfinite(number) || number<0.0 || number>1.0) return false;
            }
            slot=value;any=true;
        }
    }
    return any;
}

inline bool Decode(const std::string& text, nlohmann::json& settings, std::string* profileName=nullptr) {
    if (text.empty() || text.size()>131072) return false;
    try {
    const auto document=nlohmann::json::parse(text,nullptr,false);
    if (document.is_discarded() || !document.is_object()) return false;
    const nlohmann::json* input=&document;
    if (document.contains("format")) {
        if (document.value("format",std::string())!="shai-ethnir-config" ||
            document.value("schema",0)!=1 || !document.contains("settings") ||
            !document["settings"].is_object()) return false;
        input=&document["settings"];
    }
    settings=CaptureConfiguration();
    if (!MergeAllowed(settings,*input)) return false;
    if (profileName && document.contains("name") && document["name"].is_string())
        *profileName=NormalizeName(document["name"].get<std::string>());
    return true;
    } catch (const std::exception&) { return false; }
}

inline nlohmann::json Package(const std::string& name, const nlohmann::json& settings) {
    return nlohmann::json{{"format","shai-ethnir-config"},{"schema",1},{"name",name},{"settings",settings}};
}

inline bool SaveLocal(const std::string& text) {
    const std::string name=NormalizeName(text);
    if (name.empty()) return false;
    return Write(File(name),Package(name,CaptureConfiguration()).dump(2));
}

inline std::vector<Entry> List() {
    std::vector<Entry> result;
    const std::string root=Directory();
    if (root.empty()) return result;
    DIR* dir=opendir(root.c_str());
    if (!dir) return result;
    while (dirent* entry=readdir(dir)) {
        std::string name=entry->d_name;
        if (name.size()<6 || name.substr(name.size()-5)!=".json") continue;
        name.erase(name.size()-5);
        if (!name.empty() && name==NormalizeName(name)) result.push_back({name});
        if (result.size()>=60) break;
    }
    closedir(dir);
    std::sort(result.begin(),result.end(),[](const Entry& a,const Entry& b){return a.name<b.name;});
    return result;
}

inline bool LoadLocal(const std::string& name) {
    std::string data; nlohmann::json config;
    return Read(File(name),data) && Decode(data,config) && ApplyConfiguration(config);
}

inline bool DeleteLocal(const std::string& name) {
    const std::string path=File(name);
    return !path.empty() && std::remove(path.c_str())==0;
}

inline bool ExportLocal(const std::string& name) {
    std::string data; nlohmann::json settings;
    if (!Read(File(name),data) || !Decode(data,settings)) return false;
    const std::string portable=Package(name,settings).dump(2);
    return android_config_exchange::DownloadJson("SHAI-"+name+".json",portable);
}

inline bool ImportJson(const std::string& text, std::string& importedName) {
    nlohmann::json settings;
    if (!Decode(text,settings,&importedName)) return false;
    if (importedName.empty()) importedName="Imported";
    const std::string base=importedName;
    const auto existing=List();
    if (existing.size()>=60) return false;
    const auto exists=[&existing](const std::string& name) {
        return std::any_of(existing.begin(),existing.end(),[&name](const Entry& entry) { return entry.name==name; });
    };
    if (exists(importedName)) {
        bool found=false;
        for (unsigned index=2;index<=61;++index) {
            const std::string suffix="_"+std::to_string(index);
            const std::string candidate=base.substr(0,40-suffix.size())+suffix;
            if (!exists(candidate)) {importedName=candidate;found=true;break;}
        }
        if (!found) return false;
    }
    return Write(File(importedName),Package(importedName,settings).dump(2));
}

inline bool ImportClipboard(std::string& importedName) {
    std::string data;
    return android_config_exchange::ReadClipboard(data) && ImportJson(data,importedName);
}

}
