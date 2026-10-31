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
    int row=0,startBar=0;
    juce::String id;
    bool operator<(const ClipKey& b) const {return std::tie(row,startBar,id)<std::tie(b.row,b.startBar,b.id);}
    bool operator==(const ClipKey&) const=default;
};
using Selection=std::set<ClipKey>;
struct SelectionItem {ClipKey key;int lengthBars=1;};
}
