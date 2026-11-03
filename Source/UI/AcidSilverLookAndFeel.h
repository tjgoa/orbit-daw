#pragma once
#include "KnobStyle.h"
namespace orbit::ui
{
class AcidSilverLookAndFeel final : public FirstSynthReusableHomeKnobLookAndFeel
{
public:
    AcidSilverLookAndFeel()
    {
        setSpriteImage(homeKnobAtlas());darkWhite.setSpriteImage(homeKnobAtlas());
        // Only Dark / White uses this local renderer. The shared Home style,
        // Silver / White, Dark / Original and Mixer knobs retain their artwork.
        configureCleanWhiteKnob(darkWhite);
        setDark(false);
    }
    void setDark(bool value)
    {
        dark=value;const auto ink=dark?juce::Colour(225,239,247):juce::Colour(35,45,58);
        const int ids[]={juce::Label::textColourId,juce::ToggleButton::textColourId,juce::ToggleButton::tickColourId,juce::ComboBox::textColourId,juce::ComboBox::arrowColourId};for(int id:ids)setColour(id,ink);
        setColour(juce::ToggleButton::tickDisabledColourId,dark?juce::Colour(109,143,162):juce::Colour(105,119,137));
        setColour(juce::ComboBox::backgroundColourId,dark?juce::Colour(34,49,61):juce::Colour(248,249,251));
        setColour(juce::ComboBox::outlineColourId,dark?juce::Colour(100,134,151):juce::Colour(140,151,167));
        setColour(juce::TextButton::buttonColourId,dark?juce::Colour(38,56,68):juce::Colour(247,248,250));
        setColour(juce::TextButton::buttonOnColourId,juce::Colour(37,112,143));
        setColour(juce::TextButton::textColourOffId,ink);setColour(juce::TextButton::textColourOnId,ink);
        setColour(juce::Slider::thumbColourId,juce::Colour(78,172,201));
        setColour(juce::PopupMenu::backgroundColourId,dark?juce::Colour(27,43,55):juce::Colour(248,249,251));
        setColour(juce::PopupMenu::textColourId,ink);setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(37,112,143));
    }
    int skin=0;
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider)override
    {
        if(skin==2 && effectiveKnobStyle(slider,2)==2)
            darkWhite.drawRotarySlider(g,x,y,w,h,position,start,end,slider);
        else drawSelectedKnob(g,x,y,w,h,position,start,end,slider,skin==1?1:2);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool over,bool down)override
    {
        if(dark){juce::LookAndFeel_V4::drawButtonBackground(g,b,colour,over,down);return;}
        auto r=b.getLocalBounds().toFloat().reduced(.5f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(down||b.getToggleState()?0xffc8dce6:over?0xffffffff:0xfff7f8fa),r.getTopLeft(),juce::Colour(0xffc2c9d3),r.getBottomLeft(),false));g.fillRoundedRectangle(r,4);g.setColour(juce::Colour(135,146,162));g.drawRoundedRectangle(r,4,1);
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool over,bool down)override
    {
        if(dark){juce::LookAndFeel_V4::drawButtonText(g,b,over,down);return;}
        g.setColour(juce::Colour(35,45,58).withAlpha(b.isEnabled()?1.f:.45f));g.setFont(juce::FontOptions(13));g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(4),juce::Justification::centred,1);
    }
private:
    FirstSynthReusableHomeKnobLookAndFeel darkWhite;
    bool dark=false;
};
}
