#include "../src/main/jni/System/Audio/DriveMusic.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
int main(int argc,char** argv) {
    assert(argc==2);
    const auto source=drive_music::ParseLink(argv[1]);assert(source);
    char directory[]="/tmp/npc-drive-http-XXXXXX";assert(mkdtemp(directory));
    drive_music::Client client;assert(client.Configure(directory));
    const auto catalog=client.Read(source);
    if(!catalog.error.empty()) std::fprintf(stderr,"%s\n",catalog.error.c_str());
    assert(catalog.error.empty()&&!catalog.tracks.empty());
    std::string error;
    const auto audio=client.Fetch(catalog.tracks.front(),error);
    if(!error.empty()) std::fprintf(stderr,"%s\n",error.c_str());
    struct stat info{};assert(!audio.empty()&&error.empty()&&stat(audio.c_str(),&info)==0);
    assert(info.st_size>=12&&static_cast<std::uint64_t>(info.st_size)<=drive_music::fileLimit);
    unsigned char header[32]{};std::ifstream stream(audio,std::ios::binary);stream.read(reinterpret_cast<char*>(header),sizeof(header));
    assert(drive_music::AudioBytes(header,stream.gcount()));
    float progress=0;
    assert(client.Fetch(catalog.tracks.front(),error,{},[&](float p){progress=p;})==audio&&progress==1.f&&error.empty());
    assert(client.Save(catalog));const auto restored=client.Restore();
    assert(restored.source.id==source.id&&restored.tracks.size()==catalog.tracks.size()&&restored.tracks.front().title==catalog.tracks.front().title);
    std::printf("Anonymous Drive TLS/list/download passed: %zu tracks, %lld bytes; audio signature, cache reuse and catalog persistence verified\n",catalog.tracks.size(),static_cast<long long>(info.st_size));
    unlink(audio.c_str());unlink((std::string(directory)+"/catalog").c_str());rmdir(directory);
}
