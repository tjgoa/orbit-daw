#include "../Source/Mixing/PeakCompressor.h"
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
using Compressor=orbit::mixing::PeakCompressor;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static double db(double value){return 20*std::log10(std::max(value,1.0e-30));}
static double sine(Compressor::Controls controls,double frequency,double rate)
{
    Compressor p;p.prepare(rate);p.setControls(controls);double peak=0;
    for(int n=0;n<int(rate);++n)
    {
        const float x=float(.5*std::sin(n*frequency*6.283185307179586/rate));const auto out=p.process(x,-x);
        if(n>int(rate*.8))peak=std::max(peak,double(std::abs(out[0])));
    }
    return peak;
}
int main()
{
 try
 {
    for(double rate:{44100.,48000.,96000.,192000.})
    {
        Compressor p;Compressor::Controls c;p.prepare(rate);const int delay=p.latencySamples();
        c.ratio=1;p.setControls(c);
        for(int n=0;n<delay+300;++n)
        {
            const auto out=p.process(n==0?1.25f:0,n==0?-.5f:0);
            check(out[0]==(n==delay?1.25f:0) && out[1]==(n==delay?-.5f:0),"unity compressor changed audio/delay or clipped it");
        }
        c={};c.attackMs=.1;p.reset();p.setControls(c);std::array<float,2> out{};
        for(int n=0;n<int(rate);++n)out=p.process(.5f,-.5f);
        const auto expected=std::pow(10.0,(c.thresholdDb+(db(.5)-c.thresholdDb)/c.ratio)/20);
        check(std::abs(out[0]-expected)<.00001 && out[1]==-out[0],"peak transfer curve or anti-phase stereo link wrong");
        c.mix=0;p.reset();p.setControls(c);for(int n=0;n<int(rate*.1);++n)out=p.process(.5f,-.5f);
        check(out[0]==.5f && out[1]==-.5f,"dry blend was compressed or misaligned");
        c.mix=.5;p.reset();p.setControls(c);for(int n=0;n<int(rate);++n)out=p.process(.5f,-.5f);
        check(std::abs(out[0]-(expected+.5)*.5)<.00001,"parallel blend has wrong gain or timing");
        c={};c.attackMs=.1;p.reset();p.setControls(c);
        for(int n=0;n<int(rate*.1);++n)out=p.process(.5f,-.5f,0,0,true);
        check(out[0]==.5f,"silent external detector incorrectly followed audible input");
        for(int n=0;n<int(rate);++n)out=p.process(.01f,-.01f,.5f,-.5f,true);
        check(std::abs(out[0]-.01*expected/.5)<.00001,"external anti-phase detector or trigger isolation wrong");
        c={};c.attackMs=.1;c.ratio=100;c.thresholdDb=-12;p.reset();p.setControls(c);
        for(int n=0;n<int(rate*.1);++n){out=p.process(n%31==0?10.f:0,n%31==0?-5.f:0);check(std::abs(out[0])<=std::pow(10.,-12./20)+.00001,"instant infinite-ratio ceiling overshot");}
        c={};c.attackMs=.1;const double neutral=sine(c,1000,rate);c.detectorEqDb=12;const auto weighted=sine(c,1000,rate);
        const auto reduction=db(neutral/weighted);check(std::abs(reduction-9)<.3,"detector bell did not produce expected midrange weighting");
        c.ratio=1;const auto flat=sine(c,1000,rate);check(std::abs(flat-.5)<.0005,"detector EQ changed audible spectrum at unity ratio");
        std::cout<<"rate="<<rate<<" delay="<<delay<<" steady peak="<<expected<<" detector +12dB extra reduction="<<reduction<<"dB\n";
    }
    const double rate=48000;Compressor fast,slow;fast.prepare(rate);slow.prepare(rate);Compressor::Controls c;c.attackMs=.1;fast.setControls(c);c.attackMs=100;slow.setControls(c);
    float fastFirst=0,slowFirst=0;
    for(int n=0;n<2000;++n){auto a=fast.process(n>=800?.5f:0,0),b=slow.process(n>=800?.5f:0,0);if(n==800+fast.latencySamples()){fastFirst=a[0];slowFirst=b[0];}}
    check(fastFirst<slowFirst*.6f,"attack settings did not separate transient behavior");
    auto recovery=[](double ms){Compressor p;p.prepare(48000);Compressor::Controls controls;controls.attackMs=.1;controls.releaseMs=ms;p.setControls(controls);float result=0;for(int n=0;n<54000;++n){auto out=p.process(n<48000?.5f:.01f,0);if(n==48000+4800+p.latencySamples())result=out[0];}return result;};
    check(recovery(20)>recovery(500)*1.5,"release time did not change recovery");
    Compressor p;p.prepare(48000);c.inputDb=std::numeric_limits<double>::quiet_NaN();c.ratio=std::numeric_limits<double>::infinity();p.setControls(c);
    for(int n=0;n<2000;++n){auto out=p.process(n%2?std::numeric_limits<float>::quiet_NaN():std::numeric_limits<float>::infinity(),0);check(std::isfinite(out[0]),"nonfinite input/control contaminated compressor");}
    c={};c.detectorEqDb=12;c.attackMs=.1;p.prepare(44100);p.setControls(c);p.prepare(96000);p.setControls(c);for(int n=0;n<10000;++n)p.process(.1f,.1f);
    check(std::isfinite(p.reductionDb),"rate change failed to reconfigure detector");
    std::cout<<"PASS: original compressor transparent gain/no clipping, static dB curve, linked anti-phase stereo, aligned parallel mix, external isolation, infinite ratio, detector-only EQ, timing, finite input and rate reset.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
