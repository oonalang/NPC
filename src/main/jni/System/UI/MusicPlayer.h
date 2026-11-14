#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/TouchScroll.h"
#include "../../Fonts/Icon.h"
#include "MusicTextDialog.h"
#include "../Audio/DriveMusic.h"
#include "../Audio/UiSounds.h"
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
inline constexpr const char* DefaultDrive="https://drive.google.com/drive/folders/1Hd0PK3TO8rm4JkWfNZpwCmtHKvxgzh0T";
inline constexpr const char* AppFolder="/storage/emulated/0/Android/data/com.garena.game.codm/files/Music";
struct Track { std::string path,title; drive_music::Link remote; };
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
struct Snapshot {
    std::vector<Track> tracks;
    std::vector<std::string> folders{AppFolder};
    std::string appFolder=AppFolder,driveLink=DefaultDrive;
    int current=-1;
    bool playing=false,loading=false,scanning=false,scanned=false,available=false,truncated=false;
    bool driveMode=false;
    float position=0.f,length=0.f,download=0.f;
    double positionAt=0.;
    std::string error;
};
enum class Action { Scan,Play,Toggle,Previous,Next,Seek,Stop,Mode,Connect };
struct Command {
    Action action;int index=-1;float position=0.f;
    std::vector<std::string> folders;std::string path;
    std::uint64_t revision=0;
};
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
    std::atomic<std::uint64_t> revision_{0};
    void Publish(const Snapshot& state) {
        auto next=std::make_shared<Snapshot>(state);
        std::lock_guard<std::mutex> lock(mutex_);published_=std::move(next);
    }
    void Run() {
        Snapshot state;NativePlayer player;drive_music::Client drive;
        LibraryResult local;drive_music::Catalog remote;
        bool sourceConfigured=false,defaultResolved=false,openAttempted=false;
        const auto stop=[&] {player.Stop();state.current=-1;state.playing=false;state.position=state.length=0.f;};
        const auto library=[&] {
            std::string selected;
            if(state.current>=0&&state.current<static_cast<int>(state.tracks.size())) selected=state.tracks[state.current].path;
            state.tracks.clear();state.truncated=state.driveMode?remote.truncated:local.truncated;
            if(state.driveMode) for(const auto& entry:remote.tracks) state.tracks.push_back({drive_music::Canonical(entry.link),entry.title,entry.link});
            else state.tracks=local.tracks;
            state.current=-1;
            for(std::size_t i=0;i<state.tracks.size();++i) if(state.tracks[i].path==selected) state.current=static_cast<int>(i);
            if(!selected.empty()&&state.current<0) stop();
        };
        for(;;) {
            Command command{Action::Stop};bool hasCommand=false;PlatformVM* vm=nullptr;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait_for(lock,std::chrono::milliseconds(250),[&]{return stopping_||!commands_.empty();});
                if(stopping_) break;
                vm=vm_;
                if(!commands_.empty()) {command=std::move(commands_.front());commands_.pop_front();hasCommand=true;}
            }
            const auto canceled=[&] {return revision_.load()!=command.revision;};
            const bool wasAvailable=state.available;
            if(!state.available&&vm&&!openAttempted) {openAttempted=true;state.available=player.Open(vm);}
            if(state.available&&!defaultResolved) {
                const auto folder=player.ExternalMusicFolder();
                if(!folder.empty()) {
                    state.appFolder=folder;mkdir(folder.c_str(),0700);defaultResolved=true;
                    ui_sounds::Configure(folder);
                    if(drive.Configure(folder+"/.drive-cache")) {
                        remote=drive.Restore();if(remote.source) state.driveLink=drive_music::Canonical(remote.source);
                        if(state.driveMode) library();
                    }
                    if(!sourceConfigured) {
                        state.folders={folder};
                        if(!hasCommand) {command={Action::Scan};command.revision=revision_;hasCommand=true;}
                    }
                }
            }
            const auto connect=[&](const drive_music::Link& link) {
                if(canceled()) return;
                state.scanning=true;state.error.clear();Publish(state);
                auto catalog=drive.Read(link,canceled);state.scanning=false;
                if(canceled()) return;
                if(catalog.error.empty()) {
                    if(!state.driveMode) stop();
                    remote=std::move(catalog);state.driveMode=true;state.driveLink=drive_music::Canonical(link);
                    drive.Save(remote);library();state.scanned=true;
                } else state.error=catalog.error;
            };
            if(hasCommand&&command.action==Action::Mode) {
                const bool mode=command.index!=0;
                if(mode!=state.driveMode) {stop();state.driveMode=mode;state.error.clear();library();}
                if(mode&&!remote.source&&drive.Ready()) connect(drive_music::ParseLink(state.driveLink));
            } else if(hasCommand&&(command.action==Action::Connect||(command.action==Action::Scan&&state.driveMode))) {
                const auto link=drive_music::ParseLink(command.action==Action::Connect?command.path:state.driveLink);
                if(!link) state.error="Paste a Google Drive folder or audio-file sharing link.";
                else if(!drive.Ready()) state.error="Drive music is available when the Android app can open its music cache.";
                else connect(link);
            } else if(hasCommand&&command.action==Action::Scan) {
                state.scanning=true;state.error.clear();Publish(state);
                if(!command.folders.empty()) sourceConfigured=true;
                auto folders=command.folders.empty()?state.folders:command.folders;
#ifdef __ANDROID__
                if(!sourceConfigured&&!defaultResolved) folders.clear();
#endif
                if(folders.size()==1&&folders.front()==state.appFolder) mkdir(state.appFolder.c_str(),0700);
                local=ReadLibrary(folders);state.folders=std::move(folders);library();
                state.scanning=false;state.scanned=true;
                if(!local.readable&&!state.folders.empty()) state.error="Can't read this folder. Choose a folder this app can access.";
            } else if(hasCommand) {
                if(command.action==Action::Stop) stop();
                else if(!state.available) state.error="Audio playback is available in the Android app.";
                else {
                    int index=command.index;
                    if(command.action==Action::Play&&!command.path.empty()) {
                        index=-1;
                        for(std::size_t i=0;i<state.tracks.size();++i) if(state.tracks[i].path==command.path) {index=static_cast<int>(i);break;}
                    }
                    const int count=static_cast<int>(state.tracks.size());
                    if(command.action==Action::Previous) index=state.current<0?0:(state.current-1+count)%std::max(1,count);
                    if(command.action==Action::Next) index=state.current<0?0:(state.current+1)%std::max(1,count);
                    if(command.action==Action::Toggle&&state.current<0) index=0;
                    const bool selecting=command.action==Action::Play||command.action==Action::Previous||command.action==Action::Next||(command.action==Action::Toggle&&state.current<0);
                    if(selecting&&index>=0&&index<count&&!canceled()) {
                        stop();state.current=index;state.loading=true;state.download=0.f;state.error.clear();Publish(state);
                        const auto& track=state.tracks[index];auto path=track.path;
                        if(track.remote) {
                            auto last=std::chrono::steady_clock::now();
                            path=drive.Fetch({track.remote,track.title},state.error,canceled,[&](float progress) {
                                const auto now=std::chrono::steady_clock::now();
                                if(now-last>std::chrono::milliseconds(120)||progress>=1.f) {state.download=progress;Publish(state);last=now;}
                            });
                        }
                        const bool ok=!path.empty()&&!canceled()&&player.Play(path);
                        state.loading=false;
                        if(ok) player.Poll(state);
                        else {stop();if(canceled()) state.error.clear();else if(state.error.empty()) state.error="Couldn't play this track. Try another audio file.";}
                    } else if(command.action==Action::Toggle&&state.current>=0) {
                        if(!player.Toggle(state.playing,state.length>0&&state.position>=state.length-.1f)) state.error="Playback couldn't be changed.";
                        else state.error.clear();
                    } else if(command.action==Action::Seek&&state.current>=0) {
                        if(!player.Seek(std::clamp(command.position,0.f,state.length))) state.error="This track couldn't seek to that position.";
                    }
                }
            }
            if(state.current>=0&&!player.Poll(state)) {stop();state.error="Playback stopped. Choose a track to try again.";}
            if(hasCommand||state.current>=0||wasAvailable!=state.available) Publish(state);
        }
    }
public:
    ~Service() {
        {std::lock_guard<std::mutex> lock(mutex_);stopping_=true;++revision_;}
        wake_.notify_all();if(worker_.joinable()) worker_.join();
    }
    void Start(PlatformVM* vm=nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(vm) vm_=vm;
        if(!worker_.joinable()) {commands_.push_back({Action::Scan});worker_=std::thread([this]{Run();});}
    }
    std::shared_ptr<const Snapshot> Read() {std::lock_guard<std::mutex> lock(mutex_);return published_;}
    void Queue(Command command) {
        std::lock_guard<std::mutex> lock(mutex_);if(stopping_) return;
        if(command.action==Action::Seek||command.action==Action::Scan||command.action==Action::Connect||command.action==Action::Mode)
            commands_.erase(std::remove_if(commands_.begin(),commands_.end(),[&](const Command& other){return other.action==command.action;}),commands_.end());
        if(commands_.size()>=32) return;
        if(command.action!=Action::Seek&&command.action!=Action::Toggle) ++revision_;
        command.revision=revision_;
        commands_.push_back(std::move(command));
        wake_.notify_one();
    }
};
inline Service& Engine() {static Service service;return service;}
inline void Init(PlatformVM* vm) {Engine().Start(vm);}
inline std::shared_ptr<const Snapshot> GetSnapshot() {return Engine().Read();}
inline void Scan() {Engine().Queue({Action::Scan});}
inline void ScanFolders(std::vector<std::string> folders) {Engine().Queue({Action::Scan,-1,0.f,std::move(folders)});}
inline void Play(int index) {
    const auto state=GetSnapshot();
    if(index>=0&&index<static_cast<int>(state->tracks.size())) Engine().Queue({Action::Play,index,0.f,{},state->tracks[index].path});
}
inline void Stop() {Engine().Queue({Action::Stop});}
inline void Toggle() {Engine().Queue({Action::Toggle});}
inline void Next() {Engine().Queue({Action::Next});}
inline void Previous() {Engine().Queue({Action::Previous});}
inline void Seek(float seconds) {Engine().Queue({Action::Seek,-1,seconds});}
inline void Update() {}
inline void SetDriveMode(bool drive) {Engine().Queue({Action::Mode,drive?1:0});}
inline void ConnectDrive(const std::string& link) {Engine().Queue({Action::Connect,-1,0.f,{},link});}
inline music_ui::TextDialog sourceDialog,driveDialog,searchDialog;
inline bool Matches(const Track& track,const std::string& filter) {
    const auto lower=[](std::string value) {for(char& c:value) if(c>='A'&&c<='Z') c+=32;return value;};
    return filter.empty()||lower(track.title).find(lower(filter))!=std::string::npos;
}
inline bool TapButton(const char* id,ImVec2 size) {
    const bool pressed=ImGui::InvisibleButton(id,size);
    const bool accepted=pressed&&(!custom::touch::Applies()||custom::touch::Tap());
    if(accepted) ui_sounds::Play(ui_sounds::Cue::Click);
    return accepted;
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
    const auto inset=ImVec2(8.f*c::scale,0);
    draw->PushClipRect(at+inset,at+size-inset,true);
    draw->AddText(at+ImVec2(ImMax(inset.x,(size.x-measured.x)*.5f),(size.y-measured.y)*.5f),ImGui::GetColorU32(selected?c::text::text:c::text::text_hov),label);
    draw->PopClipRect();
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
    if(modeButton("##music_local_mode","Local",!state.driveMode)) SetDriveMode(false);
    ImGui::SameLine(0,8.f*scale);
    if(modeButton("##music_drive_mode","Drive",state.driveMode)) SetDriveMode(true);
    if(state.driveMode) {
        if(TextButton("##drive_connect",state.driveLink.empty()?"Connect a Google Drive folder":"Change Drive folder",{width,30.f*scale},true)) {
            driveDialog.Open(state.driveLink);ImGui::OpenPopup("Drive source");
        }
        if(state.driveLink.empty()) {
            ImGui::PushTextWrapPos();ImGui::TextColored(c::text::text_hov,"Share your music folder: Anyone with the link / Viewer. Then paste its link here.");ImGui::PopTextWrapPos();
        }
        if(music_ui::Dialog("Drive source",driveDialog,"Paste your shared Drive link","Connect")==music_ui::DialogResult::Apply) ConnectDrive(driveDialog.text);
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
    char progress[48];std::snprintf(progress,sizeof(progress),"Caching track... %.0f%%",state.download*100.f);
    const char* detail=state.loading?(state.driveMode?progress:"Opening track..."):state.scanning?"Finding your music...":state.playing?(state.driveMode?"Playing from Drive":"Playing locally"):state.current>=0?"Paused":"Your songs, ready when you are";
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
        if(released) {Seek(seekPreview);ui_sounds::Play(ui_sounds::Cue::Slider);}
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
    if(IconButton("##music_toggle",state.loading?ICON_FA_STOP:state.playing?ICON_FA_PAUSE:ICON_FA_PLAY,{buttonHeight,buttonHeight},true,state.loading?!state.available:disabled)) {
        if(state.loading) Stop();else Toggle();
    }
    ImGui::SameLine(0,buttonGap);
    if(IconButton("##music_next",ICON_FA_STEP_FORWARD,{buttonHeight,buttonHeight},false,disabled)) Next();
    static bool libraryOpen=false;
    if(compact) ImGui::SameLine(0,10.f*scale);
    const auto row=ImGui::GetCursorScreenPos();
    const float refreshWidth=28.f*scale,sourceWidth=28.f*scale;
    const float libraryWidth=ImMax(40.f,ImGui::GetContentRegionAvail().x-sourceWidth-refreshWidth-14.f*scale);
    if(TapButton("##music_library_toggle",{libraryWidth,30.f*scale})) libraryOpen=!libraryOpen;
    char libraryLabel[64];std::snprintf(libraryLabel,sizeof(libraryLabel),"%s Library %zu",libraryOpen?"-":"+",state.tracks.size());
    draw->AddText(ImGui::GetFont(),13.f*scale,row+ImVec2(0,7.f*scale),ImGui::GetColorU32(c::text::text),libraryLabel);
    ImGui::SameLine(0,6.f*scale);
    if(IconButton("##music_source",ICON_FA_FOLDER_OPEN,{sourceWidth,30.f*scale})) {
        if(state.driveMode) {driveDialog.Open(state.driveLink);ImGui::OpenPopup("Drive source");}
        else {const auto path=state.folders.empty()?state.appFolder:state.folders.front();sourceDialog.Open(path);ImGui::OpenPopup("Music source");}
    }
    if(ImGui::IsItemHovered()) ImGui::SetTooltip(state.driveMode?"Shared Drive folder":"Music folder");
    ImGui::SameLine(0,6.f*scale);
    if(IconButton("##music_refresh",ICON_FA_SYNC,{refreshWidth,30.f*scale},false,state.scanning)) Scan();
    if(!state.error.empty()) {ImGui::PushTextWrapPos();ImGui::TextColored(ImVec4(.98f,.61f,.67f,1.f),"%s",state.error.c_str());ImGui::PopTextWrapPos();}
    if(libraryOpen) {
        if(TextButton("##music_search",searchDialog.text[0]?searchDialog.text:"Search your songs",{width,28.f*scale})) {searchDialog.Open(searchDialog.text);ImGui::OpenPopup("Find a song");}
        music_ui::Dialog("Find a song",searchDialog,"Search this library","Search");
        const float listHeight=ImClamp(ImGui::GetContentRegionAvail().y,80.f*scale,220.f*scale);
        ImGui::BeginChild("##music_library",{0,listHeight},false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
        if(state.tracks.empty()) {
            ImGui::TextColored(c::text::text,"No songs here yet");
            ImGui::PushTextWrapPos();ImGui::TextColored(c::text::text_hov,state.driveMode?"Upload audio files to your shared Drive folder, then refresh.":"Add audio files to your selected folder, then refresh.");ImGui::PopTextWrapPos();
        }
        bool found=false;
        for(std::size_t i=0;i<state.tracks.size();++i) {
            if(!Matches(state.tracks[i],searchDialog.text)) continue;
            found=true;
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
        if(!found&&!state.tracks.empty()) ImGui::TextColored(c::text::text_hov,"No matching songs");
        ImGui::EndChild();
        if(state.truncated) ImGui::TextColored(c::text::text_hov,"Showing the first 256 tracks");
    }
    const auto sourceResult=music_ui::Dialog("Music source",sourceDialog,"Choose your music folder","Use folder","App folder",state.appFolder.c_str(),"Phone library");
    if(sourceResult==music_ui::DialogResult::Apply) ScanFolders({sourceDialog.text});
    else if(sourceResult==music_ui::DialogResult::Shortcut) ScanFolders({"/storage/emulated/0/Music","/storage/emulated/0/Download"});
    ImGui::PopStyleVar();
}
}
