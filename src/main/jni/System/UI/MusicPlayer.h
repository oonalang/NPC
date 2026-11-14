#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/TouchScroll.h"
#include "../../Fonts/Icon.h"
#include "Keyboard.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dirent.h>
#include <limits.h>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>
#ifdef __ANDROID__
#include <jni.h>
#endif

namespace music_player {
inline constexpr const char* AppFolder="/storage/emulated/0/Android/data/com.garena.game.codm/files/Music";
struct Track { std::string path,title; };
struct LibraryResult { std::vector<Track> tracks; bool readable=false,truncated=false; };
inline bool AudioFile(const std::string& name) {
    const auto dot=name.find_last_of('.');
    if(dot==std::string::npos || dot==0) return false;
    auto ext=name.substr(dot+1);
    for(char& c:ext) if(c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    return ext=="mp3" || ext=="m4a" || ext=="aac" || ext=="ogg" || ext=="wav" || ext=="flac";
}
inline LibraryResult ReadLibrary(const std::vector<std::string>& folders,std::size_t limit=256) {
    LibraryResult result;
    std::set<std::string> paths;
    for(const auto& folder:folders) {
        auto* directory=opendir(folder.c_str());
        if(!directory) continue;
        result.readable=true;
        while(auto* entry=readdir(directory)) {
            if(entry->d_name[0]=='.' || !AudioFile(entry->d_name)) continue;
            const auto path=folder+"/"+entry->d_name;
            struct stat info{};
            if(stat(path.c_str(),&info)!=0 || !S_ISREG(info.st_mode)) continue;
            char canonical[PATH_MAX];
            const auto unique=realpath(path.c_str(),canonical)?std::string(canonical):path;
            if(!paths.insert(unique).second) continue;
            if(result.tracks.size()>=limit) { result.truncated=true;break; }
            std::string title=entry->d_name;
            title.resize(title.find_last_of('.'));
            result.tracks.push_back({unique,title});
        }
        closedir(directory);
        if(result.truncated) break;
    }
    std::sort(result.tracks.begin(),result.tracks.end(),[](const Track& a,const Track& b) {
        std::string x=a.title,y=b.title;
        for(char& c:x) if(c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
        for(char& c:y) if(c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
        return x==y?a.path<b.path:x<y;
    });
    return result;
}
inline std::u16string Utf16(const std::string& text) {
    std::u16string output;
    for(std::size_t i=0;i<text.size();) {
        const auto first=static_cast<unsigned char>(text[i++]);
        unsigned code=first;
        int continuation=0;
        if(first>=0xc2 && first<=0xdf) {code=first&31;continuation=1;}
        else if(first>=0xe0 && first<=0xef) {code=first&15;continuation=2;}
        else if(first>=0xf0 && first<=0xf4) {code=first&7;continuation=3;}
        else if(first>=0x80) {output.push_back(u'\ufffd');continue;}
        bool valid=i+continuation<=text.size();
        for(int j=0;j<continuation && valid;++j) {
            const auto next=static_cast<unsigned char>(text[i+j]);
            if((next&0xc0)!=0x80) valid=false;
            else code=(code<<6)|(next&63);
        }
        if(!valid) {output.push_back(u'\ufffd');continue;}
        i+=continuation;
        if((continuation==1 && code<0x80) || (continuation==2 && code<0x800) ||
            (continuation==3 && code<0x10000) || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) {
            output.push_back(u'\ufffd');continue;
        }
        if(code<=0xffff) output.push_back(static_cast<char16_t>(code));
        else {code-=0x10000;output.push_back(static_cast<char16_t>(0xd800+(code>>10)));output.push_back(static_cast<char16_t>(0xdc00+(code&1023)));}
    }
    return output;
}
inline std::string Clock(float seconds) {
    const auto total=std::isfinite(seconds)?static_cast<int>(std::clamp(seconds,0.f,359999.f)):0;
    char text[24];std::snprintf(text,sizeof(text),"%d:%02d",total/60,total%60);return text;
}
enum class CloudProvider { Spotify,YouTubeMusic };
inline const char* ProviderName(CloudProvider provider) { return provider==CloudProvider::Spotify?"Spotify":"YouTube Music"; }
struct Snapshot {
    std::vector<Track> tracks;
    std::vector<std::string> folders{AppFolder};
    std::string appFolder=AppFolder;
    int current=-1;
    bool playing=false,loading=false,scanning=false,scanned=false,available=false,truncated=false;
    float position=0.f,length=0.f;
    double positionAt=0.;
    std::string error;
    bool cloudMode=false,externalPlaying=false;
    CloudProvider provider=CloudProvider::Spotify;
};
enum class Action { Scan,Play,Toggle,Previous,Next,Seek,Stop,Mode,Provider,OpenCloud,CloudToggle,CloudPrevious,CloudNext };
struct Command { Action action; int index=-1; float position=0.f; std::vector<std::string> folders; std::string path; };
#ifdef __ANDROID__
using PlatformVM=JavaVM;
class NativePlayer {
    JavaVM* vm_=nullptr;
    JNIEnv* env_=nullptr;
    bool attached_=false;
    jclass type_=nullptr;
    jobject player_=nullptr;
    jmethodID ctor_=nullptr,source_=nullptr,prepare_=nullptr,start_=nullptr,pause_=nullptr,release_=nullptr;
    jmethodID playing_=nullptr,duration_=nullptr,position_=nullptr,seek_=nullptr;
    bool Failed() { if(!env_->ExceptionCheck()) return false;env_->ExceptionClear();return true; }
public:
    bool Open(JavaVM* vm) {
        if(type_) return true;
        if(!vm) return false;
        vm_=vm;
        const auto state=vm_->GetEnv(reinterpret_cast<void**>(&env_),JNI_VERSION_1_6);
        if(state==JNI_EDETACHED) { if(vm_->AttachCurrentThread(&env_,nullptr)!=JNI_OK) return false;attached_=true; }
        else if(state!=JNI_OK) return false;
        auto local=env_->FindClass("android/media/MediaPlayer");
        if(Failed() || !local) return false;
        type_=static_cast<jclass>(env_->NewGlobalRef(local));env_->DeleteLocalRef(local);
        if(Failed() || !type_) return false;
        ctor_=env_->GetMethodID(type_,"<init>","()V");
        source_=env_->GetMethodID(type_,"setDataSource","(Ljava/lang/String;)V");
        prepare_=env_->GetMethodID(type_,"prepare","()V");start_=env_->GetMethodID(type_,"start","()V");
        pause_=env_->GetMethodID(type_,"pause","()V");release_=env_->GetMethodID(type_,"release","()V");
        playing_=env_->GetMethodID(type_,"isPlaying","()Z");duration_=env_->GetMethodID(type_,"getDuration","()I");
        position_=env_->GetMethodID(type_,"getCurrentPosition","()I");seek_=env_->GetMethodID(type_,"seekTo","(I)V");
        const bool failed=Failed();
        if(failed || !ctor_ || !source_ || !prepare_ || !start_ || !pause_ || !release_ || !playing_ || !duration_ || !position_ || !seek_) {
            env_->DeleteGlobalRef(type_);type_=nullptr;return false;
        }
        return true;
    }
    void Stop() {
        if(player_) {
            env_->CallVoidMethod(player_,release_);Failed();env_->DeleteGlobalRef(player_);player_=nullptr;
        }
    }
    bool Play(const std::string& path) {
        Stop();
        auto local=env_->NewObject(type_,ctor_);
        if(Failed() || !local) return false;
        const auto utf=Utf16(path);
        auto source=env_->NewString(reinterpret_cast<const jchar*>(utf.data()),static_cast<jsize>(utf.size()));
        if(source && !Failed()) env_->CallVoidMethod(local,source_,source);
        bool failed=Failed() || !source;
        if(source) env_->DeleteLocalRef(source);
        if(!failed) { env_->CallVoidMethod(local,prepare_);failed=Failed(); }
        if(!failed) { env_->CallVoidMethod(local,start_);failed=Failed(); }
        if(!failed) { player_=env_->NewGlobalRef(local);failed=Failed() || !player_; }
        if(failed) { env_->CallVoidMethod(local,release_);Failed(); }
        env_->DeleteLocalRef(local);
        return !failed;
    }
    bool Toggle(bool playing,bool finished) {
        if(!player_) return false;
        if(finished) { env_->CallVoidMethod(player_,seek_,0);if(Failed()) return false; }
        env_->CallVoidMethod(player_,playing?pause_:start_);return !Failed();
    }
    bool Seek(float seconds) {
        if(!player_) return false;
        env_->CallVoidMethod(player_,seek_,static_cast<jint>(std::clamp(seconds,0.f,2147483.f)*1000.f));return !Failed();
    }
    bool Poll(Snapshot& state) {
        if(!player_) return false;
        const bool playing=env_->CallBooleanMethod(player_,playing_)==JNI_TRUE;
        if(Failed()) return false;
        const int duration=env_->CallIntMethod(player_,duration_);if(Failed()) return false;
        const int position=env_->CallIntMethod(player_,position_);if(Failed()) return false;
        state.playing=playing;state.length=std::max(0,duration)/1000.f;state.position=std::max(0,position)/1000.f;
        state.positionAt=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        return true;
    }
    std::string ExternalMusicFolder() {
        if(!env_ || env_->PushLocalFrame(16)<0) {if(env_) Failed();return {};}
        const auto result=[&]() -> std::string {
            auto activity=env_->FindClass("android/app/ActivityThread");if(Failed() || !activity) return {};
            auto current=env_->GetStaticMethodID(activity,"currentApplication","()Landroid/app/Application;");if(Failed() || !current) return {};
            auto app=env_->CallStaticObjectMethod(activity,current);if(Failed() || !app) return {};
            auto appClass=env_->GetObjectClass(app);
            auto getFiles=env_->GetMethodID(appClass,"getExternalFilesDir","(Ljava/lang/String;)Ljava/io/File;");if(Failed() || !getFiles) return {};
            auto directory=env_->CallObjectMethod(app,getFiles,nullptr);if(Failed() || !directory) return {};
            auto fileClass=env_->GetObjectClass(directory);
            auto absolute=env_->GetMethodID(fileClass,"getAbsolutePath","()Ljava/lang/String;");if(Failed() || !absolute) return {};
            auto text=static_cast<jstring>(env_->CallObjectMethod(directory,absolute));if(Failed() || !text) return {};
            const char* chars=env_->GetStringUTFChars(text,nullptr);if(Failed() || !chars) return {};
            std::string path(chars);env_->ReleaseStringUTFChars(text,chars);return path+"/Music";
        }();
        Failed();env_->PopLocalFrame(nullptr);return result;
    }
    bool CloudAction(int key,CloudProvider provider,bool launch,std::string& error) {
        if(!env_ || env_->PushLocalFrame(32)<0) { if(env_) Failed();return false; }
        const bool ok=[&] {
            auto activity=env_->FindClass("android/app/ActivityThread");if(Failed() || !activity) return false;
            auto current=env_->GetStaticMethodID(activity,"currentApplication","()Landroid/app/Application;");if(Failed() || !current) return false;
            auto app=env_->CallStaticObjectMethod(activity,current);if(Failed() || !app) return false;
            auto appClass=env_->GetObjectClass(app);
            if(launch) {
                auto intentClass=env_->FindClass("android/content/Intent");if(Failed() || !intentClass) return false;
                auto uriClass=env_->FindClass("android/net/Uri");if(Failed() || !uriClass) return false;
                auto parse=env_->GetStaticMethodID(uriClass,"parse","(Ljava/lang/String;)Landroid/net/Uri;");if(Failed() || !parse) return false;
                auto ctor=env_->GetMethodID(intentClass,"<init>","(Ljava/lang/String;Landroid/net/Uri;)V");if(Failed() || !ctor) return false;
                auto setPackage=env_->GetMethodID(intentClass,"setPackage","(Ljava/lang/String;)Landroid/content/Intent;");if(Failed() || !setPackage) return false;
                auto flags=env_->GetMethodID(intentClass,"addFlags","(I)Landroid/content/Intent;");if(Failed() || !flags) return false;
                auto category=env_->GetMethodID(intentClass,"addCategory","(Ljava/lang/String;)Landroid/content/Intent;");if(Failed() || !category) return false;
                auto start=env_->GetMethodID(appClass,"startActivity","(Landroid/content/Intent;)V");if(Failed() || !start) return false;
                const auto package=env_->NewStringUTF(provider==CloudProvider::Spotify?"com.spotify.music":"com.google.android.apps.youtube.music");
                const auto attempt=[&](const char* address,bool packaged,bool launcher) {
                    jobject uri=nullptr;
                    if(address) {
                        auto text=env_->NewStringUTF(address);if(Failed() || !text) return false;
                        uri=env_->CallStaticObjectMethod(uriClass,parse,text);if(Failed() || !uri) return false;
                    }
                    auto action=env_->NewStringUTF(launcher?"android.intent.action.MAIN":"android.intent.action.VIEW");if(Failed() || !action) return false;
                    auto intent=env_->NewObject(intentClass,ctor,action,uri);if(Failed() || !intent) return false;
                    if(packaged) {env_->CallObjectMethod(intent,setPackage,package);if(Failed()) return false;}
                    if(launcher) {
                        auto name=env_->NewStringUTF("android.intent.category.LAUNCHER");if(Failed() || !name) return false;
                        env_->CallObjectMethod(intent,category,name);if(Failed()) return false;
                    }
                    env_->CallObjectMethod(intent,flags,0x10000000);if(Failed()) return false;
                    env_->CallVoidMethod(app,start,intent);return !Failed();
                };
                if(attempt(provider==CloudProvider::Spotify?"spotify:collection":"https://music.youtube.com/library",true,false)) return true;
                if(attempt(nullptr,true,true)) return true;
                return attempt(provider==CloudProvider::Spotify?"https://open.spotify.com/":"https://music.youtube.com/library",false,false);
            }
            auto service=env_->GetMethodID(appClass,"getSystemService","(Ljava/lang/String;)Ljava/lang/Object;");if(Failed() || !service) return false;
            auto audioName=env_->NewStringUTF("audio");auto audio=env_->CallObjectMethod(app,service,audioName);if(Failed() || !audio) return false;
            auto audioClass=env_->GetObjectClass(audio);
            if(key<0) {
                auto active=env_->GetMethodID(audioClass,"isMusicActive","()Z");if(Failed() || !active) return false;
                return env_->CallBooleanMethod(audio,active)==JNI_TRUE && !Failed();
            }
            auto dispatch=env_->GetMethodID(audioClass,"dispatchMediaKeyEvent","(Landroid/view/KeyEvent;)V");if(Failed() || !dispatch) return false;
            auto eventClass=env_->FindClass("android/view/KeyEvent");if(Failed() || !eventClass) return false;
            auto eventCtor=env_->GetMethodID(eventClass,"<init>","(II)V");if(Failed() || !eventCtor) return false;
            for(int action:{0,1}) {
                auto event=env_->NewObject(eventClass,eventCtor,action,key);if(Failed() || !event) return false;
                env_->CallVoidMethod(audio,dispatch,event);if(Failed()) return false;
            }
            return true;
        }();
        if(Failed()) error="The music app couldn't receive this action.";
        env_->PopLocalFrame(nullptr);
        if(!ok && key>=0 && error.empty()) error=launch?"Couldn't open the selected music app.":"No compatible active music session was found.";
        return ok;
    }
    ~NativePlayer() {
        Stop();
        if(type_) env_->DeleteGlobalRef(type_);
        if(attached_ && vm_) vm_->DetachCurrentThread();
    }
};
#else
using PlatformVM=void;
class NativePlayer {
public:
    bool Open(void*) { return false; }
    void Stop() {}
    bool Play(const std::string&) { return false; }
    bool Toggle(bool,bool) { return false; }
    bool Seek(float) { return false; }
    bool Poll(Snapshot&) { return false; }
    bool CloudAction(int,CloudProvider,bool,std::string&) { return false; }
    std::string ExternalMusicFolder() { return AppFolder; }
};
#endif
class Service {
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<Command> commands_;
    std::shared_ptr<const Snapshot> published_=std::make_shared<Snapshot>();
    std::thread worker_;
    PlatformVM* vm_=nullptr;
    bool stopping_=false;
    void Publish(const Snapshot& state) {
        auto next=std::make_shared<Snapshot>(state);
        std::lock_guard<std::mutex> lock(mutex_);published_=std::move(next);
    }
    void Run() {
        Snapshot state;
        NativePlayer player;
        bool sourceConfigured=false,defaultResolved=false;
        for(;;) {
            Command command{Action::Stop};bool hasCommand=false;
            PlatformVM* vm=nullptr;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait_for(lock,std::chrono::milliseconds(250),[&]{return stopping_ || !commands_.empty();});
                if(stopping_) break;
                vm=vm_;
                if(!commands_.empty()) {command=std::move(commands_.front());commands_.pop_front();hasCommand=true;}
            }
            const bool wasAvailable=state.available;
            if(!state.available && vm) state.available=player.Open(vm);
            if(state.available && !defaultResolved) {
                const auto folder=player.ExternalMusicFolder();
                if(!folder.empty()) {
                    state.appFolder=folder;defaultResolved=true;
                    if(!sourceConfigured) {
                        state.folders={folder};
                        if(!hasCommand) {command={Action::Scan};hasCommand=true;}
                    }
                }
            }
            if(hasCommand && command.action==Action::Scan) {
                state.scanning=true;state.error.clear();Publish(state);
                if(!command.folders.empty()) sourceConfigured=true;
                auto folders=command.folders.empty()?state.folders:command.folders;
#ifdef __ANDROID__
                if(!sourceConfigured && !defaultResolved) folders.clear();
#endif
                if(folders.size()==1 && folders.front()==state.appFolder) mkdir(state.appFolder.c_str(),0755);
                auto library=ReadLibrary(folders);
                std::string selected;
                if(state.current>=0 && state.current<static_cast<int>(state.tracks.size())) selected=state.tracks[state.current].path;
                state.tracks=std::move(library.tracks);state.folders=folders;state.current=-1;
                for(std::size_t i=0;i<state.tracks.size();++i) if(state.tracks[i].path==selected) state.current=static_cast<int>(i);
                if(!selected.empty() && state.current<0) {player.Stop();state.playing=false;state.position=state.length=0.f;}
                state.scanning=false;state.scanned=true;state.truncated=library.truncated;
                if(!library.readable) state.error="Can't read this folder. Choose a folder this app can access.";
            } else if(hasCommand && command.action==Action::Mode) {
                const bool cloud=command.index!=0;
                state.error.clear();
                if(cloud && state.playing) {player.Toggle(true,false);player.Poll(state);}
                if(!cloud && state.cloudMode) player.CloudAction(127,state.provider,false,state.error);
                state.cloudMode=cloud;
            } else if(hasCommand && command.action==Action::Provider) {
                state.provider=command.index==0?CloudProvider::Spotify:CloudProvider::YouTubeMusic;state.error.clear();
            } else if(hasCommand && (command.action==Action::OpenCloud || command.action==Action::CloudToggle || command.action==Action::CloudPrevious || command.action==Action::CloudNext)) {
                if(state.playing) {player.Toggle(true,false);player.Poll(state);}
                state.cloudMode=true;state.error.clear();
                const int key=command.action==Action::CloudPrevious?88:command.action==Action::CloudNext?87:85;
                if(!state.available || !player.CloudAction(key,state.provider,command.action==Action::OpenCloud,state.error)) {
                    if(state.error.empty()) state.error="Cloud controls are available in the Android app.";
                }
            } else if(hasCommand) {
                if(command.action==Action::Stop) {player.Stop();state.current=-1;state.playing=false;state.position=state.length=0.f;}
                else if(!state.available) state.error="Audio playback is available in the Android app.";
                else {
                    int index=command.index;
                    if(command.action==Action::Play && !command.path.empty()) {
                        index=-1;
                        for(std::size_t i=0;i<state.tracks.size();++i) if(state.tracks[i].path==command.path) {index=static_cast<int>(i);break;}
                    }
                    if(command.action==Action::Previous) index=state.current<0?0:(state.current-1+static_cast<int>(state.tracks.size()))%std::max(1,static_cast<int>(state.tracks.size()));
                    if(command.action==Action::Next) index=state.current<0?0:(state.current+1)%std::max(1,static_cast<int>(state.tracks.size()));
                    if(command.action==Action::Toggle && state.current<0) index=0;
                    const bool selecting=command.action==Action::Play || command.action==Action::Previous || command.action==Action::Next || (command.action==Action::Toggle && state.current<0);
                    if(selecting && index>=0 && index<static_cast<int>(state.tracks.size())) {
                        if(state.cloudMode) player.CloudAction(127,state.provider,false,state.error);
                        state.cloudMode=false;state.loading=true;state.playing=false;state.error.clear();Publish(state);
                        const bool ok=player.Play(state.tracks[index].path);
                        state.loading=false;
                        if(ok) {state.current=index;player.Poll(state);}
                        else {state.current=-1;state.position=state.length=0.f;state.error="Couldn't play this track. Try another audio file.";}
                    } else if(command.action==Action::Toggle && state.current>=0) {
                        if(!player.Toggle(state.playing,state.length>0 && state.position>=state.length-.1f)) state.error="Playback couldn't be changed.";
                        else state.error.clear();
                    } else if(command.action==Action::Seek && state.current>=0) {
                        if(!player.Seek(std::clamp(command.position,0.f,state.length))) state.error="This track couldn't seek to that position.";
                    }
                }
            }
            if(state.current>=0 && !player.Poll(state)) {player.Stop();state.playing=false;state.current=-1;state.position=state.length=0;state.error="Playback stopped. Choose a track to try again.";}
            if(state.cloudMode && state.available) {std::string ignored;state.externalPlaying=player.CloudAction(-1,state.provider,false,ignored);}
            if(hasCommand || state.current>=0 || state.cloudMode || wasAvailable!=state.available) Publish(state);
        }
    }
public:
    ~Service() {
        {std::lock_guard<std::mutex> lock(mutex_);stopping_=true;}
        wake_.notify_all();if(worker_.joinable()) worker_.join();
    }
    void Start(PlatformVM* vm=nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(vm) vm_=vm;
        if(!worker_.joinable()) {commands_.push_back({Action::Scan});worker_=std::thread([this]{Run();});}
    }
    std::shared_ptr<const Snapshot> Read() { std::lock_guard<std::mutex> lock(mutex_);return published_; }
    void Queue(Command command) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(stopping_) return;
        if(command.action==Action::Seek || command.action==Action::Scan)
            commands_.erase(std::remove_if(commands_.begin(),commands_.end(),[&](const Command& other){return other.action==command.action;}),commands_.end());
        if(commands_.size()<32) commands_.push_back(std::move(command));
        wake_.notify_one();
    }
};
inline Service& Engine() { static Service service;return service; }
inline void Init(PlatformVM* vm) { Engine().Start(vm); }
inline std::shared_ptr<const Snapshot> GetSnapshot() { return Engine().Read(); }
inline void Scan() { Engine().Queue({Action::Scan}); }
inline void ScanFolders(std::vector<std::string> folders) { Engine().Queue({Action::Scan,-1,0.f,std::move(folders)}); }
inline void Play(int index) {
    const auto state=GetSnapshot();
    if(index>=0 && index<static_cast<int>(state->tracks.size())) Engine().Queue({Action::Play,index,0.f,{},state->tracks[index].path});
}
inline void Stop() { Engine().Queue({Action::Stop}); }
inline void Toggle() { Engine().Queue({Action::Toggle}); }
inline void Next() { Engine().Queue({Action::Next}); }
inline void Previous() { Engine().Queue({Action::Previous}); }
inline void Seek(float seconds) { Engine().Queue({Action::Seek,-1,seconds}); }
inline void Update() {}
inline void SetCloudMode(bool cloud) { Engine().Queue({Action::Mode,cloud?1:0}); }
inline void SetProvider(CloudProvider provider) { Engine().Queue({Action::Provider,provider==CloudProvider::Spotify?0:1}); }
inline void OpenCloud() { Engine().Queue({Action::OpenCloud}); }
inline void CloudToggle() { Engine().Queue({Action::CloudToggle}); }
inline void CloudPrevious() { Engine().Queue({Action::CloudPrevious}); }
inline void CloudNext() { Engine().Queue({Action::CloudNext}); }
inline bool TapButton(const char* id,ImVec2 size) {
    const bool pressed=ImGui::InvisibleButton(id,size);
    return pressed && (!custom::touch::Applies() || custom::touch::Tap());
}
inline bool IconButton(const char* id,const char* icon,ImVec2 size,bool primary=false,bool disabled=false) {
    ImGui::BeginDisabled(disabled);
    const auto lo=ImGui::GetCursorScreenPos();
    const bool pressed=TapButton(id,size);
    const bool hover=ImGui::IsItemHovered(),held=ImGui::IsItemActive();
    auto* draw=ImGui::GetWindowDrawList();
    const auto color=primary?c::accent:ImVec4(.20f,.19f,.27f,1.f);
    draw->AddRectFilled(lo,lo+size,ImGui::GetColorU32(ImVec4(color.x,color.y,color.z,held?.95f:hover?.85f:.72f)),size.y*.5f);
    draw->AddRect(lo,lo+size,ImGui::GetColorU32(ImVec4(.9f,.85f,1.f,.12f)),size.y*.5f);
    auto* font=F107?F107:ImGui::GetFont();const float fontSize=size.y*.42f;
    const auto text=font->CalcTextSizeA(fontSize,FLT_MAX,0,icon);
    draw->AddText(font,fontSize,lo+(size-text)*.5f,ImGui::GetColorU32(ImVec4(1,1,1,1)),icon);
    ImGui::EndDisabled();return pressed;
}
inline bool TextButton(const char* id,const char* label,ImVec2 size,bool selected=false) {
    const auto at=ImGui::GetCursorScreenPos();
    const bool tapped=TapButton(id,size);
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(at,at+size,ImGui::GetColorU32(selected?ImVec4(c::accent.x,c::accent.y,c::accent.z,.24f):ImVec4(.17f,.16f,.23f,.75f)),8.f*c::scale);
    const auto measured=ImGui::CalcTextSize(label);
    draw->AddText(at+(size-measured)*.5f,ImGui::GetColorU32(selected?c::text::text:c::text::text_hov),label);
    return tapped;
}
inline void Draw() {
    Engine().Start();
    const auto snapshot=GetSnapshot();const auto& state=*snapshot;
    const float scale=c::scale,width=ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{6.f*scale,3.f*scale});
    auto* draw=ImGui::GetWindowDrawList();
    const float modeWidth=(width-8.f*scale)*.5f;
    const auto modeButton=[&](const char* id,const char* label,bool selected) {
        const auto at=ImGui::GetCursorScreenPos();
        const bool tapped=TapButton(id,{modeWidth,22.f*scale});
        draw->AddRectFilled(at,at+ImVec2(modeWidth,22.f*scale),ImGui::GetColorU32(selected?ImVec4(c::accent.x,c::accent.y,c::accent.z,.22f):ImVec4(.15f,.14f,.20f,.65f)),8.f*scale);
        const auto measured=ImGui::CalcTextSize(label);
        draw->AddText(at+(ImVec2(modeWidth,22.f*scale)-measured)*.5f,ImGui::GetColorU32(selected?c::text::text:c::text::text_hov),label);
        return tapped;
    };
    if(modeButton("##music_local_mode","Local",!state.cloudMode)) SetCloudMode(false);
    ImGui::SameLine(0,8.f*scale);
    if(modeButton("##music_cloud_mode","Cloud",state.cloudMode)) SetCloudMode(true);
    if(state.cloudMode) {
        const float providerWidth=(width-6.f*scale)*.5f;
        if(TextButton("##cloud_spotify","Spotify",{providerWidth,24.f*scale},state.provider==CloudProvider::Spotify)) SetProvider(CloudProvider::Spotify);
        ImGui::SameLine(0,6.f*scale);
        if(TextButton("##cloud_youtube","YouTube Music",{providerWidth,24.f*scale},state.provider==CloudProvider::YouTubeMusic)) SetProvider(CloudProvider::YouTubeMusic);
        if(TextButton("##cloud_open_library","Open library",{width,28.f*scale},true)) OpenCloud();
        const float h=30.f*scale,gap=9.f*scale;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ImMax(0.f,(width-h*3-gap*2)*.5f));
        if(IconButton("##cloud_previous",ICON_FA_STEP_BACKWARD,{h,h},false,!state.available)) CloudPrevious();
        ImGui::SameLine(0,gap);
        if(IconButton("##cloud_toggle",state.externalPlaying?ICON_FA_PAUSE:ICON_FA_PLAY,{h,h},true,!state.available)) CloudToggle();
        ImGui::SameLine(0,gap);
        if(IconButton("##cloud_next",ICON_FA_STEP_FORWARD,{h,h},false,!state.available)) CloudNext();
        auto* noteDraw=ImGui::GetWindowDrawList();
        noteDraw->AddText(ImGui::GetFont(),13.f*scale,ImGui::GetCursorScreenPos(),ImGui::GetColorU32(c::text::text_hov),"Controls the active music app");
        ImGui::Dummy({0,15.f*scale});
        if(!state.error.empty()) {ImGui::PushTextWrapPos();ImGui::TextColored(ImVec4(.98f,.61f,.67f,1.f),"%s",state.error.c_str());ImGui::PopTextWrapPos();}
        ImGui::PopStyleVar();
        return;
    }
    const auto origin=ImGui::GetCursorScreenPos();
    const float artSize=36.f*scale;
    draw->AddRectFilled(origin,origin+ImVec2(artSize,artSize),ImGui::GetColorU32(ImVec4(c::accent.x,c::accent.y,c::accent.z,.19f)),12.f*scale);
    const float level=state.playing?static_cast<float>(ImGui::GetTime()):0.f;
    for(int i=0;i<4;++i) {
        const float height=(8.f+(state.playing?(std::sin(level*3.f+i*1.4f)+1.f)*10.f:6.f))*scale;
        const float x=origin.x+(8.f+i*6.f)*scale;
        draw->AddRectFilled({x,origin.y+(artSize-height)*.5f},{x+3.f*scale,origin.y+(artSize+height)*.5f},ImGui::GetColorU32(c::accent),2.f*scale);
    }
    const char* title=state.current>=0 && state.current<static_cast<int>(state.tracks.size())?state.tracks[state.current].title.c_str():"Pick a soundtrack";
    const char* detail=state.loading?"Opening track...":state.scanning?"Finding your music...":state.playing?"Playing locally":state.current>=0?"Paused":"Your songs, ready when you are";
    const auto textStart=origin+ImVec2(artSize+10.f*scale,1.f*scale);
    draw->PushClipRect(textStart,{origin.x+width,origin.y+42.f*scale},true);
    draw->AddText(ImGui::GetFont(),18.f*scale,textStart,ImGui::GetColorU32(c::text::text),title);
    draw->AddText(ImGui::GetFont(),13.f*scale,textStart+ImVec2(0,23.f*scale),ImGui::GetColorU32(c::text::text_hov),detail);
    draw->PopClipRect();
    if(ImGui::IsMouseHoveringRect(textStart,{origin.x+width,origin.y+22.f*scale})) ImGui::SetTooltip("%s",title);
    ImGui::SetCursorScreenPos(origin+ImVec2(0,artSize+8.f*scale));
    static float seekPreview=0.f;
    const double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    const float livePosition=ImClamp(state.position+(state.playing?static_cast<float>(std::max(0.,now-state.positionAt)):0.f),0.f,state.length);
    float shown=livePosition;
    const auto seekMin=ImGui::GetCursorScreenPos();const ImVec2 seekSize(width,16.f*scale);
    ImGui::BeginDisabled(state.current<0 || state.length<=0 || state.loading);
    const bool rawRelease=ImGui::InvisibleButton("##music_seek",seekSize);
    if(ImGui::IsItemActivated()) seekPreview=livePosition;
    const bool released=rawRelease && (!custom::touch::Applies() || custom::touch::Tap() || custom::touch::Horizontal());
    const bool dragging=ImGui::IsItemActive() && (!custom::touch::Applies() || custom::touch::Horizontal());
    if(dragging || released) {
        seekPreview=state.length*ImClamp((ImGui::GetIO().MousePos.x-seekMin.x)/ImMax(1.f,width),0.f,1.f);
        if(released) Seek(seekPreview);
    }
    if(dragging || released) shown=seekPreview;
    const float ratio=state.length>0?ImClamp(shown/state.length,0.f,1.f):0.f;
    draw->AddRectFilled(seekMin+ImVec2(0,8.f*scale),seekMin+ImVec2(width,12.f*scale),ImGui::GetColorU32(ImVec4(1,1,1,.12f)),2.f*scale);
    if(ratio>0) draw->AddRectFilled(seekMin+ImVec2(0,8.f*scale),seekMin+ImVec2(width*ratio,12.f*scale),ImGui::GetColorU32(c::accent),2.f*scale);
    draw->AddCircleFilled(seekMin+ImVec2(width*ratio,10.f*scale),4.5f*scale,ImGui::GetColorU32(ImVec4(.96f,.93f,1.f,1.f)),12);
    ImGui::EndDisabled();
    const auto clockAt=ImGui::GetCursorScreenPos();
    const auto leftClock=Clock(shown),rightClock=Clock(state.length);
    const float small=12.f*scale;
    draw->AddText(ImGui::GetFont(),small,clockAt,ImGui::GetColorU32(c::text::text_hov),leftClock.c_str());
    const float rightWidth=ImGui::GetFont()->CalcTextSizeA(small,FLT_MAX,0,rightClock.c_str()).x;
    draw->AddText(ImGui::GetFont(),small,{clockAt.x+width-rightWidth,clockAt.y},ImGui::GetColorU32(c::text::text_hov),rightClock.c_str());
    ImGui::Dummy({0,14.f*scale});
    const float buttonHeight=30.f*scale,buttonGap=6.f*scale;
    const bool compact=width>=272.f*scale;
    const float controlsWidth=buttonHeight*3.f+buttonGap*2.f;
    if(!compact) ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ImMax(0.f,(width-controlsWidth)*.5f));
    const bool disabled=state.tracks.empty() || state.loading || !state.available;
    if(IconButton("##music_previous",ICON_FA_STEP_BACKWARD,{buttonHeight,buttonHeight},false,disabled)) Previous();
    ImGui::SameLine(0,buttonGap);
    if(IconButton("##music_toggle",state.playing?ICON_FA_PAUSE:ICON_FA_PLAY,{buttonHeight,buttonHeight},true,disabled)) Toggle();
    ImGui::SameLine(0,buttonGap);
    if(IconButton("##music_next",ICON_FA_STEP_FORWARD,{buttonHeight,buttonHeight},false,disabled)) Next();
    static bool libraryOpen=false,keyboard=false;
    static char source[512]{};
    if(compact) ImGui::SameLine(0,10.f*scale);
    const auto row=ImGui::GetCursorScreenPos();
    const float refreshWidth=28.f*scale,sourceWidth=28.f*scale;
    const float libraryWidth=ImMax(40.f,width-(compact?controlsWidth+10.f*scale:0.f)-sourceWidth-refreshWidth-12.f*scale);
    if(TapButton("##music_library_toggle",{libraryWidth,30.f*scale})) libraryOpen=!libraryOpen;
    char libraryLabel[64];std::snprintf(libraryLabel,sizeof(libraryLabel),"%s Library %zu",libraryOpen?"-":"+",state.tracks.size());
    draw->AddText(ImGui::GetFont(),13.f*scale,row+ImVec2(0,7.f*scale),ImGui::GetColorU32(c::text::text),libraryLabel);
    ImGui::SameLine(0,6.f*scale);
    if(IconButton("##music_source",ICON_FA_FOLDER_OPEN,{sourceWidth,30.f*scale})) {
        const auto path=state.folders.empty()?state.appFolder:state.folders.front();
        std::snprintf(source,sizeof(source),"%s",path.c_str());keyboard=false;ImGui::OpenPopup("Music source");
    }
    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Music folder");
    ImGui::SameLine(0,6.f*scale);
    if(IconButton("##music_refresh",ICON_FA_SYNC,{refreshWidth,30.f*scale},false,state.scanning)) Scan();
    if(!state.error.empty()) {ImGui::PushTextWrapPos();ImGui::TextColored(ImVec4(.98f,.61f,.67f,1.f),"%s",state.error.c_str());ImGui::PopTextWrapPos();}
    if(libraryOpen) {
        const float listHeight=ImClamp(ImGui::GetContentRegionAvail().y,80.f*scale,220.f*scale);
        ImGui::BeginChild("##music_library",{0,listHeight},false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
        if(state.tracks.empty()) {
            ImGui::TextColored(c::text::text,"No songs here yet");
            ImGui::PushTextWrapPos();ImGui::TextColored(c::text::text_hov,"Add audio files to your selected folder, then refresh.");ImGui::PopTextWrapPos();
        }
        for(std::size_t i=0;i<state.tracks.size();++i) {
            ImGui::PushID(static_cast<int>(i));
            const auto item=ImGui::GetCursorScreenPos();const ImVec2 itemSize(ImGui::GetContentRegionAvail().x,35.f*scale);
            const bool tapped=TapButton("##track",itemSize);
            auto* listDraw=ImGui::GetWindowDrawList();
            if(static_cast<int>(i)==state.current) listDraw->AddRectFilled(item,item+itemSize,ImGui::GetColorU32(ImVec4(c::accent.x,c::accent.y,c::accent.z,.16f)),8.f*scale);
            listDraw->PushClipRect(item+ImVec2(10,0),item+itemSize-ImVec2(8,0),true);
            listDraw->AddText(item+ImVec2(10,8)*scale,ImGui::GetColorU32(static_cast<int>(i)==state.current?c::accent:c::text::text),state.tracks[i].title.c_str());
            listDraw->PopClipRect();
            if(tapped && !state.loading) Play(static_cast<int>(i));
            ImGui::PopID();
        }
        ImGui::EndChild();
        if(state.truncated) ImGui::TextColored(c::text::text_hov,"Showing the first 256 tracks");
    }
    const auto* viewport=ImGui::GetMainViewport();
    const float dialogWidth=ImMin(560.f*scale,viewport->Size.x-28.f);
    if(keyboard) ImGui::SetNextWindowPos({viewport->GetCenter().x,viewport->Pos.y+viewport->Size.y*.22f},ImGuiCond_Always,{.5f,.5f});
    ImGui::SetNextWindowSize({dialogWidth,0},ImGuiCond_Always);
    if(ImGui::BeginPopupModal("Music source",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings)) {
        if(!keyboard) {
            ImGui::TextColored(c::text::text,"Choose your music folder");
            const bool presetsInline=ImGui::GetContentRegionAvail().x>310.f*scale;
            if(TextButton("##music_app_folder","App folder",{130.f*scale,28.f*scale})) {std::snprintf(source,sizeof(source),"%s",state.appFolder.c_str());}
            if(presetsInline) ImGui::SameLine();
            if(TextButton("##music_phone_folder","Phone library",{170.f*scale,28.f*scale})) {ScanFolders({"/storage/emulated/0/Music","/storage/emulated/0/Download"});keyboard=false;ImGui::CloseCurrentPopup();}
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##music_source",source,sizeof(source));
        if(ImGui::GetIO().MouseSource==ImGuiMouseSource_TouchScreen && ImGui::IsItemHovered() &&
            (custom::touch::Applies()?custom::touch::Tap():ImGui::IsItemClicked())) keyboard=true;
        if(TextButton("##music_apply_folder","Use folder",{140.f*scale,30.f*scale},true)) {ScanFolders({source});keyboard=false;ImGui::CloseCurrentPopup();}
        ImGui::SameLine();
        if(TextButton("##music_cancel_folder","Cancel",{100.f*scale,30.f*scale})) {keyboard=false;ImGui::CloseCurrentPopup();}
        ImGui::EndPopup();
    }
    if(keyboard) RenderVirtualKeyboard("##MusicFolderKeyboard",source,sizeof(source),&keyboard);
    ImGui::PopStyleVar();
}
}
