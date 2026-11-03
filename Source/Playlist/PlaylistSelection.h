#pragma once
#include <JuceHeader.h>
#include <set>
#include <optional>
#include <tuple>
#include <vector>

namespace orbit::playlist
{
struct ClipKey
{
    int row=0;double startBar=0;
    juce::String id;
    // An audio-only time selection; identity remains the owning native clip.
    double rangeStart=-1,rangeEnd=-1;
    bool operator<(const ClipKey& b) const {return std::tie(row,startBar,id)<std::tie(b.row,b.startBar,b.id);}
    bool operator==(const ClipKey& b) const {return row==b.row && startBar==b.startBar && id==b.id;}
};
using Selection=std::set<ClipKey>;
struct SelectionItem {ClipKey key;double lengthBars=1;};
}
