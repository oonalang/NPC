#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ui_sounds {
enum class Cue { Click,ToggleOn,ToggleOff,Slider,Open,Drop,Success,Error,Whoosh,
                 AuthPress,ButtonMorph,Droplet,Fall,Splash,Wave,SuckIn,Singularity,Explosion,
                 LoadingReveal,LoadingStep,LoadingDone,Count };
enum class Channel { UI,Startup,Count };
inline constexpr unsigned sampleRate=48000;
inline constexpr float defaultGain=1.f;
std::vector<std::int16_t> Synthesize(Cue cue);
double Duration(Cue cue);
bool DecodeSettings(const char* text,bool& enabled,float& gain);
#ifdef __ANDROID__
void Play(Cue cue,Channel channel=Channel::UI);
void Stop(Channel channel);
void Configure(const std::string& folder);
bool Enabled();
float Gain();
void SetEnabled(bool enabled);
void SetGain(float gain);
#else
inline bool enabled=true;
inline float gain=defaultGain;
inline void (*observer)(Cue)=nullptr;
inline void (*channelObserver)(Cue,Channel)=nullptr;
inline void (*stopObserver)(Channel)=nullptr;
inline void Play(Cue cue,Channel channel=Channel::UI) {
    if(!enabled||gain<=0||static_cast<unsigned>(cue)>=static_cast<unsigned>(Cue::Count)||static_cast<unsigned>(channel)>=static_cast<unsigned>(Channel::Count)) return;
    if(observer) observer(cue);if(channelObserver) channelObserver(cue,channel);
}
inline void Stop(Channel channel) {if(stopObserver&&static_cast<unsigned>(channel)<static_cast<unsigned>(Channel::Count)) stopObserver(channel);}
inline void Configure(const std::string&) {}
inline bool Enabled() {return enabled;}
inline float Gain() {return gain;}
inline void SetEnabled(bool value) {enabled=value;}
inline void SetGain(float value) {gain=value;}
#endif
}
