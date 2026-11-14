#include "../src/main/jni/System/Audio/DriveMusic.h"
#include "../src/main/jni/System/Audio/UiSounds.h"
#include "../src/main/jni/System/UI/StartupSounds.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <numeric>
#include <sys/stat.h>
#include <unistd.h>

using namespace drive_music;
static const std::string folderId="shared_folder_123456789",firstId="audio_file_123456789",secondId="audio_file_987654321";
static std::string Row(const std::string& id,const std::string& name) {
    return "<div class=\"flip-entry\" id=\"entry-"+id+"\"><a href=\"https://drive.google.com/file/d/"+id+"/view?usp=drive_web\"><div class=\"flip-entry-title\">"+name+"</div></a></div>";
}
static void Put(const std::string& path,const std::string& value) {std::ofstream(path,std::ios::binary)<<value;}
static void U32(std::ostream& out,std::uint32_t n,unsigned bytes=4) {for(unsigned i=0;i<bytes;++i) out.put(char(n>>(8*i)));}
static void Wave(const char* path,const std::vector<std::int16_t>& pcm) {
    std::ofstream out(path,std::ios::binary);out.write("RIFF",4);U32(out,36+pcm.size()*2);out.write("WAVEfmt ",8);
    U32(out,16);U32(out,1,2);U32(out,1,2);U32(out,ui_sounds::sampleRate);U32(out,ui_sounds::sampleRate*2);
    U32(out,2,2);U32(out,16,2);out.write("data",4);U32(out,pcm.size()*2);for(auto v:pcm) U32(out,static_cast<std::uint16_t>(v),2);
}
int main(int argc,char** argv) {
    auto source=ParseLink(" https://drive.google.com/drive/folders/"+folderId+"?usp=sharing&resourcekey=0-public_key_123 \n");
    assert(source&&source.kind==Kind::Folder&&source.id==folderId&&source.key=="0-public_key_123");
    assert(ParseLink(Canonical(source)).key==source.key);
    assert(ParseLink("drive.google.com/file/d/"+firstId+"/view").kind==Kind::File);
    assert(ParseLink("https://drive.google.com/open?id="+firstId).id==firstId);
    assert(!ParseLink("https://drive.google.com.evil.example/drive/folders/"+folderId));
    assert(!ParseLink("https://drive.google.com/drive/folders/../../etc/passwd"));
    assert(!ParseLink("https://drive.google.com/open?id="+firstId+"%00bad"));
    assert(!ParseLink("https://drive.google.com/open?id="+firstId+"&resourcekey=../../outside"));
    assert(AllowedHost("https://drive.usercontent.google.com/download?id=abc"));
    assert(AllowedHost("https://doc-12-34-docs.googleusercontent.com/docs/securesc/test"));
    for(auto s:{"http://drive.google.com/x","https://drive.google.com@evil.test/x","https://drive.google.com.evil.test/x","https://accounts.google.com/login","https://googleusercontent.com.evil.test/x","file:///tmp/private"}) assert(!AllowedHost(s));
    assert(Encode("a b&🎵")=="a%20b%26%F0%9F%8E%B5");
    assert(HtmlText("a &amp; b &#x1f3b5; &lt;hi&gt;")=="a & b 🎵 <hi>");
    auto catalog=ParseFolder(source,"<div class=\"flip-entries\">"+Row(firstId,"Zulu &amp; chill.MP3")+Row(secondId,"alpha &#x1f3b5;.flac")+Row(firstId,"duplicate.mp3")+Row("not_audio_123456","readme.txt")+"</div>");
    assert(catalog.error.empty()&&catalog.tracks.size()==2&&catalog.tracks[0].title=="alpha 🎵"&&catalog.tracks[1].title=="Zulu & chill");
    assert(!ParseFolder(source,"<html>Sign in</html>").error.empty());
    assert(ParseFolder(source,"<div class=\"flip-entries\"></div>").tracks.empty());
    std::string many="flip-entries";for(unsigned i=0;i<260;++i) many+=Row("audio_file_123456"+std::to_string(i),"song.mp3");
    assert(ParseFolder(source,many).truncated&&ParseFolder(source,many).tracks.size()==trackLimit);
    const std::string html="<html><title>Unavailable</title></html>";
    assert(!AudioBytes(reinterpret_cast<const unsigned char*>(html.data()),html.size()));
    char root[]="/tmp/npc-drive-audio-XXXXXX";assert(mkdtemp(root));
    Client client;assert(client.Configure(root)&&client.Save(catalog));
    const auto restored=client.Restore();assert(restored.source.key==source.key&&restored.tracks.size()==2&&restored.tracks[0].title==catalog.tracks[0].title);
    const auto cached=std::string(root)+"/song-"+firstId+".audio";
    Put(cached,std::string("RIFF\x10\0\0\0WAVEfmt ",16)+std::string(64,'\0'));
    std::string error;float progress=0;
    assert(client.Fetch({{Kind::File,firstId,{}},"song"},error,{},[&](float v){progress=v;})==cached&&error.empty()&&progress==1.f);
    assert(client.Fetch({{Kind::File,firstId,{}},"song"},error,[]{return true;}).empty()&&error=="Canceled");
    assert(client.Fetch({{Kind::File,"../../outside",{}},"song"},error).empty());
    Put(std::string(root)+"/catalog","DRIVE1\nhttps://drive.google.com/drive/folders/"+folderId+"\n../../outside\t\tbad\n");
    assert(client.Restore().tracks.empty());
    unlink(cached.c_str());unlink((std::string(root)+"/catalog").c_str());rmdir(root);
    std::vector<std::int16_t> sampler;
    for(unsigned i=0;i<static_cast<unsigned>(ui_sounds::Cue::Count);++i) {
        const auto cue=static_cast<ui_sounds::Cue>(i);const auto pcm=ui_sounds::Synthesize(cue);
        assert(pcm==ui_sounds::Synthesize(cue)&&pcm.size()>=2400&&pcm.size()<=19200&&pcm.front()==0&&pcm.back()==0);
        int peak=0,jump=0;long long sum=0;
        for(std::size_t n=0;n<pcm.size();++n) {peak=std::max(peak,std::abs(int(pcm[n])));sum+=pcm[n];if(n) jump=std::max(jump,std::abs(int(pcm[n])-pcm[n-1]));}
        assert(peak>1000&&peak<26000&&jump<5000&&std::abs(double(sum)/pcm.size())<100.);
        for(auto v:pcm) sampler.push_back(static_cast<std::int16_t>(v*ui_sounds::defaultGain));
        sampler.insert(sampler.end(),ui_sounds::sampleRate/4,0);
    }
    assert(ui_sounds::Synthesize(ui_sounds::Cue::Count).empty());
    startup_ui::SoundObserver observer;startup_ui::LoginTransitionFrame frame;
    const auto observe=[&](startup_ui::AuthStage stage,ui_sounds::Cue expected) {frame.stage=stage;assert(observer.Update(frame)==expected);assert(!observer.Update(frame));};
    observe(startup_ui::AuthStage::Splash,ui_sounds::Cue::Drop);observe(startup_ui::AuthStage::Check,ui_sounds::Cue::Success);
    observe(startup_ui::AuthStage::Suck,ui_sounds::Cue::Whoosh);observe(startup_ui::AuthStage::Reveal,ui_sounds::Cue::Open);
    frame.stage=startup_ui::AuthStage::Idle;assert(!observer.Update(frame));observe(startup_ui::AuthStage::Check,ui_sounds::Cue::Success);
    frame.stage=startup_ui::AuthStage::Loading;assert(!observer.Update(frame));
    if(argc>1) Wave(argv[1],sampler);
    std::puts("Drive link/parser limits, host restrictions, catalog/cache integrity, cancellation, PCM envelopes and one-shot authentication cues passed");
}
