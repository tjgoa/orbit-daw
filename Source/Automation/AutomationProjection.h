#pragma once
#include "../Project/ProjectDocument.h"
#include <map>

namespace orbit::automation
{
inline float valueAt(const ProjectDocument::AutomationLane& lane, double step)
{
    if (lane.points.isEmpty()) return 0.5f;
    const auto* before = &lane.points.getReference(0); const auto* after = &lane.points.getReference(lane.points.size() - 1);
    for (const auto& point : lane.points)
    {
        if (point.position <= step) before = &point;
        if (point.position >= step) { after = &point; break; }
    }
    if (step <= lane.points.getFirst().position) return lane.points.getFirst().value;
    if (step >= lane.points.getLast().position) return lane.points.getLast().value;
    return before->value + float((step - before->position) / (after->position - before->position)) * (after->value - before->value);
}
// Every repeated cycle resets at its boundary. A tiny pre-boundary sample keeps
// linear engine curves from introducing a long ramp across loops or silent gaps.
inline std::map<double, float> arrangedPatternCurve(const ProjectDocument& project, const juce::String& owner,
    const juce::String& plugin, const juce::String& parameter, float baseline)
{
    std::map<double, float> result;
    const auto meter = project.getInitialTimeSignature();
    const double barSteps = meter.numerator * 16.0 / juce::jmax(1, meter.denominator);
    constexpr double edge = 0.001;
    auto clips = project.getPlaylistClips(owner);
    std::sort(clips.begin(), clips.end(), [](const auto& a, const auto& b) { return a.startBar < b.startBar; });
    for (const auto& clip : clips)
    {
        const int slot = project.getPatternIndexForId(owner, clip.patternId);
        if (slot < 0) continue;
        ProjectDocument::AutomationLane lane;
        for (int i = 0; i < project.getPatternAutomationLaneCount(owner, slot); ++i)
        {
            auto candidate = project.getPatternAutomationLane(owner, slot, i);
            if (candidate.pluginName == plugin && candidate.parameterId == parameter) { lane = std::move(candidate); break; }
        }
        if (lane.points.isEmpty()) continue;
        const double start = clip.startBar * barSteps, end = (clip.startBar + clip.lengthBars) * barSteps;
        const int length = juce::jmax(1, project.getPatternLengthSteps(owner, slot));
        if (result.empty()) result[0] = baseline;
        if (start > 0 && (result.empty() || result.rbegin()->first < start)) result[juce::jmax(0.0, start - edge)] = baseline;
        for (double cycle = start; cycle < end; cycle += length)
        {
            const auto stop = juce::jmin(end, cycle + length);
            result[cycle] = valueAt(lane, 0);
            for (const auto& point : lane.points)
                if (point.position < stop - cycle) result[cycle + point.position] = point.value;
            if (stop - cycle > edge) result[stop - edge] = valueAt(lane, stop - cycle - edge);
        }
        result[end] = baseline;
    }
    return result;
}
}
