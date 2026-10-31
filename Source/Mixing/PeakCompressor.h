#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace orbit::mixing
{
// Original transparent, linked-stereo peak compressor. No waveshaping or audio
// clipping. The detector bell uses the published RBJ peaking-EQ mathematics:
// https://www.w3.org/TR/audio-eq-cookbook/ . No third-party DSP code is embedded.
class PeakCompressor
{
public:
    struct Controls
    {
        double inputDb=0, thresholdDb=-18, ratio=4, outputDb=0;
        double detectorEqDb=0, attackMs=10, releaseMs=150, mix=1;
    };
    void prepare(double sampleRate)
    {
        rate=bounded(sampleRate,8000,384000,48000);
        delay=std::max(1,int(std::ceil(rate*.005)));
        audio.resize(size_t(delay+1));queue.resize(size_t(delay+2));
        detectorEq=1000; // Force coefficient configuration at the new rate.
        controlStep=1-std::exp(-1/(.005*rate));reset();
    }
    void reset()
    {
        std::fill(audio.begin(),audio.end(),std::array<double,2>{});
        for(auto& filter:detector)filter.reset();
        sequence=0;head=0;count=0;gain=1;firstControls=true;
        inputPeak=outputPeak=reductionDb=0;
    }
    int latencySamples() const {return delay;}
    double sampleRate() const {return rate;}
    void setControls(Controls controls)
    {
        controls.inputDb=bounded(controls.inputDb,-24,36,0);
        controls.thresholdDb=bounded(controls.thresholdDb,-60,0,-18);
        controls.ratio=bounded(controls.ratio,1,100,4);
        controls.outputDb=bounded(controls.outputDb,-24,24,0);
        controls.detectorEqDb=bounded(controls.detectorEqDb,-12,12,0);
        controls.attackMs=bounded(controls.attackMs,.1,200,10);
        controls.releaseMs=bounded(controls.releaseMs,10,2000,150);
        controls.mix=bounded(controls.mix,0,1,1);
        targetInput=toGain(controls.inputDb);targetOutput=toGain(controls.outputDb);
        targetThreshold=controls.thresholdDb;targetInverseRatio=controls.ratio>=99.5?0:1/controls.ratio;
        targetMix=controls.mix;
        attack=controls.attackMs<=.5?0:std::exp(-1/(rate*controls.attackMs*.001));
        release=std::exp(-1/(rate*controls.releaseMs*.001));
        if(firstControls)
        {
            inputGain=targetInput;outputGain=targetOutput;threshold=targetThreshold;
            inverseRatio=targetInverseRatio;mix=targetMix;firstControls=false;
        }
        if(controls.detectorEqDb!=detectorEq)
        {
            detectorEq=controls.detectorEqDb;
            for(auto& filter:detector)filter.configure(rate,detectorEq);
        }
    }
    std::array<float,2> process(float left,float right,float sideLeft=0,float sideRight=0,bool external=false)
    {
        if(audio.empty())return {};
        inputGain+=(targetInput-inputGain)*controlStep;outputGain+=(targetOutput-outputGain)*controlStep;
        threshold+=(targetThreshold-threshold)*controlStep;inverseRatio+=(targetInverseRatio-inverseRatio)*controlStep;
        mix+=(targetMix-mix)*controlStep;
        const std::array<double,2> input{finiteAudio(left)*inputGain,finiteAudio(right)*inputGain};
        const std::array<double,2> trigger=external?std::array<double,2>{finiteAudio(sideLeft),finiteAudio(sideRight)}:input;
        double peak=0;
        for(size_t channel=0;channel<2;++channel)
            peak=std::max(peak,std::abs(detector[channel].process(trigger[channel])));
        const double level=20*std::log10(std::max(peak,1.0e-30));
        const double desiredReduction=std::max(0.0,level-threshold)*(1-inverseRatio);
        const double required=toGain(-desiredReduction);
        const auto oldest=sequence-delay;
        while(count>0 && queue[head].index<oldest){head=(head+1)%queue.size();--count;}
        const Node node{sequence,required+double(sequence)/delay};
        while(count>0 && queue[(head+count-1)%queue.size()].value>=node.value)--count;
        queue[(head+count)%queue.size()]=node;++count;
        audio[size_t(sequence%std::int64_t(audio.size()))]=input;
        std::array<float,2> result{};
        if(oldest>=0)
        {
            const double target=std::clamp(queue[head].value-double(oldest)/delay,0.0,1.0);
            const double coefficient=target<gain?attack:release;
            gain=target+(gain-target)*coefficient;
            const auto delayed=audio[size_t(oldest%std::int64_t(audio.size()))];
            const double multiplier=(1+mix*(gain-1))*outputGain;
            for(size_t channel=0;channel<2;++channel)result[channel]=float(delayed[channel]*multiplier);
        }
        inputPeak=std::max(std::abs(input[0]),std::abs(input[1]));
        outputPeak=std::max(std::abs(double(result[0])),std::abs(double(result[1])));
        reductionDb=-20*std::log10(std::max(gain,1.0e-30));++sequence;return result;
    }
    double inputPeak=0,outputPeak=0,reductionDb=0;
private:
    static double bounded(double value,double low,double high,double fallback)
    {return std::clamp(std::isfinite(value)?value:fallback,low,high);}
    static double finiteAudio(float sample)
    {return std::isfinite(sample)?std::clamp(double(sample),-1.0e12,1.0e12):0;}
    static double toGain(double db){return std::pow(10.0,db*.05);}
    struct DetectorBell
    {
        void reset(){x1=x2=y1=y2=0;}
        void configure(double rate,double gainDb)
        {
            const double a=std::pow(10.0,gainDb/40),w=6.283185307179586*1000/rate;
            const double alpha=std::sin(w)/(2*.7071067811865476),cosine=std::cos(w),normal=1+alpha/a;
            b0=(1+alpha*a)/normal;b1=-2*cosine/normal;b2=(1-alpha*a)/normal;
            a1=b1;a2=(1-alpha/a)/normal;
        }
        double process(double value)
        {
            const auto output=b0*value+b1*x1+b2*x2-a1*y1-a2*y2;
            x2=x1;x1=value;y2=y1;y1=std::abs(output)<1.0e-30?0:output;return y1;
        }
        double b0=1,b1=0,b2=0,a1=0,a2=0,x1=0,x2=0,y1=0,y2=0;
    };
    struct Node{std::int64_t index=0;double value=1;};
    std::vector<std::array<double,2>> audio;std::vector<Node> queue;
    std::array<DetectorBell,2> detector;
    size_t head=0,count=0;std::int64_t sequence=0;int delay=240;
    double rate=48000,controlStep=.004,attack=.99,release=.9998,gain=1,detectorEq=0;
    double inputGain=1,outputGain=1,threshold=-18,inverseRatio=.25,mix=1;
    double targetInput=1,targetOutput=1,targetThreshold=-18,targetInverseRatio=.25,targetMix=1;
    bool firstControls=true;
};
}
