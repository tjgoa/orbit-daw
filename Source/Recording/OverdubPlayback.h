#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace orbit::recording
{
struct LoopNote {int pitch=60,start=0,length=1,velocity=100; std::int64_t firstTick=0;};
// Runtime-only MIDI replay, ahead of generator arp/transpose. Publishing swaps
// prepared vectors; audio never waits for the writer or allocates note storage.
class OverdubPlayback
{
public:
    void publish(std::vector<LoopNote> notes,int length,int transportLength,double reference,bool newTake=false)
    {
        std::stable_sort(notes.begin(),notes.end(),[](const auto& a,const auto& b){return a.start<b.start;});
        {const juce::SpinLock::ScopedLockType guard(lock);data.swap(notes);patternTicks=length;
            transportTicks=transportLength;referenceTick=reference;enabled=true;if(newTake)++generation;}
    }
    void clear()
    {
        std::vector<LoopNote> retired;
        {const juce::SpinLock::ScopedLockType guard(lock);data.swap(retired);enabled=false;++generation;}
    }
    template<class Emit> void process(double phase,double duration,bool playing,Emit emit)
    {
        if(duration<=0)return;
        const juce::SpinLock::ScopedTryLockType guard(lock);
        // Note-offs remain serviceable even if a publication wins this block.
        if(!guard.isLocked())
        {if(initialised){clock+=lastDuration;drain(clock,clock+duration,emit);lastPhase=phase;lastDuration=duration;}return;}
        if(seenGeneration!=generation || !playing || !enabled)
        {
            for(size_t i=0;i<heldSize;++i)emit(false,held[i].pitch,0,0.0);
            heldSize=0;initialised=false;seenGeneration=generation;
        }
        if(!playing || !enabled || patternTicks<=0 || transportTicks<=0 || phase<0)return;
        if(!initialised)
        {clock=phase+std::round((referenceTick-phase)/transportTicks)*transportTicks;initialised=true;}
        else
        {
            auto delta=phase-lastPhase;
            if(delta< -transportTicks*0.5)delta+=transportTicks;
            if(std::abs(delta-lastDuration)>2.0)
            {for(size_t i=0;i<heldSize;++i)emit(false,held[i].pitch,0,0.0);heldSize=0;
                clock=phase+std::round((referenceTick-phase)/transportTicks)*transportTicks;}
            else clock+=delta;
        }
        lastPhase=phase;lastDuration=duration;
        const auto end=clock+duration;drain(clock,end,emit);
        for(auto cycle=std::int64_t(std::floor(clock/patternTicks));double(cycle)*patternTicks<end;++cycle)
        {
            const double origin=double(cycle)*patternTicks;
            auto it=std::lower_bound(data.begin(),data.end(),clock-origin,[](const auto& n,double tick){return n.start<tick-1.0e-7;});
            for(;it!=data.end() && origin+it->start<end-1.0e-7;++it)
            {
                const auto on=origin+it->start;if(on+1.0e-7<clock || on+1.0e-7<double(it->firstTick))continue;
                if(heldSize==held.size())continue;
                emit(true,it->pitch,it->velocity,(on-clock)/duration);
                const auto off=on+it->length;
                if(off<end-1.0e-7)emit(false,it->pitch,0,(off-clock)/duration);
                else held[heldSize++]={it->pitch,off};
            }
        }
    }
private:
    template<class Emit> void drain(double begin,double end,Emit emit)
    {
        for(size_t i=0;i<heldSize;)
            if(held[i].off<end-1.0e-7)
            {emit(false,held[i].pitch,0,juce::jlimit(0.0,1.0,(held[i].off-begin)/(end-begin)));held[i]=held[--heldSize];}
            else ++i;
    }
    juce::SpinLock lock;std::vector<LoopNote> data;
    int patternTicks=0,transportTicks=0;double referenceTick=0;
    bool enabled=false,initialised=false;std::uint64_t generation=0,seenGeneration=0;
    struct Held {int pitch;double off;};std::array<Held,4096> held{};size_t heldSize=0;
    double clock=0,lastPhase=0,lastDuration=0;
};
}
