#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/System/UI/MusicPlayer.h"
#include <cassert>
#include <fstream>
#include <unistd.h>

static void File(const std::string& path) { std::ofstream(path).put('x'); }
int main() {
    char root[]="/tmp/npc-music-XXXXXX";
    assert(mkdtemp(root));
    const std::string a=std::string(root)+"/a",b=std::string(root)+"/b";
    assert(mkdir(a.c_str(),0700)==0 && mkdir(b.c_str(),0700)==0);
    File(a+"/Zulu.MP3");File(a+"/alpha.flac");File(a+"/.hidden.mp3");File(a+"/readme.txt");
    assert(mkdir((a+"/folder.wav").c_str(),0700)==0);
    File(b+"/Beta.ogg");
    assert(symlink((a+"/alpha.flac").c_str(),(b+"/duplicate.flac").c_str())==0);
    const auto library=music_player::ReadLibrary({a,b,a});
    assert(library.readable && !library.truncated && library.tracks.size()==3);
    assert(library.tracks[0].title=="alpha" && library.tracks[1].title=="Beta" && library.tracks[2].title=="Zulu");
    const auto repeat=music_player::ReadLibrary({a,b});
    assert(repeat.tracks.size()==library.tracks.size());
    for(std::size_t i=0;i<library.tracks.size();++i) assert(repeat.tracks[i].path==library.tracks[i].path);
    const auto limited=music_player::ReadLibrary({a,b},2);
    assert(limited.tracks.size()==2 && limited.truncated);
    const auto missing=music_player::ReadLibrary({std::string(root)+"/missing"});
    assert(!missing.readable && missing.tracks.empty());
    assert(music_player::AudioFile("recording.AAC") && !music_player::AudioFile("video.mp4"));
    assert(music_player::Utf16("Music \xf0\x9f\x8e\xb5")==u"Music \U0001f3b5");
    assert(music_player::Utf16("\xed\xa0\x80")==u"\ufffd");
    assert(music_player::Clock(65.9f)=="1:05" && music_player::Clock(-5)=="0:00");
    {
        music_player::Service service;
        service.Start();
        service.Queue({music_player::Action::Scan,-1,0.f,{a,b}});
        auto wait=[&](auto predicate) {
            for(int i=0;i<400;++i) {
                auto snapshot=service.Read();
                if(predicate(*snapshot)) return snapshot;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            assert(false);return service.Read();
        };
        auto first=wait([&](const auto& s){return s.scanned && !s.scanning && s.folders.size()==2 && s.folders[0]==a;});
        assert(first->tracks.size()==3);
        service.Queue({music_player::Action::Scan,-1,0.f,{b}});
        auto second=wait([&](const auto& s){return s.scanned && !s.scanning && s.folders.size()==1 && s.folders[0]==b;});
        assert(second->tracks.size()==2 && first->tracks.size()==3);
        service.Queue({music_player::Action::Mode,1});
        service.Queue({music_player::Action::Provider,1});
        wait([](const auto& s){return s.cloudMode && s.provider==music_player::CloudProvider::YouTubeMusic;});
        service.Queue({music_player::Action::OpenCloud});
        auto cloud=wait([](const auto& s){return s.cloudMode && !s.error.empty();});
        assert(!cloud->externalPlaying && !cloud->available);
        service.Queue({music_player::Action::Mode,0});
        wait([](const auto& s){return !s.cloudMode;});
    }
    unlink((a+"/Zulu.MP3").c_str());unlink((a+"/alpha.flac").c_str());unlink((a+"/.hidden.mp3").c_str());unlink((a+"/readme.txt").c_str());
    rmdir((a+"/folder.wav").c_str());unlink((b+"/Beta.ogg").c_str());unlink((b+"/duplicate.flac").c_str());
    rmdir(a.c_str());rmdir(b.c_str());rmdir(root);
    std::puts("Music scan filtering, ordering, deduplication, limits, immutable snapshots, rescans and cloud-mode state passed");
}
