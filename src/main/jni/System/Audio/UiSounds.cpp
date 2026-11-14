#include "UiSounds.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace ui_sounds {
std::vector<std::int16_t> Synthesize(Cue cue) {
    constexpr double pi=3.14159265358979323846;
    const double durations[]={.085,.125,.125,.055,.135,.23,.34,.2,.38};
    const auto index=static_cast<unsigned>(cue);
    if(index>=static_cast<unsigned>(Cue::Count)) return {};
    const double duration=durations[index];
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
struct Event {Cue cue;Clock::time_point time;};
struct Voice {
    SLObjectItf object=nullptr;
    SLPlayItf play=nullptr;
    SLAndroidSimpleBufferQueueItf queue=nullptr;
    SLVolumeItf volume=nullptr;
    Clock::time_point until{};
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
    void Play(const std::vector<std::int16_t>& pcm,float gain) {
        auto* selected=&voices[0];
        for(auto& voice:voices) if(voice.until<selected->until) selected=&voice;
        Stop(*selected);
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
    Clock::time_point last_{};
    std::thread thread_;
    void Run() {
        std::array<std::vector<std::int16_t>,static_cast<unsigned>(Cue::Count)> samples;
        for(unsigned i=0;i<samples.size();++i) samples[i]=Synthesize(static_cast<Cue>(i));
        Output output;const bool ready=output.Open();
        for(;;) {
            Event event{};bool have=false,settings=false,load=false;std::string root;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock,[&]{return stop_||settings_||!events_.empty();});
                if(stop_) break;
                settings=settings_;settings_=false;load=load_;load_=false;root=root_;
                if(!events_.empty()) {event=events_.front();events_.pop_front();have=true;}
            }
            if(load&&!root.empty()) {
                auto* file=std::fopen((root+"/.ui-audio").c_str(),"rb");
                int enabled=1;float gain=defaultGain;
                if(file) {
                    if(std::fscanf(file,"UIA1 %d %f",&enabled,&gain)==2&&!touched.load()&&std::isfinite(gain)) {
                        enabledState=enabled!=0;gainState=std::clamp(gain,0.f,1.f);
                    }
                    std::fclose(file);
                }
            }
            if(settings&&!root.empty()) {
                const auto path=root+"/.ui-audio",temporary=path+".part";
                auto* file=std::fopen(temporary.c_str(),"wb");
                if(file) {std::fprintf(file,"UIA1 %d %.4f\n",Enabled()?1:0,Gain());if(std::fclose(file)==0) std::rename(temporary.c_str(),path.c_str());}
            }
            if(!Enabled()||Gain()<=0) output.Stop();
            else if(ready&&have&&Clock::now()-event.time<std::chrono::milliseconds(180)) output.Play(samples[static_cast<unsigned>(event.cue)],Gain());
        }
    }
public:
    Engine():thread_([this]{Run();}) {}
    ~Engine() {{std::lock_guard<std::mutex> lock(mutex_);stop_=true;}wake_.notify_one();thread_.join();}
    void Play(Cue cue) {
        const auto now=Clock::now();std::lock_guard<std::mutex> lock(mutex_);
        if((cue<=Cue::Open&&now-last_<std::chrono::milliseconds(45))||events_.size()>=16) return;
        last_=now;events_.push_back({cue,now});wake_.notify_one();
    }
    void Settings(const std::string& root={}) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!root.empty()&&root!=root_) {root_=root;load_=true;}
        settings_=true;wake_.notify_one();
    }
};
Engine& Get() {static Engine engine;return engine;}
}
void Play(Cue cue) {if(Enabled()&&Gain()>0&&static_cast<unsigned>(cue)<static_cast<unsigned>(Cue::Count)) Get().Play(cue);}
void Configure(const std::string& folder) {Get().Settings(folder);}
bool Enabled() {return enabledState.load();}
float Gain() {return gainState.load();}
void SetEnabled(bool enabled) {touched=true;enabledState=enabled;Get().Settings();}
void SetGain(float gain) {if(std::isfinite(gain)) {touched=true;gainState=std::clamp(gain,0.f,1.f);Get().Settings();}}
}
#endif
