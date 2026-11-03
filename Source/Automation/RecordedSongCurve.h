#pragma once
#include "AutomationProjection.h"
#include <optional>

namespace orbit::automation
{
struct RecordLoop { double start=0,end=0; };
// Finish a native knob take using the actual user samples, without the native
// recorder's synthetic glide back to the old curve. Later authored events keep
// their positions/values; the recorded value holds until the next such event.
inline juce::Array<ProjectDocument::AutomationPoint> recordedSongCurve(
    const ProjectDocument::AutomationLane& original,
    const juce::Array<ProjectDocument::AutomationPoint>& samples,double finish,
    std::optional<RecordLoop> loop={})
{
    using Lane=ProjectDocument::AutomationLane;
    using Points=juce::Array<ProjectDocument::AutomationPoint>;
    constexpr double edge=.0001;
    if(samples.isEmpty())return original.points;
    Lane captured;captured.points=samples;
    Lane current=original;
    finish=std::max(finish,samples.getLast().position);
    if(!std::isfinite(finish))return original.points;
    double begin=samples.getFirst().position;
    if(loop && loop->end<=loop->start)loop.reset();
    if(loop && finish>loop->end+(loop->end-loop->start)*2)
        begin=std::max(begin,loop->start+(std::floor((finish-loop->start)/(loop->end-loop->start))-1)*(loop->end-loop->start));
    do
    {
        double stop=finish,start=begin,delta=0;
        if(loop)
        {
            const double length=loop->end-loop->start;
            if(begin>=loop->end)
            {
                const auto cycle=std::floor((begin-loop->start)/length);
                delta=cycle*length;start=begin-delta;stop=std::min(finish,loop->end+delta);
            }
            else stop=std::min(finish,loop->end);
        }
        const double end=stop-delta;
        std::map<double,float> next;
        for(auto p:current.points)if(p.position<start || p.position>=end)next[p.position]=p.value;
        if(start>0)next[std::max(0.,start-edge)]=current.points.isEmpty()?samples.getFirst().value:valueAt(current,std::max(0.,start-edge));
        next[start]=valueAt(captured,begin);
        for(auto p:samples)if(p.position>begin && p.position<stop)next[p.position-delta]=p.value;
        const float held=valueAt(captured,stop);
        const double tail=std::max(start,end-edge);
        next[tail]=held;
        // A loop boundary belongs to the material outside the loop, while the
        // next pass overwrites only its own recorded portion of the loop.
        if(loop && std::abs(end-loop->end)<1.e-8)
            next[end]=current.points.isEmpty()?original.points.isEmpty()?held:valueAt(original,end):valueAt(current,end);
        else
        {
            auto future=next.lower_bound(end);
            if(future!=next.end() && future->first>end)
            {const double at=future->first;next[end]=held;next[std::max(end,at-edge)]=held;}
        }
        current.points.clear();for(auto [at,value]:next)current.points.add({at,value});
        if(stop>=finish || !loop)break;
        begin=stop;
    }while(true);
    return current.points;
}
}
