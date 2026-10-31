#include "../Source/Synth/AcidBassVoice.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

static void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
static double phrase(bool accent,bool slide)
{
    orbit::synth::AcidBassVoice voice;voice.prepare(48000);double energy=0;
    voice.controller(119,(accent?1:0)|(slide?2:0));voice.noteOn(1,36,100);voice.finishEvents();
    for(int n=0;n<4800;++n){const auto sample=voice.sample();check(std::isfinite(sample) && std::abs(sample)<=1,"invalid audio");energy+=sample*sample;}
    voice.noteOff(1,36);voice.controller(119,0);voice.noteOn(1,48,100);voice.finishEvents();
    for(int n=0;n<4800;++n){const auto sample=voice.sample();energy+=sample*sample;}
    voice.panic();for(int n=0;n<256;++n)check(voice.sample()==0,"panic left audio running");
    return energy;
}
static void phaseRetrigger(double rate,bool square,bool sequenced,bool legato)
{
    using namespace orbit::synth;
    AcidBassVoice original,restarted;original.prepare(rate);restarted.prepare(rate);
    AcidParameters p;p.waveform=square?1.f:0.f;original.setParameters(p);p.phaseRetrigger=true;restarted.setParameters(p);
    if(sequenced){original.controller(119,legato?2:0);restarted.controller(119,legato?2:0);}
    original.noteOn(1,37,90);restarted.noteOn(1,37,90);original.finishEvents();restarted.finishEvents();
    for(int n=0;n<int(rate*.073);++n)check(original.sample()==restarted.sample(),"phase switch changed the initial idle-note attack");
    // Retrigger before the amplitude tail reaches idle. The default engine
    // therefore still has an advancing phase when the second attack arrives.
    if(sequenced || !legato){original.noteOff(1,37);restarted.noteOff(1,37);}
    if(sequenced){original.controller(119,0);restarted.controller(119,0);}
    original.noteOn(1,44,90);restarted.noteOn(1,44,90);original.finishEvents();restarted.finishEvents();
    double difference=0;
    for(int n=0;n<int(rate*.08);++n)
    {const float a=original.sample(),b=restarted.sample();check(std::isfinite(a) && std::isfinite(b),"retrigger produced invalid audio");difference+=std::abs(a-b);}
    check(legato?difference==0:difference>.01,"phase retrigger failed to distinguish fresh notes from slides/legato");
    restarted.panic();for(int n=0;n<256;++n)check(restarted.sample()==0,"phase-enabled Panic leaked audio");
}
int main()
{
    try
    {
        const double plain=phrase(false,false),accent=phrase(true,false),slide=phrase(false,true);
        check(plain>0.01 && std::abs(plain-accent)>0.01 && std::abs(plain-slide)>0.01,"accent/slide have no effect");
        for(double rate:{8000,44100,48000,96000,192000})
        {
            for(bool square:{false,true})for(bool sequenced:{false,true})for(bool legato:{false,true})phaseRetrigger(rate,square,sequenced,legato);
            orbit::synth::AcidBassVoice voice;voice.prepare(rate);
            for(int pitch:{0,36,127})
            {
                voice.panic();orbit::synth::AcidParameters p;p.cutoff=10000;p.resonance=100;p.waveform=1;p.tune=12;voice.setParameters(p);
                voice.noteOn(16,pitch,127);voice.finishEvents();
                for(int n=0;n<2000;++n)check(std::isfinite(voice.sample()),"endpoint/sample-rate instability");
            }
        }
        std::cout<<"PASS: MIT Open303 wrapper produces finite bounded audio, distinct accent/slide output, deterministic panic, full MIDI endpoints and five sample rates; phase restart changes fresh attacks while idle attacks/slides/live legato retain original output for both waveforms.\n";
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
