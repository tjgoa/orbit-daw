#pragma once
#include <JuceHeader.h>
namespace orbit::automation
{
class ParameterEventSlider : public juce::Slider
{
public:
    std::function<void(juce::Component*)> onEvents;
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onEvents) { onEvents(this); return; }
        juce::Slider::mouseDown(e);
    }
};
}
