#pragma once
#include "../Automation/AutomationProjection.h"
#include <vector>
#include <set>
#include <optional>

namespace orbit::playlist
{
struct EventRange { double start=0, end=0; };
inline std::vector<EventRange> mergeEventRanges(std::vector<EventRange> ranges)
{
    std::sort(ranges.begin(),ranges.end(),[](auto a,auto b){return a.start<b.start;});
    std::vector<EventRange> result;
    for(auto range:ranges)
    {
        if(range.end<=range.start)continue;
        if(!result.empty() && range.start<=result.back().end)result.back().end=std::max(result.back().end,range.end);
        else result.push_back(range);
    }
    return result;
}
struct EventPaste
{
    EventRange destination;
    double delta=0;
    ProjectDocument::AutomationLane source;
};
// All sources are immutable snapshots. Cut first, then paste, including when a
// destination overlaps its source. Tiny edge guards preserve linear curves on
// either side of a block instead of drawing ramps across unrelated material.
inline juce::Array<ProjectDocument::AutomationPoint> editSongEventBlocks(
    const ProjectDocument::AutomationLane& original, float baseline,
    const std::vector<EventRange>& cuts, const std::vector<EventPaste>& pastes)
{
    constexpr double edge=0.0001; // sixteenth-note steps, 12.5 us at 120 BPM
    auto at=[&](const auto& lane,double t){return lane.points.isEmpty()?baseline:automation::valueAt(lane,t);};
    std::vector<EventRange> changed=cuts;
    for(const auto& paste:pastes)changed.push_back(paste.destination);
    changed=mergeEventRanges(std::move(changed));
    if(changed.empty())return original.points;
    auto inside=[&](double t){return std::any_of(changed.begin(),changed.end(),[&](auto r){return t>=r.start && t<r.end;});};
    std::map<double,float> points;
    for(const auto& point:original.points)if(!inside(point.position))points[point.position]=point.value;
    std::optional<float> terminalHold;
    for(auto span:changed)
    {
        if(span.start>0)points[std::max(0.0,span.start-edge)]=terminalHold.value_or(at(original,std::max(0.0,span.start-edge)));
        points[span.end]=at(original,span.end);
        std::set<double> boundaries{span.start,span.end};
        for(auto cut:cuts){if(cut.start>span.start && cut.start<span.end)boundaries.insert(cut.start);if(cut.end>span.start && cut.end<span.end)boundaries.insert(cut.end);}
        for(const auto& paste:pastes){if(paste.destination.start>span.start && paste.destination.start<span.end)boundaries.insert(paste.destination.start);if(paste.destination.end>span.start && paste.destination.end<span.end)boundaries.insert(paste.destination.end);}
        for(auto i=boundaries.begin(),j=std::next(i);j!=boundaries.end();++i,++j)
        {
            const double start=*i,end=*j,tail=std::max(start,end-std::min(edge,(end-start)*0.25));
            const EventPaste* active=nullptr;
            for(const auto& paste:pastes)if(start>=paste.destination.start && start<paste.destination.end)active=&paste;
            if(active)
            {
                points[start]=at(active->source,start-active->delta);points[tail]=at(active->source,tail-active->delta);
                for(const auto& point:active->source.points)
                {const double t=point.position+active->delta;if(t>start && t<tail)points[t]=point.value;}
            }
            else
            {
                const auto cut=std::find_if(cuts.begin(),cuts.end(),[&](auto r){return start>=r.start && start<r.end;});
                const auto value=cut!=cuts.end()?at(original,std::max(0.0,cut->start-edge)):at(original,start);
                points[start]=value;points[tail]=cut!=cuts.end()?value:at(original,tail);
            }
        }
        if(original.points.isEmpty() || original.points.getLast().position<span.end)
        {
            auto last=points.lower_bound(span.end);if(last!=points.begin())--last;
            terminalHold=last->second;points.erase(span.end);
        }
        else terminalHold.reset();
    }
    juce::Array<ProjectDocument::AutomationPoint> result;
    for(auto [time,value]:points)result.add({time,value});
    return result;
}
}
