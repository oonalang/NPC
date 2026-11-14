#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ui_sounds {
enum class Cue { Click,ToggleOn,ToggleOff,Slider,Open,Drop,Success,Error,Whoosh,Count };
inline constexpr unsigned sampleRate=48000;
inline constexpr float defaultGain=.28f;
std::vector<std::int16_t> Synthesize(Cue cue);
#ifdef __ANDROID__
void Play(Cue cue);
void Configure(const std::string& folder);
bool Enabled();
float Gain();
void SetEnabled(bool enabled);
void SetGain(float gain);
#else
inline bool enabled=true;
inline float gain=defaultGain;
inline void (*observer)(Cue)=nullptr;
inline void Play(Cue cue) {if(observer&&enabled&&gain>0&&static_cast<unsigned>(cue)<static_cast<unsigned>(Cue::Count)) observer(cue);}
inline void Configure(const std::string&) {}
inline bool Enabled() {return enabled;}
inline float Gain() {return gain;}
inline void SetEnabled(bool value) {enabled=value;}
inline void SetGain(float value) {gain=value;}
#endif
}
