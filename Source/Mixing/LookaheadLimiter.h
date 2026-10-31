#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace orbit::mixing
{
// Original linked-stereo sample-peak limiter. A monotonic queue finds the lowest
// future gain plus its permitted attack ramp. Each sample enters/leaves once;
// processing does not allocate. The current sample is always in the window, so
// attack smoothing cannot permit an overshoot. This is not a true-peak limiter.
class LookaheadLimiter
{
public:
    void prepare(double sampleRate)
    {
        rate=std::clamp(std::isfinite(sampleRate)?sampleRate:48000.0,8000.0,384000.0);
        delay=std::max(1,int(std::ceil(rate*0.005)));
        audio.assign(size_t(delay+1),{});queue.resize(size_t(delay+2));reset();
    }
    void reset()
    {
        std::fill(audio.begin(),audio.end(),std::array<double,2>{});
        sequence=0;head=0;count=0;gain=1;inputPeak=0;reductionDb=0;
    }
    int latencySamples() const {return delay;}
    double sampleRate() const {return rate;}
    void setControls(double inputDb,double releaseMs)
    {
        if(!std::isfinite(inputDb))inputDb=0;
        if(!std::isfinite(releaseMs))releaseMs=100;
        inputGain=std::pow(10.0,std::clamp(inputDb,-24.0,36.0)/20.0);
        release=std::exp(-1.0/(rate*std::clamp(releaseMs,5.0,2000.0)*0.001));
    }
    std::array<float,2> process(float left,float right)
    {
        if(audio.empty())return {};
        std::array<double,2> input{};
        for(int channel=0;channel<2;++channel)
        {
            const auto value=channel==0?left:right;
            input[size_t(channel)]=std::isfinite(value)?std::clamp(double(value)*inputGain,-1.0e12,1.0e12):0;
        }
        const double peak=std::max(std::abs(input[0]),std::abs(input[1]));
        inputPeak=peak;
        const double required=peak>1?1/peak:1;
        const auto oldest=sequence-delay;
        while(count>0 && queue[head].index<oldest){head=(head+1)%queue.size();--count;}
        const Node node{sequence,required+double(sequence)/delay};
        while(count>0 && queue[(head+count-1)%queue.size()].value>=node.value)--count;
        queue[(head+count)%queue.size()]=node;++count;
        audio[size_t(sequence%std::int64_t(audio.size()))]=input;
        std::array<float,2> output{};
        if(oldest>=0)
        {
            const auto target=std::clamp(queue[head].value-double(oldest)/delay,0.0,1.0);
            gain=std::min(target,1-(1-gain)*release);
            const auto delayed=audio[size_t(oldest%std::int64_t(audio.size()))];
            for(int channel=0;channel<2;++channel)
                output[size_t(channel)]=float(std::clamp(delayed[size_t(channel)]*gain,-1.0,1.0));
        }
        reductionDb=-20*std::log10(std::max(gain,1.0e-12));++sequence;return output;
    }
    double inputPeak=0,reductionDb=0;
private:
    struct Node {std::int64_t index=0;double value=1;};
    std::vector<std::array<double,2>> audio;std::vector<Node> queue;
    size_t head=0,count=0;std::int64_t sequence=0;
    int delay=240;double rate=48000,inputGain=1,release=0.9998,gain=1;
};
}
