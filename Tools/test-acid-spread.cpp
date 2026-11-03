// Shared-oscillator filter-bank stability, compatibility and resonance probe.
#include "../Source/Synth/AcidBassVoice.h"
#include "../Source/Synth/ThirdParty/Open303/rosic_Open303.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <chrono>
using namespace orbit::synth;
static void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void compare(double rate,int count,bool square)
{
    AcidBassVoice mono,bank;mono.prepare(rate);bank.prepare(rate);AcidParameters p;p.waveform=square?1.f:0.f;p.phaseRetrigger=true;
    mono.setParameters(p);p.filterSpreadEnabled=true;p.filterVoices=count;p.stereoWidth=100;bank.setParameters(p);
    mono.controller(119,3);bank.controller(119,3);mono.noteOn(1,37,100);bank.noteOn(1,37,100);
    double error=0,stereo=0;
    for(int n=0;n<int(rate*.1);++n)
    {
        if(n==int(rate*.05)){mono.noteOff(1,37);bank.noteOff(1,37);mono.controller(119,0);bank.controller(119,0);mono.noteOn(1,45,90);bank.noteOn(1,45,90);}
        mono.finishEvents();bank.finishEvents();const auto a=mono.sampleStereo(),b=bank.sampleStereo();
        error=std::max(error,double(std::abs(a[0]-b[0])));stereo=std::max(stereo,double(std::abs(b[0]-b[1])));
    }
    require(error<2.e-5,"zero spread changed the shared oscillator/envelope or level");require(stereo<2.e-5,"zero spread unexpectedly widened identical filters");
    p.filterSpread=70;p.stereoWidth=100;bank.setParameters(p);double difference=0;
    for(int n=0;n<int(rate*.15);++n){auto a=bank.sampleStereo();require(std::isfinite(a[0]) && std::isfinite(a[1]) && std::abs(a[0])<=1 && std::abs(a[1])<=1,"unstable spread audio");difference+=std::abs(a[0]-a[1]);}
    require(difference>.001,"filter spread and width failed to create stereo audio");
    p.stereoWidth=0;bank.setParameters(p);
    for(int n=0;n<int(rate*.3);++n)bank.sampleStereo();
    for(int n=0;n<100;++n){auto a=bank.sampleStereo();require(std::abs(a[0]-a[1])<2.e-6,"zero width is not mono");}
    p.filterSpreadEnabled=false;bank.setParameters(p);
    for(int n=0;n<int(rate*.3);++n)bank.sampleStereo();
    require(!bank.isFilterSpreadActive(),"Off never returned to original single-filter path");
    bank.panic();for(int n=0;n<64;++n){auto a=bank.sampleStereo();require(a[0]==0 && a[1]==0,"Panic leaked a spread voice");}
}
int main(int argc,char**)
{
    try
    {
        if(argc>1)
        {
            for(int count:{1,2,4,8})
            {
                AcidBassVoice voice;voice.prepare(48000);AcidParameters p;p.filterSpreadEnabled=count>1;p.filterVoices=count;p.filterSpread=70;p.stereoWidth=100;voice.setParameters(p);voice.noteOn(1,36,100);voice.finishEvents();
                for(int n=0;n<48000;++n)voice.sampleStereo();
                const auto start=std::chrono::steady_clock::now();double total=0;
                for(int n=0;n<480000;++n){const auto s=voice.sampleStereo();total+=s[0]+s[1];}
                std::cout<<count<<" filters: "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<" ms for 10 s of DSP; checksum "<<total<<'\n';
            }
            return 0;
        }
        for(double rate:{8000.,44100.,48000.,96000.,192000.})for(int count:{2,4,8})for(bool square:{false,true})compare(rate,count,square);
        AcidBassVoice v;v.prepare(48000);v.noteOn(1,24,127);v.finishEvents();
        for(int n=0;n<160;++n)
        {
            AcidParameters p;p.filterSpreadEnabled=n%4!=0;p.filterVoices=2<<(n%3);p.filterSpread=n%2?100:0;p.stereoWidth=n%2?100:0;p.resonance=100;p.cutoff=n%2?10000:30;p.phaseRetrigger=true;
            v.setParameters(p);if(n%8==0){v.controller(119,n%4);v.noteOn(1,24+n%60,127);v.finishEvents();}
            for(int k=0;k<317;++k){auto a=v.sampleStereo();require(std::isfinite(a[0]) && std::isfinite(a[1]) && std::abs(a[0])<=1 && std::abs(a[1])<=1,"switching extreme values unstable");}
        }
        // Isolate feedback from the oscillator and amplifier: excitation then
        // zero input. This measures ringing, not an amplitude-envelope tail.
        for(double resonance:{0.,90.,100.})
        {
            rosic::TeeBeeFilter f;f.setSampleRate(192000);f.setCutoff(1500);f.setResonance(resonance);double early=0,late=0;
            for(int n=0;n<192000;++n){const double x=f.getSample(n==0?.1:0);require(std::isfinite(x),"isolated filter unstable");if(n<48000)early+=x*x;if(n>=144000)late+=x*x;}
            std::cout<<resonance<<"% resonance: initial RMS "<<sqrt(early/48000)<<", late zero-input RMS "<<sqrt(late/48000)<<'\n';
        }
        std::cout<<"PASS: zero-spread compatibility, stereo separation, zero-width mono, Off fast path, panic, live count/enable changes and finite endpoints at five rates, saw and square.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
