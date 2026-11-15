#include "../src/main/jni/System/UI/StartupSounds.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

using ui_sounds::Cue;
using ui_sounds::Channel;
using startup_ui::AuthTiming;
struct Event {double time;Cue cue;Channel channel;bool stop;};
static std::vector<Event> events;
static double timeMs=0;
static void Capture() {
    ui_sounds::SetEnabled(true);ui_sounds::SetGain(1.f);
    ui_sounds::channelObserver=[](Cue cue,Channel channel){events.push_back({timeMs,cue,channel,false});};
    ui_sounds::stopObserver=[](Channel channel){events.push_back({timeMs,Cue::Count,channel,true});};
    events.clear();
}
static int Count(Cue cue) {return std::count_if(events.begin(),events.end(),[&](const Event& event){return !event.stop&&event.cue==cue;});}
static double First(Cue cue) {for(const auto& event:events) if(!event.stop&&event.cue==cue) return event.time;assert(false);return 0;}
static void Run(int fps,double response=.1,bool reduced=false) {
    Capture();startup_ui::LoginTransition auth;assert(auth.Start(0.,reduced));
    startup_ui::SoundObserver sound;startup_ui::LoadingSoundObserver loading;
    startup_ui::LoadingTimeline timeline;bool accepted=false,finished=false;
    for(int i=0;i<=fps*18;++i) {
        const double now=double(i)/fps;timeMs=now*1000.;
        if(!accepted&&now>=response) {auth.Resolve(now,true);accepted=true;}
        const auto frame=auth.At(now);startup_ui::PlaySounds(sound.Update(frame));
        if(frame.loadingVisible&&!timeline.active&&!finished) timeline.Start(now,7.);
        startup_ui::PlaySounds(loading.Update(timeline.active,timeline.Progress(now)));
        if(timeline.active&&timeline.Finish(now)) finished=true;
        auth.Presented(frame);
        if(frame.ready) auth.Reset();
    }
}
static std::vector<std::int16_t> Mix(double seconds) {
    std::array<std::vector<std::int16_t>,static_cast<unsigned>(Cue::Count)> clips;
    for(unsigned i=0;i<clips.size();++i) clips[i]=ui_sounds::Synthesize(static_cast<Cue>(i));
    std::vector<int> mix(static_cast<std::size_t>(seconds*ui_sounds::sampleRate));
    for(std::size_t i=0;i<events.size();++i) {
        const auto& event=events[i];if(event.stop) continue;
        const auto& pcm=clips[static_cast<unsigned>(event.cue)];
        const auto start=static_cast<std::size_t>(std::llround(event.time*ui_sounds::sampleRate/1000.));
        std::size_t end=std::min(mix.size(),start+pcm.size());
        for(std::size_t j=i+1;j<events.size();++j) if(events[j].stop&&events[j].channel==event.channel) {
            end=std::min(end,static_cast<std::size_t>(std::llround(events[j].time*ui_sounds::sampleRate/1000.)));break;
        }
        for(std::size_t n=start;n<end;++n) mix[n]+=pcm[n-start];
    }
    std::vector<std::int16_t> output;output.reserve(mix.size());int peak=0;
    for(int value:mix) {peak=std::max(peak,std::abs(value));assert(std::abs(value)<32767);output.push_back(static_cast<std::int16_t>(value));}
    assert(peak>10000);return output;
}
static void U32(std::ostream& out,std::uint32_t n,unsigned bytes=4) {for(unsigned i=0;i<bytes;++i) out.put(char(n>>(8*i)));}
static void Wave(const char* path,const std::vector<std::int16_t>& pcm) {
    std::ofstream out(path,std::ios::binary);out.write("RIFF",4);U32(out,36+pcm.size()*2);out.write("WAVEfmt ",8);U32(out,16);
    U32(out,1,2);U32(out,1,2);U32(out,ui_sounds::sampleRate);U32(out,ui_sounds::sampleRate*2);U32(out,2,2);U32(out,16,2);
    out.write("data",4);U32(out,pcm.size()*2);for(auto sample:pcm) U32(out,static_cast<std::uint16_t>(sample),2);
}
int main(int argc,char** argv) {
    using startup_ui::PlaySounds;
    const std::pair<Cue,double> starts[]{
        {Cue::AuthPress,0},{Cue::ButtonMorph,AuthTiming::labelEndMs},{Cue::Droplet,AuthTiming::spinEndMs},
        {Cue::Fall,AuthTiming::hoverEndMs},{Cue::Splash,AuthTiming::fallEndMs},{Cue::Wave,AuthTiming::splashEndMs},
        {Cue::Success,AuthTiming::waveEndMs},{Cue::SuckIn,AuthTiming::suckStartMs},{Cue::Singularity,AuthTiming::suckEndMs},
        {Cue::Explosion,AuthTiming::singularityEndMs},{Cue::LoadingReveal,AuthTiming::revealStartMs},
        {Cue::LoadingDone,AuthTiming::revealStartMs+7000.}
    };
    assert(std::fabs(ui_sounds::Duration(Cue::SuckIn)*1000.-AuthTiming::suckMs)<.001);
    assert(std::fabs(ui_sounds::Duration(Cue::Explosion)*1000.-AuthTiming::flashMs-AuthTiming::explosionMs)<.001);
    for(int fps:{30,60,120}) {
        Run(fps);
        for(const auto& expected:starts) {assert(Count(expected.first)==1);assert(std::fabs(First(expected.first)-expected.second)<=1000./fps+.02);}
        assert(Count(Cue::LoadingStep)==3);
        for(const auto& event:events) if(!event.stop&&event.time<AuthTiming::revealStartMs) assert(event.cue!=Cue::LoadingReveal&&event.cue!=Cue::LoadingStep&&event.cue!=Cue::LoadingDone);
        const auto mix=Mix(16.);
        const auto begin=static_cast<std::size_t>(std::ceil((AuthTiming::explosionEndMs+1000./fps)*ui_sounds::sampleRate/1000.));
        const auto end=static_cast<std::size_t>(AuthTiming::revealStartMs*ui_sounds::sampleRate/1000.);
        assert(std::all_of(mix.begin()+begin,mix.begin()+end,[](auto sample){return sample==0;}));
        std::printf("%d FPS: stage-aligned cues within one frame, one explosion, three loading checks, silent calm beat and unclipped mix\n",fps);
    }
    Run(60,4.);
    assert(Count(Cue::AuthPress)==1&&Count(Cue::ButtonMorph)==1&&Count(Cue::Explosion)==1);
    assert(First(Cue::Droplet)>=4000.&&std::fabs(First(Cue::SuckIn)-(AuthTiming::suckStartMs+1550.))<17.);
    Run(60,.1,true);
    assert(Count(Cue::Success)==1&&Count(Cue::LoadingReveal)==1&&Count(Cue::LoadingDone)==1);
    for(auto cue:{Cue::SuckIn,Cue::Explosion,Cue::Singularity,Cue::Splash,Cue::Wave,Cue::Fall}) assert(Count(cue)==0);
    Capture();startup_ui::LoginTransition auth;assert(auth.Start(0.));auth.Resolve(.1,true);startup_ui::SoundObserver sound;
    timeMs=6100;PlaySounds(sound.Update(auth.At(6.1)));assert(Count(Cue::Explosion)==1);
    timeMs=6200;PlaySounds(sound.Update(auth.At(6.2),false));assert(events.back().stop&&events.back().channel==Channel::Startup);
    timeMs=6500;PlaySounds(sound.Update(auth.At(6.5),true));assert(Count(Cue::Explosion)==1);
    Capture();ui_sounds::SetEnabled(false);startup_ui::SoundObserver muted;
    PlaySounds(muted.Update(auth.At(6.1)));assert(Count(Cue::Explosion)==0);ui_sounds::SetEnabled(true);
    Capture();startup_ui::LoadingSoundObserver progress;
    PlaySounds(progress.Update(true,.8f));assert(Count(Cue::LoadingStep)==0);
    PlaySounds(progress.Update(true,.8f));assert(Count(Cue::LoadingStep)==0);
    PlaySounds(progress.Update(true,1.f));assert(Count(Cue::LoadingDone)==1&&events.back().channel==Channel::UI);
    PlaySounds(progress.Update(false,1.f));assert(events.back().stop&&events.back().channel==Channel::Startup);
    Capture();startup_ui::LoadingSoundObserver boundary;
    PlaySounds(boundary.Update(true,0.f));PlaySounds(boundary.Update(true,.99999f));
    PlaySounds(boundary.Update(true,1.f));PlaySounds(boundary.Update(true,1.f));
    assert(Count(Cue::LoadingDone)==1);
    Capture();startup_ui::SoundObserver canceled;
    PlaySounds(canceled.Update(auth.At(5.3)));assert(Count(Cue::SuckIn)==1);
    auth.Reset();PlaySounds(canceled.Update(auth.At(5.4)));assert(events.back().stop);
    Run(60);if(argc>1) Wave(argv[1],Mix(16.));
    std::puts("Delayed auth, reduced motion, hidden/canceled effects, mute, skipped progress and completion-channel lifetime passed");
}
