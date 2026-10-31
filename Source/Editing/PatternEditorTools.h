#pragma once
#include "PatternTools.h"

namespace orbit::editing
{
inline bool translatePhrase(std::vector<Note>& notes, int pitches, int ticks, int patternTicks)
{
    for (const auto& n:notes)
        if (int64_t(n.pitch)+pitches<0 || int64_t(n.pitch)+pitches>127 || int64_t(n.startTick)+ticks<0
            || int64_t(n.startTick)+ticks+n.lengthTicks>patternTicks) return false;
    for(auto& n:notes){n.pitch+=pitches;n.startTick+=ticks;}
    return true;
}
inline void flipPhrase(std::vector<Note>& notes, bool time, int patternTicks, bool wholePattern)
{
    if(notes.empty())return;
    int low=127,high=0,begin=patternTicks,end=0;
    for(const auto& n:notes){low=std::min(low,n.pitch);high=std::max(high,n.pitch);begin=std::min(begin,n.startTick);end=std::max(end,n.startTick+n.lengthTicks);}
    if(wholePattern){begin=0;end=patternTicks;}
    for(auto& n:notes)if(time)n.startTick=int(int64_t(begin)+end-n.startTick-n.lengthTicks);else n.pitch=low+high-n.pitch;
}
inline std::vector<Note> scissorPhrase(const std::vector<Note>& notes)
{
    std::vector<Note> result;
    for(auto n:notes)
    {
        if(n.lengthTicks<=1){result.push_back(n);continue;}
        const int tail=n.lengthTicks-n.lengthTicks/2;n.lengthTicks/=2;result.push_back(n);
        n.startTick+=n.lengthTicks;n.lengthTicks=tail;result.push_back(n);
    }
    return result;
}
inline std::vector<Note> rotatePhrase(const std::vector<Note>& notes,int ticks,int patternTicks)
{
    std::vector<Note> result;if(patternTicks<=0)return notes;
    for(auto n:notes)
    {
        n.startTick=int(((int64_t(n.startTick)+ticks)%patternTicks+patternTicks)%patternTicks);
        const int tail=int(std::max<int64_t>(0,int64_t(n.startTick)+n.lengthTicks-patternTicks));n.lengthTicks-=tail;
        result.push_back(n);if(tail>0){n.startTick=0;n.lengthTicks=tail;result.push_back(n);}
    }
    return result;
}
enum class LfoShape { sine, triangle, rising, falling, square };
inline std::vector<std::pair<int,float>> lfoPoints(int steps,LfoShape shape,double cycles,double phase,double amplitude,double centre)
{
    std::vector<std::pair<int,float>> result;const int count=std::clamp(steps,1,4096);
    for(int i=0;i<count;++i)
    {
        const int position=count==1?0:int(int64_t(i)*(steps-1)/(count-1));
        const double t=position/double(std::max(1,steps))*cycles+phase;
        const double p=t-std::floor(t);
        const double wave=shape==LfoShape::sine?std::sin(t*6.283185307179586)
            :shape==LfoShape::triangle?1-4*std::abs(p-0.5)
            :shape==LfoShape::rising?2*p-1:shape==LfoShape::falling?1-2*p:p<0.5?1:-1;
        result.emplace_back(position,float(std::clamp(centre+wave*amplitude,0.0,1.0)));
    }
    return result;
}
}
