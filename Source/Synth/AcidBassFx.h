#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace orbit::synth
{
// Original post-synth processor. No AudioRealism implementation is used.
// A constant oversampling latency keeps dry/wet and automation phase-aligned.
class AcidBassFx
{
public:
    struct Controls { float drive=35,distort=0,lowCut=30,tone=12000,output=0,mode=0,triangleFold=0; };
    AcidBassFx():oversampling(1,2,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true){prepare(44100,512);}
    void prepare(double sampleRate,int blockSize)
    {
        rate=juce::jlimit(8000.,384000.,std::isfinite(sampleRate)?sampleRate:44100.);
        capacity=juce::jlimit(1,65536,blockSize);oversampling.initProcessing(size_t(capacity));
        latency=juce::roundToInt(oversampling.getLatencyInSamples());
        work.setSize(1,capacity);dry.setSize(1,capacity);delay.resize(size_t(latency+1));
        dcCoefficient=std::exp(-juce::MathConstants<double>::twoPi*5/(rate*4));
        smooth=1-std::exp(-1/(.005*rate));fastSmooth=1-std::exp(-1/(.005*rate*4));reset();
    }
    void reset()
    {oversampling.reset();std::fill(delay.begin(),delay.end(),0);position=0;hp=lp=dcInput=dcOutput=0;first=true;}
    int latencySamples()const{return latency;}
    void setControls(Controls c)
    {
        const double gain=std::pow(10.,bounded(c.drive,0,100,35)*.36/20);
        target={gain,bounded(c.distort,0,100,0)*.01,
            1-std::exp(-juce::MathConstants<double>::twoPi*bounded(c.lowCut,20,1500,30)/(rate*4)),
            1-std::exp(-juce::MathConstants<double>::twoPi*juce::jmin(bounded(c.tone,500,20000,12000),rate*.45)/(rate*4)),
            std::pow(10.,bounded(c.output,-24,12,0)/20),bounded(c.mode,0,1,0),std::pow(gain,-.25),bounded(c.triangleFold,0,1,0)};
        if(first){current=target;first=false;}
    }
    void process(float* samples,int count)
    {
        juce::ScopedNoDenormals noDenormals;
        for(int offset=0;offset<count;offset+=capacity)
        {
            const auto n=juce::jmin(capacity,count-offset);
            for(int i=0;i<n;++i)
            {
                const float input=std::isfinite(samples[offset+i])?juce::jlimit(-32.f,32.f,samples[offset+i]):0;
                work.setSample(0,i,input);delay[position]=input;
                dry.setSample(0,i,delay[(position+1)%delay.size()]);position=(position+1)%delay.size();
            }
            auto block=juce::dsp::AudioBlock<float>(work).getSubBlock(0,size_t(n));auto up=oversampling.processSamplesUp(block);
            for(size_t i=0;i<up.getNumSamples();++i)
            {
                for(auto j:{0,2,3,5,6,7})current[size_t(j)]+=(target[size_t(j)]-current[size_t(j)])*fastSmooth;
                const double input=up.getSample(0,int(i));
                hp+=current[2]*(input-hp);const double driven=(input-hp)*current[0];
                // Preserve the old Soft/Sine interpolation, with a separate
                // smoothed selector so saved 0..1 FX Mode curves retain meaning.
                const double classic=std::tanh(driven)*(1-current[5])+std::sin(driven)*current[5];
                const double turn=(driven+1)*.25;
                const double triangle=4*std::abs(turn-std::floor(turn+.5))-1;
                const double shaped=(classic*(1-current[7])+triangle*current[7])*current[6];
                const double dc=shaped-dcInput+dcCoefficient*dcOutput;
                dcInput=shaped;dcOutput=dc;
                lp+=current[3]*(dc-lp);up.setSample(0,int(i),float(lp));
            }
            oversampling.processSamplesDown(block);
            for(int i=0;i<n;++i)
            {
                for(auto j:{1,4})current[size_t(j)]+=(target[size_t(j)]-current[size_t(j)])*smooth;
                samples[offset+i]=float((dry.getSample(0,i)*(1-current[1])+work.getSample(0,i)*current[1])*current[4]);
            }
        }
    }
private:
    static double bounded(float v,double lo,double hi,double fallback){return juce::jlimit(lo,hi,std::isfinite(v)?double(v):fallback);}
    juce::dsp::Oversampling<float> oversampling;juce::AudioBuffer<float> work,dry;
    std::vector<float> delay;size_t position=0;int capacity=512,latency=0;
    double rate=44100,smooth=0,fastSmooth=0,hp=0,lp=0,dcInput=0,dcOutput=0,dcCoefficient=0;
    std::array<double,8> current{},target{};bool first=true;
};
}
