#pragma once
#include <JuceHeader.h>
namespace formantfm {
inline const juce::Colour cyan{0xff66e5fc},gold{0xffedc47e},ink{0xff071017},edge{0xff365364};
class Style:public juce::LookAndFeel_V4 {
public:
 Style(){setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffd6e9ed));setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setColour(juce::TextButton::buttonColourId,juce::Colour(0xff142630));setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff11536b));setColour(juce::ComboBox::backgroundColourId,ink);setColour(juce::ComboBox::outlineColourId,edge);setColour(juce::ComboBox::textColourId,juce::Colour(0xffe6f5f8));}
 void drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&s)override{
  auto box=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(5);float r=std::min(box.getWidth(),box.getHeight())*.42f;auto c=box.getCentre();
  g.setColour(juce::Colours::black.withAlpha(.6f));g.fillEllipse(c.x-r-2,c.y-r+1,2*r+4,2*r+4);
  juce::ColourGradient fill(juce::Colour(0xff304650),c.x-r,c.y-r,juce::Colour(0xff071017),c.x+r,c.y+r,false);g.setGradientFill(fill);g.fillEllipse(c.x-r,c.y-r,2*r,2*r);
  g.setColour(edge);g.drawEllipse(c.x-r,c.y-r,2*r,2*r,1.f);juce::Path arc;arc.addCentredArc(c.x,c.y,r+2,r+2,0,start,start+pos*(end-start),true);
  auto colour=s.getProperties()["gold"]==juce::var(true)?gold:cyan;g.setColour(colour.withAlpha(.18f));g.strokePath(arc,juce::PathStrokeType(7));g.setColour(colour);g.strokePath(arc,juce::PathStrokeType(2.5f));
  float angle=start+pos*(end-start);g.setColour(juce::Colours::white.withAlpha(.9f));g.drawLine(c.x+std::sin(angle)*r*.40f,c.y-std::cos(angle)*r*.40f,c.x+std::sin(angle)*r*.85f,c.y-std::cos(angle)*r*.85f,1.5f);
 }
};
inline void frame(juce::Graphics&g,juce::Rectangle<float>r,const juce::String&title){juce::ColourGradient bg(juce::Colour(0xff1a2d39),r.getX(),r.getY(),juce::Colour(0xff081117),r.getX(),r.getBottom(),false);g.setGradientFill(bg);g.fillRoundedRectangle(r,7);g.setColour(edge);g.drawRoundedRectangle(r.reduced(.5f),7,1);g.setColour(juce::Colour(0xffedf7fa));g.setFont(14.f);g.drawText(title,r.reduced(12,5).removeFromTop(23),juce::Justification::centredLeft);g.setColour(edge.withAlpha(.5f));g.drawHorizontalLine(int(r.getY()+33),r.getX()+10,r.getRight()-10);}
}
