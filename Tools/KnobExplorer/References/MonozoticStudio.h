#pragma once
#include <JuceHeader.h>
namespace hf2 {
// Local to the Full HF2 editor; other synth pages keep their existing look and feel.
class StudioStyle : public juce::LookAndFeel_V4 {
public:
 StudioStyle(){
  setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff142c34));
  setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff46616b));
  setColour(juce::ComboBox::textColourId,juce::Colour(0xffe2f2f3));
  setColour(juce::TextButton::buttonColourId,juce::Colour(0xff142c34));
  setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff096b70));
  setColour(juce::TextButton::textColourOffId,juce::Colour(0xffd9eff0));
  setColour(juce::TextButton::textColourOnId,juce::Colours::white);
  setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff0b2229));
  setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff12545c));
  setColour(juce::PopupMenu::textColourId,juce::Colour(0xffd9eff0));
 }
 void drawRotarySlider(juce::Graphics&g,int x,int y,int width,int height,float proportion,float start,float end,juce::Slider&s)override{
  auto bounds=juce::Rectangle<float>(float(x),float(y),float(width),float(height)).reduced(5);
  float size=juce::jmin(bounds.getWidth(),bounds.getHeight());auto outer=juce::Rectangle<float>(size,size).withCentre(bounds.getCentre());
  auto centre=outer.getCentre();float radius=size*.44f,thickness=juce::jlimit(3.f,9.f,size*.075f);float angle=start+proportion*(end-start);
  g.setColour(juce::Colours::black.withAlpha(.35f));g.fillEllipse(outer.translated(0,2));
  juce::Path background;background.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
  g.setColour(juce::Colour(0xff123d47));g.strokePath(background,juce::PathStrokeType(thickness,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  juce::Path active;active.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
  auto accent=s.findColour(juce::Slider::rotarySliderFillColourId);g.setColour(accent);g.strokePath(active,juce::PathStrokeType(thickness,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  float body=radius-thickness*.8f;auto face=juce::Rectangle<float>(body*2,body*2).withCentre(centre);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff29454f),face.getTopLeft(),juce::Colour(0xff07191f),face.getBottomRight(),false));g.fillEllipse(face);
  g.setColour(juce::Colour(0xff47717a).withAlpha(.45f));g.drawEllipse(face.reduced(.5f),1.f);
  auto dot=centre+juce::Point<float>(std::sin(angle),-std::cos(angle))*body*.66f;
  float dotRadius=juce::jlimit(2.f,4.f,size*.032f);g.setColour(accent.brighter(.35f));g.fillEllipse(dot.x-dotRadius,dot.y-dotRadius,dotRadius*2,dotRadius*2);
 }
};
}
