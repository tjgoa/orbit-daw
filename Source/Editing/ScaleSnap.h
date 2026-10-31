#pragma once

#include <algorithm>
#include <array>
#include <vector>

namespace orbit::editing
{
struct ScaleDefinition
{
    const char* name;
    std::vector<int> intervals;
};
inline const std::array<ScaleDefinition, 17>& scaleDefinitions()
{
    static const std::array<ScaleDefinition, 17> scales {{
        {"Major (Ionian)", {0,2,4,5,7,9,11}},
        {"Natural minor (Aeolian)", {0,2,3,5,7,8,10}},
        {"Harmonic minor", {0,2,3,5,7,8,11}},
        {"Melodic minor (ascending)", {0,2,3,5,7,9,11}},
        {"Double harmonic minor (Hungarian minor)", {0,2,3,6,7,8,11}},
        {"Double harmonic major", {0,1,4,5,7,8,11}},
        {"Dorian", {0,2,3,5,7,9,10}},
        {"Phrygian", {0,1,3,5,7,8,10}},
        {"Lydian", {0,2,4,6,7,9,11}},
        {"Mixolydian", {0,2,4,5,7,9,10}},
        {"Locrian", {0,1,3,5,6,8,10}},
        {"Major pentatonic", {0,2,4,7,9}},
        {"Minor pentatonic", {0,3,5,7,10}},
        {"Blues", {0,3,5,6,7,10}},
        {"Whole tone", {0,2,4,6,8,10}},
        {"Whole-half diminished", {0,2,3,5,6,8,9,11}},
        {"Half-whole diminished", {0,1,3,4,6,7,9,10}}
    }};
    return scales;
}
struct ScaleSnap
{
    bool enabled = false;
    int root = 0, scale = 0;
    ScaleSnap normalized() const
    { return {enabled, std::clamp(root, 0, 11), std::clamp(scale, 0, int(scaleDefinitions().size()) - 1)}; }
    bool contains(int pitch) const
    {
        const auto s = normalized();
        const int degree = ((pitch - s.root) % 12 + 12) % 12;
        const auto& intervals = scaleDefinitions()[size_t(s.scale)].intervals;
        return std::find(intervals.begin(), intervals.end(), degree) != intervals.end();
    }
    int snap(int pitch) const
    {
        pitch = std::clamp(pitch, 0, 127);
        if (!enabled || contains(pitch)) return pitch;
        // Prefer the lower note in a tie, with valid MIDI endpoints in every key.
        for (int distance = 1; distance <= 12; ++distance)
        {
            if (pitch - distance >= 0 && contains(pitch - distance)) return pitch - distance;
            if (pitch + distance <= 127 && contains(pitch + distance)) return pitch + distance;
        }
        return pitch;
    }
};
}
