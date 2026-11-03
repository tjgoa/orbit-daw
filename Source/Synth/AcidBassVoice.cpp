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
    // One oscillator/envelope feeds independent copies of the existing filter
    // and its output conditioning. No detune, extra oscillator or sequencer.
    struct Lane
    {
        rosic::TeeBeeFilter filter;
        rosic::EllipticQuarterBandFilter antiAlias;
        rosic::OnePoleFilter allpass,highpass;
        rosic::BiquadFilter notch;
    };
    std::array<Lane,7> lanes;
    std::array<double,8> weights{1,0,0,0,0,0,0,0},positions{},targetWeights{1,0,0,0,0,0,0,0},targetPositions{};
    double width=0,spread=0,spreadSmoothing=0;
    bool spreadRunning=false;
    void copyLane(Lane& lane)
    {lane.filter=filter;lane.antiAlias=antiAliasFilter;lane.allpass=allpass;lane.highpass=highpass2;lane.notch=notch;}
    void configureSpread()
    {
        const int count=params.filterSpreadEnabled?params.filterVoices:1;
        for(int n=0;n<8;++n)
        {
            if(n>0 && n<count && targetWeights[size_t(n)]==0 && weights[size_t(n)]<1.e-5)copyLane(lanes[size_t(n-1)]);
            targetWeights[size_t(n)]=n<count?1.0/count:0;
            targetPositions[size_t(n)]=count>1 && n<count?-1.0+2.0*n/(count-1):0;
        }
        if(params.filterSpreadEnabled)spreadRunning=true;
    }
    std::array<double,2> spreadSample()
    {
        if(idle)return {0,0};
        // Mirrors Open303's host-driven sample path; the upstream internal
        // sequencer stays Off. Pitch, envelope, accent and glide advance once.
        const double frequency=pitchSlewLimiter.getSample(oscFreq);
        oscillator.setFrequency(frequency*pitchWheelFactor);oscillator.calculateIncrement();
        const double envelope=mainEnv.getSample();
        const double normal=envScaler*(n1*rc1.getSample(envelope)-envOffset);
        const double accented=accentGain*n2*rc2.getSample(accentGain>0?envelope:0);
        const double frequencyCutoff=cutoff*std::pow(2.0,normal+accented);
        double amplitude=ampEnv.getSample();
        if(ampEnv.isNoteOn())amplitude+=(.45+4*accentGain)*envelope;
        amplitude=ampDeClicker.getSample(amplitude);
        width+=spreadSmoothing*((params.filterSpreadEnabled?params.stereoWidth*.01:0)-width);
        spread+=spreadSmoothing*(params.filterSpread*.02-spread); // +/- two octaves at 100%.
        double total=0,weightedPan=0;
        for(size_t n=0;n<8;++n)
        {
            weights[n]+=spreadSmoothing*(targetWeights[n]-weights[n]);
            positions[n]+=spreadSmoothing*(targetPositions[n]-positions[n]);total+=weights[n];
            weightedPan+=weights[n]*positions[n]*width;
            if(n==0 || weights[n]>1.e-7 || targetWeights[n]>0)
            {
                auto& f=n==0?filter:lanes[n-1].filter;
                f.setCutoff(frequencyCutoff*std::exp2(positions[n]*spread));
            }
        }
        std::array<double,8> output{};
        for(int over=0;over<oversampling;++over)
        {
            const double input=highpass1.getSample(-oscillator.getSample());
            output[0]=antiAliasFilter.getSample(filter.getSample(input));
            for(size_t n=1;n<8;++n)if(weights[n]>1.e-7 || targetWeights[n]>0)
                output[n]=lanes[n-1].antiAlias.getSample(lanes[n-1].filter.getSample(input));
        }
        output[0]=notch.getSample(highpass2.getSample(allpass.getSample(output[0])));
        std::array<double,2> result{};
        for(size_t n=0;n<8;++n)if(n==0 || weights[n]>1.e-7 || targetWeights[n]>0)
        {
            if(n>0){auto& lane=lanes[n-1];output[n]=lane.notch.getSample(lane.highpass.getSample(lane.allpass.getSample(output[n])));}
            const double value=output[n]*amplitude*ampScaler;
            const double pan=positions[n]*width;
            // Linear, normalized pan: identical filters sum to the original
            // level, including at full width; width zero is exactly mono.
            result[0]+=value*weights[n]*(1-pan)/std::max(1.e-12,total-weightedPan);
            result[1]+=value*weights[n]*(1+pan)/std::max(1.e-12,total+weightedPan);
        }
        if(!params.filterSpreadEnabled && weights[0]>1-1.e-7 && std::abs(positions[0])<1.e-7)
        {spreadRunning=false;weights={1,0,0,0,0,0,0,0};positions.fill(0);width=0;}
        return result;
    }
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
        weights={1,0,0,0,0,0,0,0};positions.fill(0);width=spread=0;spreadRunning=false;
        for(auto& lane:lanes)copyLane(lane);
        configureSpread();
    }
};
AcidBassVoice::AcidBassVoice():impl(std::make_unique<Impl>()){prepare(44100);}
AcidBassVoice::~AcidBassVoice()=default;
void AcidBassVoice::prepare(double rate)
{
    impl->rate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:44100;
    impl->setSampleRate(impl->rate);impl->setVolume(-12);impl->reset();setParameters(impl->params);
    impl->spreadSmoothing=1-std::exp(-1/(.01*impl->rate));
}
void AcidBassVoice::setParameters(AcidParameters p)
{
    auto bounded=[](float value,float low,float high,float fallback){return std::isfinite(value)?std::clamp(value,low,high):fallback;};
    p.tune=bounded(p.tune,-12,12,0);p.cutoff=bounded(p.cutoff,30,10000,650);p.resonance=bounded(p.resonance,0,100,65);
    p.envelope=bounded(p.envelope,0,100,55);p.decay=bounded(p.decay,30,3000,180);p.accent=bounded(p.accent,0,100,70);
    p.accentEnvelope=bounded(p.accentEnvelope,0,100,25);p.slide=bounded(p.slide,1,400,60);p.waveform=bounded(p.waveform,0,1,0);
    p.filterSpread=bounded(p.filterSpread,0,100,0);p.stereoWidth=bounded(p.stereoWidth,0,100,0);
    p.filterVoices=p.filterVoices<=2?2:p.filterVoices<=4?4:8;
    impl->params=p;impl->setTuning(440*std::exp2(p.tune/12));impl->setWaveform(p.waveform>=0.5f?1:0);
    impl->setEnvMod(p.envelope);impl->setDecay(p.decay);impl->setAccent(p.accent);impl->setSlideTime(p.slide);
    impl->setAccentDecay(30*std::pow(30.0,p.accentEnvelope/100.0));
    impl->configureSpread();
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
        const bool wasIdle=i.idle;i.triggerNote(pitch,accent);
        if(wasIdle && i.spreadRunning)for(auto& lane:i.lanes)i.copyLane(lane);
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
bool AcidBassVoice::isFilterSpreadActive() const{return impl->spreadRunning;}
float AcidBassVoice::sample(){const auto stereo=sampleStereo();return (stereo[0]+stereo[1])*.5f;}
std::array<float,2> AcidBassVoice::sampleStereo()
{
    auto& i=*impl;
    if((i.smoothingCounter++&15)==0)
    {
        const auto coefficient=1-std::exp(-16/(0.008*i.rate));
        i.smoothedCutoff+=coefficient*(i.params.cutoff-i.smoothedCutoff);
        i.smoothedResonance+=coefficient*(i.params.resonance-i.smoothedResonance);
        i.setCutoff(i.smoothedCutoff);i.setResonance(i.smoothedResonance);
        if(i.spreadRunning)for(auto& lane:i.lanes)lane.filter.setResonance(i.smoothedResonance);
    }
    i.gain+=static_cast<float>((1-std::exp(-1/(0.003*i.rate)))*(i.targetGain-i.gain));
    const auto output=i.spreadRunning?i.spreadSample():[&]{const auto value=i.getSample();return std::array<double,2>{value,value};}();
    if(!std::isfinite(output[0]) || !std::isfinite(output[1])){panic();return {0,0};}
    return {static_cast<float>(std::tanh(output[0]*i.gain)),static_cast<float>(std::tanh(output[1]*i.gain))};
}
}
