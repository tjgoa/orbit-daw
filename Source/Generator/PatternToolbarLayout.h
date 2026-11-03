#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace orbit::generator
{
// Fixed Orion ordering and control sizes in both independent and Piano Roll hosts.
struct PatternToolbarLayout
{
    juce::Rectangle<int> solo,mute,piano,banks,stepsLabel,steps,up,down,presetLabel,preset,open,save;
    explicit PatternToolbarLayout(juce::Rectangle<int> area)
    {
        const int x=area.getX()+5,y=area.getCentreY()-17;
        solo={x,y+1,22,16};mute={x,y+18,22,16};piano={x+27,y+1,30,33};banks={x+63,y+1,160,33};
        stepsLabel={x+232,y-3,64,13};steps={x+232,y+11,48,25};up={x+281,y+11,16,12};down={x+281,y+24,16,12};
        const int width=juce::jlimit(100,180,area.getWidth()-377);
        presetLabel={x+309,y-3,width,13};preset={x+309,y+11,width,25};open={preset.getRight()+5,y+11,25,25};save={open.getRight()+3,y+11,25,25};
    }
};
class PianoIconButton:public juce::TextButton
{
public:
    void paintButton(juce::Graphics& g,bool over,bool down)override
    {
        auto r=getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(over||down?0xff34768e:0xff294854));g.fillRoundedRectangle(r,3);
        g.setColour(juce::Colour(0xff9cc8d8));g.drawRoundedRectangle(r,3,1);
        auto k=r.reduced(5,6);g.setColour(juce::Colours::whitesmoke);g.fillRect(k);
        g.setColour(juce::Colour(0xff172832));const float w=k.getWidth()/4;
        for(int i=1;i<4;++i){g.drawVerticalLine(int(k.getX()+i*w),k.getY(),k.getBottom());if(i!=3)g.fillRect(k.getX()+i*w-w*.22f,k.getY(),w*.45f,k.getHeight()*.6f);}
    }
};
}

namespace orbit::generator
{
class PresetIconButton:public juce::TextButton
{
public:
    explicit PresetIconButton(bool saving):save(saving){}
    void paintButton(juce::Graphics& g,bool over,bool down)override
    {
        auto r=getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(over||down?0xff385868:0xff203340));g.fillRoundedRectangle(r,3);g.setColour(juce::Colour(0xff9cc8d8));g.drawRoundedRectangle(r,3,1);
        auto icon=r.reduced(5);if(save){g.drawRect(icon,1.3f);g.drawRect(icon.withTrimmedBottom(icon.getHeight()*.55f).reduced(3,0),1);g.drawRect(icon.withTrimmedTop(icon.getHeight()*.55f).reduced(2,0),1);}
        else{juce::Path folder;folder.startNewSubPath(icon.getX(),icon.getBottom());folder.lineTo(icon.getX(),icon.getY());folder.lineTo(icon.getCentreX()-2,icon.getY());folder.lineTo(icon.getCentreX(),icon.getY()+3);folder.lineTo(icon.getRight(),icon.getY()+3);folder.lineTo(icon.getRight(),icon.getBottom());folder.closeSubPath();g.strokePath(folder,juce::PathStrokeType(1.3f));}
    }
private:bool save;
};
}
