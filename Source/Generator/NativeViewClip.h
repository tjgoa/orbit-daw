#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace orbit::generator
{
// Cocoa plugin views are attached directly to JUCE's native peer, outside the
// software Component paint order. Bound them to the editor body and subtract
// workspace panels that are in front. Other platforms keep JUCE's hierarchy.
class NativeViewClip
{
public:
#if JUCE_MAC
    NativeViewClip();
    ~NativeViewClip();
    void update(juce::Component& editor,juce::Component& body,const std::vector<juce::Rectangle<int>>& blocked);
    void clear();
    int attachedCount() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
#else
    void update(juce::Component&,juce::Component&,const std::vector<juce::Rectangle<int>>&) {}
    void clear() {}
    int attachedCount() const {return 0;}
#endif
};
}
