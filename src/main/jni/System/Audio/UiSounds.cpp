#include "UiSounds.h"
#include "../UI/StartupState.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ui_sounds {
bool DecodeSettings(const char* text,bool& enabled,float& gain) {
    char version[5]{};int value=1;float volume=defaultGain;
    if(!text||std::sscanf(text,"%4s %d %f",version,&value,&volume)!=3||
       (std::strcmp(version,"UIA1")&&std::strcmp(version,"UIA2"))||!std::isfinite(volume)) return false;
    enabled=value!=0;
    gain=std::strcmp(version,"UIA1")==0&&std::fabs(volume-.28f)<.0001f?defaultGain:std::clamp(volume,0.f,1.f);
    return true;
}
namespace {
constexpr double pi=3.14159265358979323846;
double Smooth(double value) {value=std::clamp(value,0.,1.);return value*value*(3.-2.*value);}
double Envelope(double t,double length,double attack,double decay) {
    return t<0||t>=length?0.:Smooth(t/attack)*Smooth((length-t)/.02)*std::exp(-decay*t/length);
}
double Tone(double t,double frequency,double bend=0.) {return std::sin(2.*pi*(frequency*t+.5*bend*t*t));}
double Bubble(double t,double delay,double frequency,double length=.13) {
    t-=delay;return Envelope(t,length,.004,5.)*Tone(t,frequency,-frequency*2.5);
}
std::vector<std::int16_t> StartupClip(Cue cue,double duration) {
    std::vector<std::int16_t> pcm(static_cast<std::size_t>(std::llround(duration*sampleRate)));
    std::uint32_t random=0x19837u+static_cast<unsigned>(cue)*997u;
    double low=0,air=0,dc=0;
    for(std::size_t i=0;i<pcm.size();++i) {
        const double t=double(i)/sampleRate,u=t/duration;
        random^=random<<13;random^=random>>17;random^=random<<5;
        const double white=double(random)/2147483647.5-1.;
        low+=.018*(white-low);air+=.13*(white-air);
        const double noise=air-low;
        double signal=0;
        switch(cue) {
            case Cue::AuthPress:
                signal=.56*Envelope(t,duration,.003,8.)*(Tone(t,300.,-900.)+.12*Tone(t,680.));break;
            case Cue::ButtonMorph:
                signal=Envelope(t,duration,.012,3.)*(.21*Tone(t,430.,540.)+.35*noise)*(.8+.2*std::cos(2.*pi*u));break;
            case Cue::Droplet:
                signal=.33*Bubble(t,.015,740.,.18)+.16*Bubble(t,.12,1040.,.17);break;
            case Cue::Fall:
                signal=std::sin(pi*u)*std::sin(pi*u)*(.16*Tone(t,690.,-1300.)+.6*noise)*(.3+.7*u);break;
            case Cue::Splash:
                signal=.4*Bubble(t,0,470.,.16)+.23*Bubble(t,.045,890.,.14)+.15*Bubble(t,.12,650.,.18)+
                    .1*Bubble(t,.23,1120.,.16)+.7*Envelope(t,duration,.004,5.)*noise;break;
            case Cue::Wave:
                signal=std::sin(pi*u)*(.11*Tone(t,190.,270.)+.045*Tone(t,570.,180.)+.55*noise)*(.7+.3*std::sin(2.*pi*u*2.));break;
            case Cue::SuckIn: {
                const double swell=Smooth(u/.7)*Smooth((1.-u)/.12),swirl=.67+.33*std::sin(2.*pi*(3.*t+5.*t*t));
                signal=swell*(.22*Tone(t,135.,650.)+.06*Tone(t,310.,1300.)+.8*noise)*swirl;break;
            }
            case Cue::Singularity:
                signal=Envelope(t,duration,.004,4.)*(.22*Tone(t,740.,-2200.)+.12*Tone(t,125.));break;
            case Cue::Explosion: {
                const double impact=Envelope(t,.30,.004,4.5),rumble=Envelope(t,duration,.006,5.4);
                const double rush=Envelope(t,duration,.012,3.8),glints=.08*Bubble(t,.07,1250.,.28)+.055*Bubble(t,.20,940.,.34)+.04*Bubble(t,.43,710.,.4);
                signal=impact*(.35*Tone(t,92.,-155.)+.12*Tone(t,175.,-145.))+
                    rumble*(.17*Tone(t,54.,-9.)+.7*low)+rush*.85*noise+glints;break;
            }
            case Cue::LoadingReveal:
                signal=Envelope(t,duration,.035,2.5)*(.105*Tone(t,640.,240.)+.08*Tone(t,960.,180.)+.55*noise);break;
            case Cue::LoadingStep:
                signal=Envelope(t,duration,.003,5.)*(.22*Tone(t,920.)+.04*Tone(t,1380.));break;
            case Cue::LoadingDone:
                signal=.24*Bubble(t,0,740.,.32)+.22*Bubble(t,.075,940.,.32)+.25*Bubble(t,.15,1180.,.35);break;
            default: break;
        }
        dc+=.001*(signal-dc);
        signal=(signal-dc)*Smooth(t/.002)*Smooth((duration-t)/.015);
        pcm[i]=static_cast<std::int16_t>(std::clamp(signal,-.78,.78)*32767.);
    }
    if(!pcm.empty()) pcm.front()=pcm.back()=0;
    return pcm;
}
}
double Duration(Cue cue) {
    using T=startup_ui::AuthTiming;
    switch(cue) {
        case Cue::AuthPress: return T::labelFadeMs/1000.;
        case Cue::ButtonMorph: return T::buttonCollapseMs/1000.;
        case Cue::Droplet: return (T::dropletMorphMs+T::dropletHoverMs)/1000.;
        case Cue::Fall: return T::fallMs/1000.;
        case Cue::Splash: return T::splashMs/1000.;
        case Cue::Wave: return T::waveMs/1000.;
        case Cue::SuckIn: return T::suckMs/1000.;
        case Cue::Singularity: return T::singularityMs/1000.;
        case Cue::Explosion: return (T::flashMs+T::explosionMs)/1000.;
        case Cue::LoadingReveal: return T::revealMs/1000.;
        case Cue::LoadingStep: return .10;
        case Cue::LoadingDone: return .50;
        default: break;
    }
    constexpr double durations[]{.085,.125,.125,.055,.135,.23,.34,.2,.38};
    const auto index=static_cast<unsigned>(cue);return index<std::size(durations)?durations[index]:0.;
}
std::vector<std::int16_t> Synthesize(Cue cue) {
    constexpr double pi=3.14159265358979323846;
    const auto index=static_cast<unsigned>(cue);
    if(index>=static_cast<unsigned>(Cue::Count)) return {};
    const double duration=Duration(cue);
    if(cue>=Cue::AuthPress) return StartupClip(cue,duration);
    std::vector<std::int16_t> pcm(static_cast<std::size_t>(duration*sampleRate));
    std::uint32_t random=0x13579u+index*997u;
    double noise=0,phase=0,dc=0;
    for(std::size_t i=0;i<pcm.size();++i) {
        const double t=double(i)/sampleRate,u=t/duration;
        random^=random<<13;random^=random>>17;random^=random<<5;
        noise+=.03*(double(random)/2147483647.5-1.-noise);
        double frequency=640.,tone=.7,air=.15,decay=6.;
        switch(cue) {
            case Cue::Click: frequency=460.+520.*std::exp(-t*65.);break;
            case Cue::ToggleOn: frequency=510.+440.*(1.-std::exp(-t*32.));break;
            case Cue::ToggleOff: frequency=370.+480.*std::exp(-t*30.);break;
            case Cue::Slider: frequency=610.+180.*std::exp(-t*100.);tone=.4;break;
            case Cue::Open: frequency=360.+250.*std::sin(u*pi);tone=.26;air=1.8;decay=3.;break;
            case Cue::Drop: frequency=330.+850.*std::exp(-t*45.)+160.*std::exp(-std::pow((t-.085)*42.,2.));decay=6.;break;
            case Cue::Success: frequency=880.;tone=.43;decay=3.;break;
            case Cue::Error: frequency=320.-100.*u;tone=.42;decay=4.;break;
            case Cue::Whoosh: frequency=135.+100.*(1.-u);tone=.12;air=3.;decay=2.;break;
            default: break;
        }
        phase+=2.*pi*frequency/sampleRate;
        double signal=std::sin(phase)+.15*std::sin(phase*2.);
        if(cue==Cue::Success) {
            const double second=std::max(0.,t-.075);
            signal=.55*std::sin(2.*pi*880.*t)+.75*std::sin(2.*pi*1108.73*second)*std::min(1.,second/.012);
        }
        if(cue==Cue::Error) signal*=.65+.35*std::cos(t*2.*pi*14.);
        const double attack=std::min(1.,t/.004),tail=std::min(1.,(duration-t)/.016);
        const double envelope=attack*attack*tail*tail*std::exp(-u*decay);
        double sample=(tone*signal+air*noise)*envelope;
        dc+=.001*(sample-dc);sample=(sample-dc)*std::min(1.,(duration-t)/.008);
        pcm[i]=static_cast<std::int16_t>(std::clamp(sample,-.78,.78)*32767.);
    }
    pcm.front()=pcm.back()=0;
    return pcm;
}
}

#ifdef __ANDROID__
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>

namespace ui_sounds {
namespace {
std::atomic<bool> enabledState{true},touched{false};
std::atomic<float> gainState{defaultGain};
using Clock=std::chrono::steady_clock;
struct Event {Cue cue;Clock::time_point time;Channel channel;std::uint64_t epoch;};
struct Voice {
    SLObjectItf object=nullptr;
    SLPlayItf play=nullptr;
    SLAndroidSimpleBufferQueueItf queue=nullptr;
    SLVolumeItf volume=nullptr;
    Clock::time_point until{};
    Channel channel=Channel::UI;
};
class Output {
    SLObjectItf engineObject_=nullptr,mix_=nullptr;
    SLEngineItf engine_=nullptr;
public:
    std::array<Voice,4> voices{};
    bool Open() {
        if(slCreateEngine(&engineObject_,0,nullptr,0,nullptr,nullptr)!=SL_RESULT_SUCCESS) return false;
        if((*engineObject_)->Realize(engineObject_,SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS ||
           (*engineObject_)->GetInterface(engineObject_,SL_IID_ENGINE,&engine_)!=SL_RESULT_SUCCESS) return false;
        if((*engine_)->CreateOutputMix(engine_,&mix_,0,nullptr,nullptr)!=SL_RESULT_SUCCESS ||
           (*mix_)->Realize(mix_,SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS) return false;
        SLDataLocator_AndroidSimpleBufferQueue locator{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,1};
        SLDataFormat_PCM format{SL_DATAFORMAT_PCM,1,SL_SAMPLINGRATE_48,SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,SL_SPEAKER_FRONT_CENTER,SL_BYTEORDER_LITTLEENDIAN};
        SLDataSource source{&locator,&format};
        SLDataLocator_OutputMix output{SL_DATALOCATOR_OUTPUTMIX,mix_};SLDataSink sink{&output,nullptr};
        const SLInterfaceID ids[]={SL_IID_ANDROIDSIMPLEBUFFERQUEUE,SL_IID_VOLUME};
        const SLboolean required[]={SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
        for(auto& voice:voices) {
            if((*engine_)->CreateAudioPlayer(engine_,&voice.object,&source,&sink,2,ids,required)!=SL_RESULT_SUCCESS ||
               (*voice.object)->Realize(voice.object,SL_BOOLEAN_FALSE)!=SL_RESULT_SUCCESS ||
               (*voice.object)->GetInterface(voice.object,SL_IID_PLAY,&voice.play)!=SL_RESULT_SUCCESS ||
               (*voice.object)->GetInterface(voice.object,SL_IID_ANDROIDSIMPLEBUFFERQUEUE,&voice.queue)!=SL_RESULT_SUCCESS ||
               (*voice.object)->GetInterface(voice.object,SL_IID_VOLUME,&voice.volume)!=SL_RESULT_SUCCESS) return false;
        }
        return true;
    }
    void Stop(Voice& voice) {
        if(voice.play) (*voice.play)->SetPlayState(voice.play,SL_PLAYSTATE_STOPPED);
        if(voice.queue) (*voice.queue)->Clear(voice.queue);
        voice.until={};
    }
    void Stop() {for(auto& voice:voices) Stop(voice);}
    void Stop(Channel channel) {for(auto& voice:voices) if(voice.channel==channel) Stop(voice);}
    void Play(const std::vector<std::int16_t>& pcm,float gain,Channel channel) {
        auto* selected=&voices[0];
        for(auto& voice:voices) if(voice.until<selected->until) selected=&voice;
        Stop(*selected);selected->channel=channel;
        const auto level=static_cast<SLmillibel>(std::clamp(2000.f*std::log10(std::max(.00001f,gain)),float(SL_MILLIBEL_MIN),0.f));
        (*selected->volume)->SetVolumeLevel(selected->volume,level);
        if((*selected->queue)->Enqueue(selected->queue,pcm.data(),pcm.size()*sizeof(pcm[0]))==SL_RESULT_SUCCESS &&
           (*selected->play)->SetPlayState(selected->play,SL_PLAYSTATE_PLAYING)==SL_RESULT_SUCCESS)
            selected->until=Clock::now()+std::chrono::microseconds(pcm.size()*1000000/sampleRate);
    }
    ~Output() {
        Stop();for(auto& voice:voices) if(voice.object) (*voice.object)->Destroy(voice.object);
        if(mix_) (*mix_)->Destroy(mix_);if(engineObject_) (*engineObject_)->Destroy(engineObject_);
    }
};
class Engine {
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<Event> events_;
    std::string root_;
    bool stop_=false,settings_=false,load_=false;
    std::array<Clock::time_point,static_cast<unsigned>(Channel::Count)> last_{};
    std::array<std::atomic<std::uint64_t>,static_cast<unsigned>(Channel::Count)> epochs_{{0,0}};
    unsigned stops_=0;
    std::thread thread_;
    void Run() {
        std::array<std::vector<std::int16_t>,static_cast<unsigned>(Cue::Count)> samples;
        for(unsigned i=0;i<samples.size();++i) samples[i]=Synthesize(static_cast<Cue>(i));
        Output output;const bool ready=output.Open();
        for(;;) {
            Event event{};bool have=false,settings=false,load=false;unsigned stops=0;std::string root;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock,[&]{return stop_||settings_||stops_||!events_.empty();});
                if(stop_) break;
                settings=settings_;settings_=false;load=load_;load_=false;root=root_;
                stops=stops_;stops_=0;
                if(!events_.empty()) {event=events_.front();events_.pop_front();have=true;}
            }
            if(load&&!root.empty()) {
                auto* file=std::fopen((root+"/.ui-audio").c_str(),"rb");
                bool enabled=true;float gain=defaultGain;
                if(file) {
                    char text[80]{};
                    if(std::fgets(text,sizeof(text),file)&&DecodeSettings(text,enabled,gain)&&!touched.load()) {
                        enabledState=enabled;gainState=gain;
                    }
                    std::fclose(file);
                }
            }
            if(settings&&!root.empty()) {
                const auto path=root+"/.ui-audio",temporary=path+".part";
                auto* file=std::fopen(temporary.c_str(),"wb");
                if(file) {std::fprintf(file,"UIA2 %d %.4f\n",Enabled()?1:0,Gain());if(std::fclose(file)==0) std::rename(temporary.c_str(),path.c_str());}
            }
            for(unsigned i=0;i<static_cast<unsigned>(Channel::Count);++i) if(stops&(1u<<i)) output.Stop(static_cast<Channel>(i));
            if(!Enabled()||Gain()<=0) output.Stop();
            else if(ready&&have&&event.epoch==epochs_[static_cast<unsigned>(event.channel)].load()&&Clock::now()-event.time<std::chrono::milliseconds(180))
                output.Play(samples[static_cast<unsigned>(event.cue)],Gain(),event.channel);
        }
    }
public:
    Engine():thread_([this]{Run();}) {}
    ~Engine() {{std::lock_guard<std::mutex> lock(mutex_);stop_=true;}wake_.notify_one();thread_.join();}
    void Play(Cue cue,Channel channel) {
        const auto now=Clock::now();const auto index=static_cast<unsigned>(channel);
        std::lock_guard<std::mutex> lock(mutex_);
        if((channel==Channel::UI&&cue<=Cue::Open&&now-last_[index]<std::chrono::milliseconds(45))||events_.size()>=16) return;
        last_[index]=now;events_.push_back({cue,now,channel,epochs_[index].load()});wake_.notify_one();
    }
    void Stop(Channel channel) {
        const auto index=static_cast<unsigned>(channel);std::lock_guard<std::mutex> lock(mutex_);
        ++epochs_[index];stops_|=1u<<index;
        events_.erase(std::remove_if(events_.begin(),events_.end(),[&](const Event& event){return event.channel==channel;}),events_.end());
        wake_.notify_one();
    }
    void Settings(const std::string& root={}) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!root.empty()&&root!=root_) {root_=root;load_=true;}
        settings_=true;wake_.notify_one();
    }
};
Engine& Get() {static Engine engine;return engine;}
}
void Play(Cue cue,Channel channel) {if(Enabled()&&Gain()>0&&static_cast<unsigned>(cue)<static_cast<unsigned>(Cue::Count)&&static_cast<unsigned>(channel)<static_cast<unsigned>(Channel::Count)) Get().Play(cue,channel);}
void Stop(Channel channel) {if(static_cast<unsigned>(channel)<static_cast<unsigned>(Channel::Count)) Get().Stop(channel);}
void Configure(const std::string& folder) {Get().Settings(folder);}
bool Enabled() {return enabledState.load();}
float Gain() {return gainState.load();}
void SetEnabled(bool enabled) {touched=true;enabledState=enabled;Get().Settings();}
void SetGain(float gain) {if(std::isfinite(gain)) {touched=true;gainState=std::clamp(gain,0.f,1.f);Get().Settings();}}
}
#endif
