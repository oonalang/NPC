#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../Fonts/Icon.h"
#include <string>
#include <vector>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

namespace music_player {
struct Track {
    std::string path;
    std::string title;
};
inline std::vector<Track> tracks;
inline int current = -1;
inline bool playing = false;
inline bool available = false;
inline bool scanned = false;
inline int scannedCount = 0;
inline float position = 0.f;
inline float length = 0.f;
inline char folder[192] = "/storage/emulated/0/Android/data/com.garena.game.codm/files/Music";

inline bool AudioFile(const std::string &name)
{
    const std::size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot + 1);
    for (char &c : ext)
    {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return ext == "mp3" || ext == "m4a" || ext == "aac" || ext == "ogg" || ext == "wav" || ext == "flac";
}

inline bool EnsureFolder(const char *path)
{
    struct stat info {};
    if (stat(path, &info) == 0) return S_ISDIR(info.st_mode);
    return mkdir(path, 0755) == 0;
}

inline void ScanFolder(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == nullptr) return;
    while (dirent *entry = readdir(dir))
    {
        if (entry->d_name[0] == '.') continue;
        const std::string name(entry->d_name);
        if (!AudioFile(name)) continue;
        Track track;
        track.path = std::string(path) + "/" + name;
        track.title = name.substr(0, name.find_last_of('.'));
        tracks.push_back(std::move(track));
        if (tracks.size() >= 64) break;
    }
    closedir(dir);
}

inline void Scan()
{
    EnsureFolder(folder);
    ScanFolder(folder);
    ScanFolder("/storage/emulated/0/Music");
    ScanFolder("/storage/emulated/0/Download");
    scanned = true;
    scannedCount = static_cast<int>(tracks.size());
    if (current >= scannedCount) current = -1;
}

inline std::string Clock(float seconds)
{
    if (!(seconds > 0.f)) seconds = 0.f;
    const int total = static_cast<int>(seconds);
    char text[16] = {};
    std::snprintf(text, sizeof(text), "%d:%02d", total / 60, total % 60);
    return text;
}

#ifdef __ANDROID__
inline JavaVM *vm = nullptr;
inline jobject player = nullptr;
inline jclass mediaClass = nullptr;
inline jmethodID ctorId = nullptr;
inline jmethodID sourceId = nullptr;
inline jmethodID prepareId = nullptr;
inline jmethodID startId = nullptr;
inline jmethodID pauseId = nullptr;
inline jmethodID stopId = nullptr;
inline jmethodID releaseId = nullptr;
inline jmethodID playingId = nullptr;
inline jmethodID durationId = nullptr;
inline jmethodID positionId = nullptr;

inline JNIEnv *Env()
{
    if (vm == nullptr) return nullptr;
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    }
    return env;
}

inline void Init(JavaVM *javaVm)
{
    if (vm == nullptr) vm = javaVm;
    if (mediaClass != nullptr) return;
    JNIEnv *env = Env();
    if (env == nullptr) return;
    jclass local = env->FindClass("android/media/MediaPlayer");
    if (local == nullptr)
    {
        env->ExceptionClear();
        return;
    }
    mediaClass = static_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);
    if (mediaClass == nullptr) return;
    ctorId = env->GetMethodID(mediaClass, "<init>", "()V");
    sourceId = env->GetMethodID(mediaClass, "setDataSource", "(Ljava/lang/String;)V");
    prepareId = env->GetMethodID(mediaClass, "prepare", "()V");
    startId = env->GetMethodID(mediaClass, "start", "()V");
    pauseId = env->GetMethodID(mediaClass, "pause", "()V");
    stopId = env->GetMethodID(mediaClass, "stop", "()V");
    releaseId = env->GetMethodID(mediaClass, "release", "()V");
    playingId = env->GetMethodID(mediaClass, "isPlaying", "()Z");
    durationId = env->GetMethodID(mediaClass, "getDuration", "()I");
    positionId = env->GetMethodID(mediaClass, "getCurrentPosition", "()I");
    available = ctorId != nullptr && sourceId != nullptr && prepareId != nullptr &&
        startId != nullptr && playingId != nullptr && durationId != nullptr && positionId != nullptr;
    if (env->ExceptionCheck()) env->ExceptionClear();
}

inline void Stop()
{
    JNIEnv *env = Env();
    if (env != nullptr && player != nullptr)
    {
        env->CallVoidMethod(player, stopId);
        env->CallVoidMethod(player, releaseId);
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteGlobalRef(player);
    }
    player = nullptr;
    playing = false;
    position = 0.f;
    length = 0.f;
}

inline void Play(int index)
{
    Init(vm);
    JNIEnv *env = Env();
    if (env == nullptr || !available || index < 0 || index >= static_cast<int>(tracks.size())) return;
    Stop();
    jobject local = env->NewObject(mediaClass, ctorId);
    if (local == nullptr || env->ExceptionCheck())
    {
        if (env->ExceptionCheck()) env->ExceptionClear();
        return;
    }
    jstring path = env->NewStringUTF(tracks[index].path.c_str());
    env->CallVoidMethod(local, sourceId, path);
    env->DeleteLocalRef(path);
    env->CallVoidMethod(local, prepareId);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        env->DeleteLocalRef(local);
        return;
    }
    env->CallVoidMethod(local, startId);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        env->DeleteLocalRef(local);
        return;
    }
    player = env->NewGlobalRef(local);
    env->DeleteLocalRef(local);
    current = index;
    playing = true;
}

inline void Toggle()
{
    JNIEnv *env = Env();
    if (env == nullptr || player == nullptr)
    {
        if (!tracks.empty()) Play(current < 0 ? 0 : current);
        return;
    }
    if (playing) env->CallVoidMethod(player, pauseId);
    else env->CallVoidMethod(player, startId);
    if (env->ExceptionCheck()) env->ExceptionClear();
    playing = !playing;
}

inline void Next()
{
    if (tracks.empty() || !available) return;
    Play((current + 1) % static_cast<int>(tracks.size()));
}

inline void Previous()
{
    if (tracks.empty() || !available) return;
    Play((current - 1 + static_cast<int>(tracks.size())) % static_cast<int>(tracks.size()));
}

inline void Update()
{
    JNIEnv *env = Env();
    if (env == nullptr || player == nullptr) return;
    playing = env->CallBooleanMethod(player, playingId) == JNI_TRUE;
    length = env->CallIntMethod(player, durationId) / 1000.f;
    position = env->CallIntMethod(player, positionId) / 1000.f;
    if (env->ExceptionCheck()) env->ExceptionClear();
}
#else
inline void Init(void *) {}
inline void Stop() {}
inline void Play(int) {}
inline void Toggle() {}
inline void Next() {}
inline void Previous() {}
inline void Update() {}
#endif

inline void ControlButton(const char *icon, bool enabled)
{
    ImFont *iconFont = F107 != nullptr ? F107 : ImGui::GetFont();
    ImGui::PushFont(iconFont);
    ImGui::BeginDisabled(!enabled);
    ImGui::Button(icon, ImVec2(42.f * c::scale, 34.f * c::scale));
    ImGui::EndDisabled();
    ImGui::PopFont();
}

inline void Draw()
{
    if (!scanned) Scan();
    Update();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f * c::scale, 6.f * c::scale));

    ImGui::PushFont(F107 != nullptr ? F107 : ImGui::GetFont());
    const bool rescan = ImGui::Button(ICON_FA_SYNC, ImVec2(34.f * c::scale, 30.f * c::scale));
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::TextColored(c::text::text, "%d track%s", static_cast<int>(tracks.size()), tracks.size() == 1 ? "" : "s");
    if (rescan)
    {
        Stop();
        tracks.clear();
        current = -1;
        Scan();
    }

    const float listHeight = ImMax(72.f * c::scale, ImGui::GetContentRegionAvail().y - 88.f * c::scale);
    ImGui::BeginChild("##music_tracks", ImVec2(0.f, listHeight), false);
    if (!available && tracks.empty())
    {
        ImGui::TextColored(c::text::text, "No player available on this device.");
    }
    else if (tracks.empty())
    {
        ImGui::TextColored(c::text::text, "No audio found.");
        ImGui::TextColored(c::text::text_hov, "Drop .mp3 / .m4a files in:");
        ImGui::TextColored(c::accent, "%s", folder);
    }
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i)
    {
        const std::string label = tracks[i].title + "##track_" + std::to_string(i);
        if (ImGui::Selectable(label.c_str(), i == current, ImGuiSelectableFlags_None, ImVec2(0.f, 24.f * c::scale)))
        {
            Play(i);
        }
    }
    ImGui::EndChild();

    ImGui::PushFont(F107 != nullptr ? F107 : ImGui::GetFont());
    ImGui::BeginDisabled(tracks.empty());
    if (ImGui::Button(ICON_FA_STEP_BACKWARD, ImVec2(38.f * c::scale, 32.f * c::scale))) Previous();
    ImGui::SameLine();
    if (ImGui::Button(playing ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(40.f * c::scale, 32.f * c::scale))) Toggle();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_STEP_FORWARD, ImVec2(38.f * c::scale, 32.f * c::scale))) Next();
    ImGui::EndDisabled();
    ImGui::PopFont();

    ImGui::SameLine();
    const float ratio = length > 0.f ? ImClamp(position / length, 0.f, 1.f) : 0.f;
    char clock[40] = {};
    std::snprintf(clock, sizeof(clock), "%s / %s", Clock(position).c_str(), Clock(length).c_str());
    ImGui::TextColored(c::text::text_hov, "%s", clock);
    ImGui::SameLine();
    const float barWidth = ImMax(40.f, ImGui::GetContentRegionAvail().x);
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 barMin = ImGui::GetCursorScreenPos() + ImVec2(0.f, 10.f * c::scale);
    const ImVec2 barMax = barMin + ImVec2(barWidth, 8.f * c::scale);
    draw->AddRectFilled(barMin, barMax, IM_COL32(255, 255, 255, 26), 4.f * c::scale);
    if (ratio > 0.f)
    {
        draw->AddRectFilled(barMin, barMin + ImVec2(barWidth * ratio, 8.f * c::scale),
            ImGui::GetColorU32(c::accent), 4.f * c::scale);
    }
    if (current >= 0 && current < static_cast<int>(tracks.size()))
    {
        ImGui::TextColored(c::accent, "%s", tracks[current].title.c_str());
    }

    ImGui::PopStyleVar();
}
}
