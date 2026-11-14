#include "DriveMusic.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__ANDROID__) || defined(DRIVE_HTTP_TEST)
#include <curl/curl.h>
#endif

namespace drive_music {
#ifdef __ANDROID__
extern "C" const unsigned char drive_ca_start[],drive_ca_end[];
#endif
namespace {
std::string Lower(std::string value) {for(char& c:value) if(c>='A'&&c<='Z') c+=32;return value;}
int Hex(char c) {return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
std::string Decode(const std::string& value) {
    std::string result;
    for(std::size_t i=0;i<value.size();++i) {
        if(value[i]=='%'&&i+2<value.size()&&Hex(value[i+1])>=0&&Hex(value[i+2])>=0) {
            result+=static_cast<char>(Hex(value[i+1])*16+Hex(value[i+2]));i+=2;
        } else result+=value[i];
    }
    return result;
}
std::string Parameter(const std::string& value,const std::string& key) {
    auto at=value.find('?');
    while(at!=std::string::npos && ++at<value.size()) {
        const auto end=value.find_first_of("&#",at),equal=value.find('=',at);
        if(equal!=std::string::npos && (end==std::string::npos||equal<end) && value.substr(at,equal-at)==key)
            return Decode(value.substr(equal+1,(end==std::string::npos?value.size():end)-equal-1));
        if(end==std::string::npos||value[end]=='#') break;
        at=end;
    }
    return {};
}
std::string Attribute(const std::string& tag,const std::string& name) {
    auto at=tag.find(name+"=");if(at==std::string::npos) return {};
    at+=name.size()+1;if(at>=tag.size()||(tag[at]!='\''&&tag[at]!='"')) return {};
    const auto end=tag.find(tag[at],at+1);return end==std::string::npos?std::string{}:HtmlText(tag.substr(at+1,end-at-1));
}
bool Write(const std::string& path,const std::string& data) {
    const auto temporary=path+".tmp";
    auto* file=std::fopen(temporary.c_str(),"wb");if(!file) return false;
    const bool written=std::fwrite(data.data(),1,data.size(),file)==data.size();
    const bool closed=std::fclose(file)==0;
    if(written&&closed&&std::rename(temporary.c_str(),path.c_str())==0) {chmod(path.c_str(),0600);return true;}
    std::remove(temporary.c_str());return false;
}
std::string ReadText(const std::string& path,std::size_t maximum) {
    struct stat info{};if(stat(path.c_str(),&info)!=0||info.st_size<0||static_cast<std::uint64_t>(info.st_size)>maximum) return {};
    std::ifstream input(path,std::ios::binary);return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
bool CachedAudio(const std::string& path) {
    struct stat info{};
    if(stat(path.c_str(),&info)!=0||!S_ISREG(info.st_mode)||info.st_size<12||static_cast<std::uint64_t>(info.st_size)>fileLimit) return false;
    std::ifstream input(path,std::ios::binary);unsigned char header[32]{};input.read(reinterpret_cast<char*>(header),sizeof(header));
    return AudioBytes(header,static_cast<std::size_t>(input.gcount()));
}
void Clean(const std::string& root,const std::string& keep,const std::string& playing) {
    struct File {std::string path;std::uint64_t bytes;time_t modified;};
    std::vector<File> files;std::uint64_t total=0;
    auto* dir=opendir(root.c_str());if(!dir) return;
    while(auto* entry=readdir(dir)) {
        const std::string name=entry->d_name;
        if(name.size()<12||name.compare(0,5,"song-")!=0||name.substr(name.size()-6)!=".audio") continue;
        const auto path=root+"/"+name;struct stat info{};
        if(stat(path.c_str(),&info)!=0||!S_ISREG(info.st_mode)||info.st_size<0) continue;
        files.push_back({path,static_cast<std::uint64_t>(info.st_size),info.st_mtime});total+=info.st_size;
    }
    closedir(dir);std::sort(files.begin(),files.end(),[](const File& a,const File& b){return a.modified<b.modified;});
    for(const auto& file:files) if(total>cacheLimit&&file.path!=keep&&file.path!=playing&&std::remove(file.path.c_str())==0) total-=file.bytes;
}
struct Response {long status=0;std::string body,type,location,error;bool canceled=false;};
#if defined(__ANDROID__) || defined(DRIVE_HTTP_TEST)
struct Transfer {
    Response response;FILE* file=nullptr;std::size_t received=0,maximum=0;
    const Canceled* canceled=nullptr;const Progress* progress=nullptr;
};
size_t Body(char* bytes,size_t size,size_t count,void* context) {
    auto& transfer=*static_cast<Transfer*>(context);
    if(size&&count>static_cast<size_t>(-1)/size) return 0;
    const auto length=size*count;
    if(length>transfer.maximum-transfer.received) {transfer.response.error=transfer.maximum==fileLimit?"This Drive file is too large (64 MiB maximum).":"This folder listing is too large. Try a smaller music folder.";return 0;}
    if(transfer.canceled&&*transfer.canceled&&(*transfer.canceled)()) {transfer.response.canceled=true;return 0;}
    if(transfer.file) {if(std::fwrite(bytes,1,length,transfer.file)!=length) {transfer.response.error="The music cache is full or unavailable.";return 0;}}
    else transfer.response.body.append(bytes,length);
    transfer.received+=length;return length;
}
size_t Header(char* bytes,size_t size,size_t count,void* context) {
    auto& response=static_cast<Transfer*>(context)->response;
    const std::string line(bytes,size*count);const auto colon=line.find(':');
    if(colon!=std::string::npos) {
        const auto key=Lower(line.substr(0,colon));auto value=line.substr(colon+1);
        while(!value.empty()&&std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
        while(!value.empty()&&std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
        if(key=="location") response.location=value;
    }
    return size*count;
}
int Tick(void* context,curl_off_t total,curl_off_t current,curl_off_t,curl_off_t) {
    auto& transfer=*static_cast<Transfer*>(context);
    if(transfer.canceled&&*transfer.canceled&&(*transfer.canceled)()) {transfer.response.canceled=true;return 1;}
    if(transfer.progress&&*transfer.progress&&total>0) (*transfer.progress)(std::clamp(float(current)/float(total),0.f,1.f));
    return 0;
}
Response Get(std::string url,const std::string& certificate,std::size_t maximum,FILE* file,const Canceled& canceled,const Progress& progress={}) {
    static const bool initialized=curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK;
    if(!initialized) return {0,{},{},{},"Couldn't start the Drive connection."};
    for(int redirect=0;redirect<=5;++redirect) {
        if(!AllowedHost(url)) return {0,{},{},{},"Drive requested an unsupported address. Use a public sharing link."};
        if(canceled&&canceled()) {Response response;response.canceled=true;return response;}
        auto* curl=curl_easy_init();if(!curl) return {0,{},{},{},"Couldn't start the Drive connection."};
        Transfer transfer;transfer.file=file;transfer.maximum=maximum;transfer.canceled=&canceled;transfer.progress=&progress;
        char error[CURL_ERROR_SIZE]{};
        curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
        curl_easy_setopt(curl,CURLOPT_PROTOCOLS,long(CURLPROTO_HTTPS));
        curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
        if(!certificate.empty()) curl_easy_setopt(curl,CURLOPT_CAINFO,certificate.c_str());
        curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
        curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT_MS,8000L);curl_easy_setopt(curl,CURLOPT_TIMEOUT,45L);
        curl_easy_setopt(curl,CURLOPT_LOW_SPEED_LIMIT,1024L);curl_easy_setopt(curl,CURLOPT_LOW_SPEED_TIME,15L);
        curl_easy_setopt(curl,CURLOPT_USERAGENT,"SHAI-ETHNIR-Drive-Music/1.0");
        curl_easy_setopt(curl,CURLOPT_ACCEPT_ENCODING,"");
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,Body);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&transfer);
        curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,Header);curl_easy_setopt(curl,CURLOPT_HEADERDATA,&transfer);
        curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,Tick);curl_easy_setopt(curl,CURLOPT_XFERINFODATA,&transfer);
        curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,error);
        const auto result=curl_easy_perform(curl);char* type=nullptr;
        curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&transfer.response.status);curl_easy_getinfo(curl,CURLINFO_CONTENT_TYPE,&type);
        if(type) transfer.response.type=Lower(type);
        curl_easy_cleanup(curl);
        if(transfer.response.status>=300&&transfer.response.status<400&&!transfer.response.location.empty()) {
            if(file) {std::fflush(file);if(ftruncate(fileno(file),0)!=0) return {0,{},{},{},"Couldn't reset the music download."};std::rewind(file);}
            url=transfer.response.location;if(url.compare(0,1,"/")==0) url="https://drive.google.com"+url;
            continue;
        }
        if(result!=CURLE_OK&&transfer.response.error.empty()&&!transfer.response.canceled)
            transfer.response.error="Drive download failed. Check your connection and try again.";
        if(transfer.response.status!=200&&transfer.response.error.empty()&&!transfer.response.canceled)
            transfer.response.error=transfer.response.status==429?"Drive is busy. Try again later.":"Drive couldn't open this item. Set sharing to Anyone with the link / Viewer.";
        return transfer.response;
    }
    return {0,{},{},{},"Drive redirected too many times. Check the sharing link."};
}
#else
Response Get(std::string,const std::string&,std::size_t,FILE*,const Canceled&,const Progress& = {}) {
    return {0,{},{},{},"Drive connections are available in the Android app."};
}
#endif
}
bool ValidId(const std::string& id) {
    return id.size()>=10&&id.size()<=128&&std::all_of(id.begin(),id.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-';});
}
bool AudioName(const std::string& name) {
    const auto at=name.find_last_of('.');if(at==std::string::npos||at==0) return false;
    const auto ext=Lower(name.substr(at+1));return ext=="mp3"||ext=="m4a"||ext=="aac"||ext=="ogg"||ext=="wav"||ext=="flac";
}
std::string Encode(const std::string& value) {
    static constexpr char hex[]="0123456789ABCDEF";std::string result;
    for(unsigned char c:value) {
        if((std::isalnum(c)&&c<128)||c=='-'||c=='_'||c=='.'||c=='~') result+=char(c);
        else {result+='%';result+=hex[c>>4];result+=hex[c&15];}
    }
    return result;
}
Link ParseLink(const std::string& raw) {
    auto text=raw;
    while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.front()))) text.erase(text.begin());
    while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
    if(text.size()>2048) return {};
    if(text.compare(0,17,"drive.google.com/")==0) text="https://"+text;
    if(text.compare(0,25,"https://drive.google.com/")!=0) return {};
    Link link;auto at=text.find("/folders/",24);
    if(at!=std::string::npos) {link.kind=Kind::Folder;at+=9;}
    else if((at=text.find("/file/d/",24))!=std::string::npos) {link.kind=Kind::File;at+=8;}
    else {link.kind=Kind::File;link.id=Parameter(text,"id");}
    if(at!=std::string::npos) link.id=text.substr(at,text.find_first_of("/?#",at)-at);
    link.key=Parameter(text,"resourcekey");
    if(!ValidId(link.id)||(!link.key.empty()&&!ValidId(link.key))) return {};
    return link;
}
std::string Canonical(const Link& link) {
    if(!ValidId(link.id)) return {};
    return "https://drive.google.com/"+std::string(link.kind==Kind::Folder?"drive/folders/":"file/d/")+link.id+
        (link.key.empty()?"":"?resourcekey="+Encode(link.key));
}
std::string DownloadUrl(const Link& link) {
    if(!ValidId(link.id)) return {};
    return "https://drive.google.com/uc?export=download&id="+link.id+(link.key.empty()?"":"&resourcekey="+Encode(link.key));
}
bool AllowedHost(const std::string& url) {
    if(url.compare(0,8,"https://")!=0) return false;
    const auto end=url.find_first_of("/?#",8);const auto host=Lower(url.substr(8,end-8));
    const auto suffix=[&](const std::string& tail){return host.size()>tail.size()&&host.compare(host.size()-tail.size(),tail.size(),tail)==0;};
    return host=="drive.google.com"||host=="docs.google.com"||host=="drive.usercontent.google.com"||host=="drive.googleusercontent.com"||suffix(".googleusercontent.com");
}
std::string HtmlText(const std::string& text) {
    std::string result;bool tag=false;
    for(std::size_t i=0;i<text.size();++i) {
        if(text[i]=='<') {tag=true;continue;}if(text[i]=='>') {tag=false;continue;}if(tag) continue;
        if(text[i]=='&') {
            const auto end=text.find(';',i+1);
            if(end!=std::string::npos&&end-i<14) {
                const auto entity=text.substr(i+1,end-i-1);unsigned code=0;
                if(entity=="amp") code='&';else if(entity=="quot") code='"';else if(entity=="apos"||entity=="#39") code='\'';
                else if(entity=="lt") code='<';else if(entity=="gt") code='>';else if(entity=="nbsp") code=' ';
                else if(entity.size()>1&&entity[0]=='#') {
                    const bool hex=entity.size()>2&&(entity[1]=='x'||entity[1]=='X');const auto digits=entity.substr(hex?2:1);
                    bool valid=!digits.empty();
                    for(char c:digits) {const int d=hex?Hex(c):(c>='0'&&c<='9'?c-'0':-1);if(d<0||code>0x10ffffu/(hex?16u:10u)) {valid=false;break;}code=code*(hex?16:10)+d;}
                    if(!valid||code>0x10ffff||(code>=0xd800&&code<=0xdfff)) code=0;
                }
                if(code) {
                    if(code<0x80) result+=char(code);
                    else if(code<0x800) {result+=char(0xc0|(code>>6));result+=char(0x80|(code&63));}
                    else if(code<0x10000) {result+=char(0xe0|(code>>12));result+=char(0x80|((code>>6)&63));result+=char(0x80|(code&63));}
                    else {result+=char(0xf0|(code>>18));result+=char(0x80|((code>>12)&63));result+=char(0x80|((code>>6)&63));result+=char(0x80|(code&63));}
                    i=end;continue;
                }
            }
        }
        if(static_cast<unsigned char>(text[i])>=32||text[i]=='\t') result+=text[i];
    }
    return result;
}
Catalog ParseFolder(const Link& source,const std::string& html) {
    Catalog result;result.source=source;
    if(!source||html.size()>4u*1024u*1024u||html.find("flip-entries")==std::string::npos) {
        result.error="Couldn't read the shared folder. Check sharing and try again.";return result;
    }
    std::set<std::string> seen;std::size_t at=0;
    while((at=html.find("class=\"flip-entry\"",at))!=std::string::npos) {
        const auto next=html.find("class=\"flip-entry\"",at+1);
        const auto entry=html.substr(at,(next==std::string::npos?html.size():next)-at);at=next==std::string::npos?html.size():next;
        const auto href=entry.find("href="),title=entry.find("class=\"flip-entry-title\"");
        if(href==std::string::npos||title==std::string::npos) continue;
        auto link=ParseLink(Attribute(entry.substr(href),"href"));
        const auto start=entry.find('>',title),end=entry.find("</div>",start);
        if(!link||link.kind!=Kind::File||start==std::string::npos||end==std::string::npos) continue;
        auto name=HtmlText(entry.substr(start+1,end-start-1));
        if(name.size()>1024||!AudioName(name)||!seen.insert(link.id).second) continue;
        if(result.tracks.size()>=trackLimit) {result.truncated=true;break;}
        name.resize(name.find_last_of('.'));result.tracks.push_back({std::move(link),std::move(name)});
    }
    std::sort(result.tracks.begin(),result.tracks.end(),[](const Entry& a,const Entry& b){return Lower(a.title)<Lower(b.title);});
    return result;
}
bool AudioBytes(const unsigned char* data,std::size_t size) {
    if(!data||size<12) return false;
    return std::memcmp(data,"ID3",3)==0||(data[0]==0xff&&(data[1]&0xe0)==0xe0)||std::memcmp(data,"fLaC",4)==0||
        std::memcmp(data,"OggS",4)==0||(std::memcmp(data,"RIFF",4)==0&&std::memcmp(data+8,"WAVE",4)==0)||std::memcmp(data+4,"ftyp",4)==0;
}
bool Client::Configure(const std::string& directory) {
    if(directory.empty()) return false;
    if(mkdir(directory.c_str(),0700)!=0&&errno!=EEXIST) return false;
    struct stat info{};if(stat(directory.c_str(),&info)!=0||!S_ISDIR(info.st_mode)) return false;
    root_=directory;
#ifdef __ANDROID__
    certificate_=root_+"/roots.pem";
    if(!Write(certificate_,std::string(reinterpret_cast<const char*>(drive_ca_start),drive_ca_end-drive_ca_start))) {root_.clear();return false;}
#elif defined(DRIVE_HTTP_TEST) && defined(DRIVE_TEST_CA_FILE)
    certificate_=DRIVE_TEST_CA_FILE;
#endif
    return true;
}
Catalog Client::Read(const Link& link,const Canceled& canceled) {
    Catalog result;result.source=link;
    if(!link) {result.error="Paste a Google Drive folder or audio-file sharing link.";return result;}
#ifdef __ANDROID__
    if(certificate_.empty()) {result.error="The Drive music cache isn't ready.";return result;}
#endif
    const auto url=link.kind==Kind::Folder?"https://drive.google.com/embeddedfolderview?id="+link.id+(link.key.empty()?"":"&resourcekey="+Encode(link.key)):Canonical(link);
    const auto response=Get(url,certificate_,4u*1024u*1024u,nullptr,canceled);
    if(response.canceled) {result.error="Canceled";return result;}
    if(!response.error.empty()) {result.error=response.error;return result;}
    if(link.kind==Kind::Folder) return ParseFolder(link,response.body);
    const auto start=response.body.find("<title>"),end=response.body.find("</title>",start);
    auto title=start!=std::string::npos&&end!=std::string::npos?HtmlText(response.body.substr(start+7,end-start-7)):std::string("Drive track");
    const auto suffix=title.rfind(" - Google Drive");if(suffix!=std::string::npos) title.resize(suffix);
    if(title.size()>1024||!AudioName(title)) {result.error="This link doesn't open a shared audio file. Use an audio file or music folder.";return result;}
    title.resize(title.find_last_of('.'));
    result.tracks.push_back({link,title});return result;
}
std::string Client::Fetch(const Entry& entry,std::string& error,const Canceled& canceled,const Progress& progress,const std::string& protectedPath) {
    error.clear();
    if(canceled&&canceled()) {error="Canceled";return {};}
    if(root_.empty()||!ValidId(entry.link.id)) {error="The Drive music cache isn't ready.";return {};}
    const auto path=root_+"/song-"+entry.link.id+".audio";
    if(CachedAudio(path)) {if(progress) progress(1.f);return path;}
    const auto temporary=path+".part";auto* file=std::fopen(temporary.c_str(),"wb");
    if(!file) {error="Couldn't open the music cache.";return {};}
    const auto response=Get(DownloadUrl(entry.link),certificate_,fileLimit,file,canceled,progress);
    const bool closed=std::fclose(file)==0;
    error=response.error;
    if(response.canceled) error="Canceled";
    if(error.empty()&&(!closed||response.type.find("text/")==0||!CachedAudio(temporary)))
        error="Drive returned a preview or unavailable file. Share an audio file with downloads enabled (under 64 MiB).";
    if(error.empty()&&std::rename(temporary.c_str(),path.c_str())==0) {chmod(path.c_str(),0600);Clean(root_,path,protectedPath);return path;}
    if(error.empty()) error="Couldn't save this music download.";
    std::remove(temporary.c_str());return {};
}
bool Client::Save(const Catalog& catalog) const {
    if(root_.empty()||!catalog.source||!catalog.error.empty()) return false;
    std::string text="DRIVE1\n"+Canonical(catalog.source)+"\n";
    for(const auto& entry:catalog.tracks) text+=entry.link.id+'\t'+entry.link.key+'\t'+Encode(entry.title)+'\n';
    return Write(root_+"/catalog",text);
}
Catalog Client::Restore() const {
    Catalog result;if(root_.empty()) return result;
    const auto text=ReadText(root_+"/catalog",1024u*1024u);
    if(text.compare(0,7,"DRIVE1\n")!=0) return result;
    auto start=text.find('\n',7);if(start==std::string::npos) return result;
    result.source=ParseLink(text.substr(7,start-7));if(!result.source) return {};
    while(++start<text.size()&&result.tracks.size()<trackLimit) {
        const auto end=text.find('\n',start),a=text.find('\t',start),b=a==std::string::npos?std::string::npos:text.find('\t',a+1);
        if(end==std::string::npos||a==std::string::npos||b==std::string::npos||b>=end) break;
        Link link{Kind::File,text.substr(start,a-start),text.substr(a+1,b-a-1)};
        auto title=Decode(text.substr(b+1,end-b-1));
        if(ValidId(link.id)&&(link.key.empty()||ValidId(link.key))&&title.size()<=1024) result.tracks.push_back({link,title});
        start=end;
    }
    return result;
}
}
