#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
namespace orbit::synth
{
// Three original host-owned EQ bands after the synth FX. Coefficients are
// stack arrays; control changes never allocate on the audio thread.
class AcidBassEq
{
public:
    struct Controls{float enabled=0,lowFrequency=120,lowGain=0,midFrequency=1000,midGain=0,midQ=.7071f,highFrequency=6000,highGain=0,output=0;bool operator==(const Controls&)const=default;};
    void prepare(double sampleRate){rate=juce::jlimit(8000.,384000.,sampleRate);smooth=1-std::exp(-1/(.005*rate));reset();}
    void reset(){states={};blend=0;gain=1;first=true;haveControls=false;}
    void setControls(Controls c)
    {
        targetBlend=c.enabled>=.5f?1.:0.;targetGain=std::pow(10.,juce::jlimit(-12.f,12.f,c.output)/20.);
        if(haveControls && c==last)return;last=c;haveControls=true;
        using C=juce::dsp::IIR::ArrayCoefficients<double>;
        const auto frequency=[this](float f){return juce::jlimit(10.,rate*.45,double(f));};
        const auto gainFor=[](float db){return std::pow(10.,juce::jlimit(-18.f,18.f,db)/20.);};
        const std::array<std::array<double,6>,3> raw{C::makeLowShelf(rate,frequency(c.lowFrequency),.70710678,gainFor(c.lowGain)),C::makePeakFilter(rate,frequency(c.midFrequency),juce::jlimit(.1,8.,double(c.midQ)),gainFor(c.midGain)),C::makeHighShelf(rate,frequency(c.highFrequency),.70710678,gainFor(c.highGain))};
        for(size_t band=0;band<3;++band){const auto& a=raw[band];target[band]={a[0]/a[3],a[1]/a[3],a[2]/a[3],a[4]/a[3],a[5]/a[3]};}
        if(first){current=target;gain=targetGain;first=false;}
    }
    void process(juce::AudioBuffer<float>& buffer,int start,int count)
    {
        if(targetBlend==0 && blend<1.e-9){blend=0;states={};current=target;gain=targetGain;return;}
        juce::ScopedNoDenormals noDenormals;
        for(int i=0;i<count;++i)
        {
            blend+=smooth*(targetBlend-blend);gain+=smooth*(targetGain-gain);
            for(size_t b=0;b<3;++b)for(size_t c=0;c<5;++c)current[b][c]+=smooth*(target[b][c]-current[b][c]);
            for(int channel=0;channel<juce::jmin(2,buffer.getNumChannels());++channel)
            {
                const float dry=buffer.getSample(channel,start+i);double value=dry;
                for(size_t b=0;b<3;++b){const auto& c=current[b];auto& s=states[size_t(channel)][b];const double y=c[0]*value+s[0];s[0]=c[1]*value-c[3]*y+s[1];s[1]=c[2]*value-c[4]*y;value=y;}
                const double result=dry+blend*(value*gain-dry);buffer.setSample(channel,start+i,std::isfinite(result)?float(result):0.f);
            }
            for(int channel=2;channel<buffer.getNumChannels();++channel)buffer.setSample(channel,start+i,buffer.getSample(channel%2,start+i));
        }
    }
private:
    double rate=44100,smooth=.001,blend=0,targetBlend=0,gain=1,targetGain=1;bool first=true,haveControls=false;Controls last;
    std::array<std::array<double,5>,3> current{},target{};
    std::array<std::array<std::array<double,2>,3>,2> states{};
};
}
