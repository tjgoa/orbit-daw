#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <iterator>
#include <string>
#include <vector>

namespace orbit::recording
{
// Message-thread performance timeline. Whole-bar quantization is explicit;
// authored patterns and existing arrangement data are never overwritten here.
class PatternClipTake
{
public:
    struct Target {std::string id;int initialSlot=0;};
    struct Segment {std::string id;int slot=0,startBar=0,lengthBars=0;};
    std::vector<Target> targets;
    std::map<std::string,std::map<int,int>> launches;
    bool active=false;
    int startBar=0,endBar=512,finishedBar=0;
    double secondsPerBar=2,lastSeconds=0;
    bool contains(const std::string& id)const
    {return std::any_of(targets.begin(),targets.end(),[&](const auto& t){return t.id==id;});}
    int boundary(double seconds)const
    {if(!std::isfinite(seconds) || !std::isfinite(secondsPerBar) || secondsPerBar<=0)return startBar;return std::clamp(int(std::ceil(std::clamp(seconds/secondsPerBar,0.,double(endBar))-1.e-8)),0,endBar);}
    void start(std::vector<Target> selected,int first,int end,double barSeconds,double seconds)
    {
        clear();targets=std::move(selected);startBar=first;endBar=end;secondsPerBar=barSeconds;lastSeconds=seconds;finishedBar=first;
        active=!targets.empty() && first>=0 && end>first && std::isfinite(barSeconds) && barSeconds>0;
        if(!active){clear();return;}
        for(const auto& t:targets)launches[t.id][first]=t.initialSlot;
    }
    bool select(const std::string& id,int slot,double seconds)
    {
        if(!active || !contains(id) || slot<0 || slot>=64 || !std::isfinite(seconds))return false;
        const int bar=std::max(startBar,boundary(seconds));if(bar>=endBar)return false;
        auto& events=launches[id];events[bar]=slot;
        // A bank click followed by a slot click in the same bar replaces the
        // queued choice. Re-selecting the previous pattern cancels that switch.
        for(auto i=events.begin();i!=events.end();)
            if(i!=events.begin() && std::prev(i)->second==i->second)i=events.erase(i);else ++i;
        return true;
    }
    std::vector<Segment> segments(int exclusiveEnd)const
    {
        std::vector<Segment> result;exclusiveEnd=std::clamp(exclusiveEnd,startBar,endBar);
        for(const auto& [id,events]:launches)for(auto i=events.begin();i!=events.end();++i)
        {
            const auto next=std::next(i);const int end=next==events.end()?exclusiveEnd:std::min(exclusiveEnd,next->first);
            if(end>i->first)result.push_back({id,i->second,i->first,end-i->first});
        }
        return result;
    }
    int visibleEnd(double seconds)const
    {return std::clamp(std::max(startBar+1,boundary(seconds)),startBar,endBar);}
    void finish(double seconds){if(active){finishedBar=std::clamp(boundary(seconds),startBar,endBar);active=false;}}
    void clear(){*this=PatternClipTake{};}
};
}
