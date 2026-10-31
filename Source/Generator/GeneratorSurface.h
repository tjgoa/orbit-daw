#pragma once
#include <JuceHeader.h>
#include "../Options/Appearance.h"

namespace orbit::generator
{
// One workspace sibling owns the complete generator, including its opaque
// chrome. Moving or raising it cannot interleave its body with Mixer/Playlist.
class GeneratorSurface final : public juce::Component
{
public:
    static constexpr int titleHeight=30,toolbarHeight=56;
    GeneratorSurface(){setOpaque(true);addMouseListener(this,true);}
    std::function<void()> onActivate;
    bool expandedRoll=false;
    void mouseDown(const juce::MouseEvent&) override {if(onActivate)onActivate();}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(options::colour(45,65,77));
        g.setColour(expandedRoll?options::colour(22,32,42):options::colour(45,65,77));
        g.fillRect(getLocalBounds().removeFromTop(titleHeight+toolbarHeight));
        g.setColour(options::colour(106,137,151));g.drawRect(getLocalBounds());
        if(!expandedRoll)
        {
            g.setColour(options::colour(157,193,208));
            for(int row=0;row<3;++row)for(int column=0;column<2;++column)
                g.fillEllipse(float(41+column*5),float(6+row*5),2,2);
        }
    }
};
}
