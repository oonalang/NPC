#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace drive_music {
inline constexpr std::size_t trackLimit=256;
inline constexpr std::uint64_t fileLimit=64u*1024u*1024u,cacheLimit=192u*1024u*1024u;
enum class Kind { Folder,File };
struct Link {
    Kind kind=Kind::Folder;
    std::string id,key;
    explicit operator bool() const {return !id.empty();}
};
struct Entry {Link link;std::string title;};
struct Catalog {Link source;std::vector<Entry> tracks;std::string error;bool truncated=false;};
using Canceled=std::function<bool()>;
using Progress=std::function<void(float)>;
bool AudioName(const std::string& name);
bool ValidId(const std::string& id);
Link ParseLink(const std::string& text);
std::string Encode(const std::string& value);
std::string Canonical(const Link& link);
std::string DownloadUrl(const Link& link);
std::string HtmlText(const std::string& text);
Catalog ParseFolder(const Link& source,const std::string& html);
bool AudioBytes(const unsigned char* data,std::size_t size);
bool AllowedHost(const std::string& url);
class Client {
    std::string root_,certificate_;
public:
    bool Configure(const std::string& directory);
    Catalog Read(const Link& link,const Canceled& canceled={});
    std::string Fetch(const Entry& entry,std::string& error,const Canceled& canceled={},const Progress& progress={},const std::string& protectedPath={});
    Catalog Restore() const;
    bool Save(const Catalog& catalog) const;
    bool Ready() const {return !root_.empty();}
    const std::string& Root() const {return root_;}
};
}
