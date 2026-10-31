#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace orbit::recording
{
struct Target { std::string id; int slot=0, lengthTicks=1024, startBar=0; bool song=false; };
struct Note { std::string id; int pitch=60, start=0, length=1, velocity=100; std::int64_t replayAfter=0; };
// Message-thread capture. Times are absolute monotonic musical ticks (256 PPQ),
// not transport positions, so UI queue delays and pattern wraps cannot reorder notes.
class MidiTake
{
public:
    std::vector<Target> targets;
    std::vector<Note> notes;
    bool active=false, full=false;
    int quantizeTicks=0; bool quantizeDurations=false;
    static constexpr size_t limit=262144;
    void start(std::vector<Target> selected,int grid=0,bool durations=false)
    { targets=std::move(selected);notes.clear();held.clear();pedals.clear();active=true;full=false;quantizeTicks=grid;quantizeDurations=durations; }
    void event(const std::string& id,const std::string& device,int channel,int pitch,int velocity,
               bool on,bool off,bool sustain,bool pedalDown,std::int64_t tick)
    {
        if(!active || std::none_of(targets.begin(),targets.end(),[&](const auto& t){return t.id==id;}))return;
        tick=std::max<std::int64_t>(0,tick);
        const Key key{id,device,channel,pitch};const Pedal pedal{id,device,channel};
        if(sustain)
        {
            pedals[pedal]=pedalDown;
            if(!pedalDown)for(auto i=held.begin();i!=held.end();)
            {
                if(std::get<0>(i->first)==id && std::get<1>(i->first)==device && std::get<2>(i->first)==channel && i->second.released)
                {emit(i->first,i->second,tick);i=held.erase(i);}else ++i;
            }
        }
        if(on)
        {
            if(auto i=held.find(key);i!=held.end()){emit(i->first,i->second,tick);held.erase(i);}
            if(held.size()+notes.size()<limit)held[key]={tick,velocity,false};else full=true;
        }
        if(off)if(auto i=held.find(key);i!=held.end())
        {if(pedals[pedal])i->second.released=true;else {emit(i->first,i->second,tick);held.erase(i);}}
    }
    void finish(std::int64_t tick)
    {if(!active)return;active=false;for(const auto& [key,n]:held)emit(key,n,tick);held.clear();pedals.clear();}
    void clear(){*this=MidiTake{};}
    std::vector<Note> preview(const std::string& id,std::int64_t tick) const
    {
        // Render held lengths without committing or mutating the captured take.
        MidiTake result;result.targets=targets;result.quantizeTicks=quantizeTicks;result.quantizeDurations=quantizeDurations;
        for(const auto& note:notes)if(note.id==id)result.append(note);
        if(active)for(const auto& [key,n]:held)if(std::get<0>(key)==id)result.emit(key,n,tick);
        return result.notes;
    }
private:
    using Key=std::tuple<std::string,std::string,int,int>;
    using Pedal=std::tuple<std::string,std::string,int>;
    struct Held {std::int64_t start;int velocity;bool released;};
    std::map<Key,Held> held;std::map<Pedal,bool> pedals;
    std::int64_t snap(std::int64_t value)const
    {return quantizeTicks>0?std::llround(double(value)/quantizeTicks)*quantizeTicks:value;}
    void emit(const Key& key,const Held& n,std::int64_t end)
    {
        const auto i=std::find_if(targets.begin(),targets.end(),[&](const auto& t){return t.id==std::get<0>(key);});
        if(i==targets.end() || i->lengthTicks<1)return;
        auto begin=snap(n.start);end=std::max(n.start+1,end);
        auto duration=quantizeDurations?std::max<std::int64_t>(1,snap(end-n.start)):std::max<std::int64_t>(1,end-n.start);
        if(i->song)
        {
            if(begin>=i->lengthTicks)return;
            append({i->id,std::get<3>(key),int(begin),int(std::min<std::int64_t>(duration,i->lengthTicks-begin)),n.velocity});
        }
        else
        {
            // One held voice spans at most a complete pattern. Repeated passes
            // remain separate performances, rather than merging authored notes.
            begin%=i->lengthTicks;duration=std::min<std::int64_t>(duration,i->lengthTicks);
            const auto first=std::min<std::int64_t>(duration,i->lengthTicks-begin);
            const auto replayAt=[&](std::int64_t onset)
            {return onset+std::max(n.start/i->lengthTicks+1,(end-onset+i->lengthTicks-1)/i->lengthTicks)*i->lengthTicks;};
            append({i->id,std::get<3>(key),int(begin),int(first),n.velocity,replayAt(begin)});
            if(first<duration)append({i->id,std::get<3>(key),0,int(duration-first),n.velocity,replayAt(0)});
        }
    }
    void append(Note n)
    {if(notes.size()<limit)notes.push_back(std::move(n));else full=true;}
};
}
