#include "AcidBassVoice.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "ThirdParty/Open303/rosic_Open303.h"

namespace orbit::synth
{
struct AcidBassVoice::Impl final : rosic::Open303
{
    using rosic::Open303::triggerNote;
    using rosic::Open303::slideToNote;
    using rosic::Open303::idle;
    using rosic::Open303::currentNote;
    std::array<unsigned short,2048> held{};
    std::array<bool,2048> sustained{};
    std::array<std::uint64_t,2048> order{};
    std::uint64_t serial=0;
    int key=-1,flags=0,smoothingCounter=0;
    bool gate=false,sequence=false,sourceSlide=false,marker=false,hadNote=false,pedal=false;
    double rate=44100,smoothedCutoff=650,smoothedResonance=65;
    float gain=1,targetGain=1;
    AcidParameters params;
    void restartOscillator(){oscillator.resetPhase();}
    int top() const
    {
        int result=-1;std::uint64_t newest=0;
        for(int i=0;i<2048;++i)if((held[i]>0 || sustained[i]) && order[i]>=newest){newest=order[i];result=i;}
        return result;
    }
    void release(){ampEnv.noteOff();gate=false;sourceSlide=false;key=-1;}
    void retarget()
    {
        const auto next=top();
        if(next==key && next>=0)return;
        if(next>=0){slideToNote(next%128,false);key=next;gate=true;}
        else if(!sequence || !sourceSlide)release();
    }
    void reset()
    {
        held.fill(0);sustained.fill(false);order.fill(0);serial=0;key=-1;gate=false;sequence=false;sourceSlide=false;
        marker=false;hadNote=false;pedal=false;idle=true;currentNote=-1;flags=0;
        ampEnv.reset();rc1.reset();rc2.reset();gain=targetGain=1;
    }
};
AcidBassVoice::AcidBassVoice():impl(std::make_unique<Impl>()){prepare(44100);}
AcidBassVoice::~AcidBassVoice()=default;
void AcidBassVoice::prepare(double rate)
{
    impl->rate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:44100;
    impl->setSampleRate(impl->rate);impl->setVolume(-12);impl->reset();setParameters(impl->params);
}
void AcidBassVoice::setParameters(AcidParameters p)
{
    auto bounded=[](float value,float low,float high,float fallback){return std::isfinite(value)?std::clamp(value,low,high):fallback;};
    p.tune=bounded(p.tune,-12,12,0);p.cutoff=bounded(p.cutoff,30,10000,650);p.resonance=bounded(p.resonance,0,100,65);
    p.envelope=bounded(p.envelope,0,100,55);p.decay=bounded(p.decay,30,3000,180);p.accent=bounded(p.accent,0,100,70);
    p.accentEnvelope=bounded(p.accentEnvelope,0,100,25);p.slide=bounded(p.slide,1,400,60);p.waveform=bounded(p.waveform,0,1,0);
    impl->params=p;impl->setTuning(440*std::exp2(p.tune/12));impl->setWaveform(p.waveform>=0.5f?1:0);
    impl->setEnvMod(p.envelope);impl->setDecay(p.decay);impl->setAccent(p.accent);impl->setSlideTime(p.slide);
    impl->setAccentDecay(30*std::pow(30.0,p.accentEnvelope/100.0));
}
void AcidBassVoice::noteOn(int channel,int pitch,int velocity)
{
    if(channel<1 || channel>16 || pitch<0 || pitch>127 || velocity<=0 || velocity>127)return;
    auto& i=*impl;const int key=(channel-1)*128+pitch;
    const bool legato=i.gate && (i.sequence?i.sourceSlide:i.top()>=0);
    if(i.held[key]<65535)++i.held[key];i.sustained[key]=false;i.order[key]=++i.serial;
    const bool accent=i.sequence?(i.flags&1)!=0:velocity>=100;
    if(legato)i.slideToNote(pitch,accent);
    else
    {
        // Only restart the oscillator, leaving the filter/envelope history
        // alone. Slides and live legato retain a continuous oscillator phase.
        if(i.params.phaseRetrigger)i.restartOscillator();
        i.triggerNote(pitch,accent);
    }
    i.key=key;i.currentNote=pitch;i.gate=true;i.idle=false;i.hadNote=true;
    i.sourceSlide=i.sequence && (i.flags&2)!=0;
    i.targetGain=std::sqrt(velocity/127.0f);
}
void AcidBassVoice::noteOff(int channel,int pitch)
{
    if(channel<1 || channel>16 || pitch<0 || pitch>127)return;
    auto& i=*impl;const int key=(channel-1)*128+pitch;
    if(i.held[key]==0)return;
    --i.held[key];if(i.held[key]==0 && i.pedal)i.sustained[key]=true;
    if(key==i.key)i.retarget();
}
void AcidBassVoice::controller(int number,int value)
{
    auto& i=*impl;
    // Orbit-only per-step marker: Accent bit 0, outgoing Slide bit 1.
    if(number==119){i.sequence=true;i.flags=std::clamp(value,0,3);i.marker=true;}
    else if(number==64)
    {
        i.pedal=value>=64;if(!i.pedal){i.sustained.fill(false);i.retarget();}
    }
    else if(number==120 || number==123)panic();
}
void AcidBassVoice::pitchBend(float semitones){impl->setPitchBend(std::isfinite(semitones)?std::clamp(semitones,-48.0f,48.0f):0);}
void AcidBassVoice::finishEvents()
{
    auto& i=*impl;
    if(i.marker && !i.hadNote && i.top()<0)i.release();
    i.marker=false;i.hadNote=false;
}
void AcidBassVoice::panic(){impl->reset();}
bool AcidBassVoice::hasOutgoingSlide() const{return impl->gate && impl->sequence && impl->sourceSlide;}
float AcidBassVoice::sample()
{
    auto& i=*impl;
    if((i.smoothingCounter++&15)==0)
    {
        const auto coefficient=1-std::exp(-16/(0.008*i.rate));
        i.smoothedCutoff+=coefficient*(i.params.cutoff-i.smoothedCutoff);
        i.smoothedResonance+=coefficient*(i.params.resonance-i.smoothedResonance);
        i.setCutoff(i.smoothedCutoff);i.setResonance(i.smoothedResonance);
    }
    i.gain+=static_cast<float>((1-std::exp(-1/(0.003*i.rate)))*(i.targetGain-i.gain));
    const auto output=i.getSample()*i.gain;
    if(!std::isfinite(output)){panic();return 0;}
    return static_cast<float>(std::tanh(output));
}
}
