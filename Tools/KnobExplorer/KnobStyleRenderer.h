#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include "LabyrinthHomeKnob.h"
#include "LabyrinthHomeAtlas.h"
#include "NativeKnobStudies.h"
namespace orbit::knobs {
struct Style {
 const char* name; int home=0; juce::uint32 face=0xff303844,edge=0xff151b23,pointer=0xfff4f7fb,accent=0xff61c9e4;
 float bevel=.15f,arc=1.5f,shadow=.15f; bool ticks=false,notches=false,dot=false,ring=true,body=true;
};
inline const std::array<Style,33>& styles(){static const std::array<Style,33> all{{
 {"Labyrinth Home",1}, {"Labyrinth Soft",2}, {"Labyrinth Dry",3},
 {"Studio Graphite",0,0xff303844,0xff151b23,0xfff4f7fb,0xff61c9e4,.06f,1.5f,.12f},
 {"Pearl",0,0xffe3e7ed,0xff9aa5b3,0xff26364b,0xff469bbd,.04f,1.5f,.08f},
 {"Clean Silver",0,0xffc7ced8,0xff626f80,0xff202c3c,0xff4fb5d5,.18f,1.2f,.12f},
 {"Charcoal Dot",0,0xff343d49,0xff151d27,0xffe6eef7,0xff6acbe1,.05f,1.3f,.08f,false,false,true},
 {"Graphite Needle",0,0xff414b58,0xff19222d,0xffe8f2fb,0xff8ab9d0,.08f,.8f,.12f},
 {"Recessed Silver",0,0xffc9cfd8,0xff475567,0xff26374b,0xff6bbad5,-.16f,1.5f,.08f},
 {"Slim Ring",0,0xff29343f,0xff1a2530,0xffeef5fb,0xff6bc5da,0,2.0f,0,false,false,true},
 {"Plain Dial",0,0xffcbd1db,0xff69778b,0xff253951,0xff6bbad5,.1f,0,.08f,false,false,false,false},
 {"Console Black",0,0xff303946,0xff141e2a,0xfff0f4fa,0xff6bbad5,.1f,0,.14f,true,false,false,false},
 {"Console Silver",0,0xffc9d0db,0xff66778e,0xff20374e,0xff439cbf,.15f,0,.12f,true,false,false,false},
 {"White Cap",0,0xffedf0f4,0xffa9b4c4,0xff20374e,0xff4198b6,.04f,1,.08f,false,false,true},
 {"Notched Graphite",0,0xff3e4855,0xff121b28,0xffeef5fb,0xff68c6df,.1f,1,.12f,false,true},
 {"Notched Silver",0,0xffcbd3de,0xff718096,0xff263b52,0xff479fbd,.16f,1,.12f,false,true},
 {"Satin Blue",0,0xff52738b,0xff293f56,0xffeef6fc,0xff87d2e7,.1f,1,.1f},
 {"Navy Glass",0,0xff24465b,0xff132939,0xffc9f4ff,0xff66d5eb,.3f,1.6f,.14f},
 {"Compact Dot",0,0xff4b5665,0xff253447,0xfff0f6fb,0xff7acade,0,0,0,false,false,true,false},
 {"Minimal Arc",0,0xff303844,0xff40566d,0xff243d53,0xff4e9dbd,0,2.5f,0,false,false,true,true,false},
 {"Monozotic / HF2 (Labyrinth)"}, {"Formant FM Cyan (Labyrinth)"}, {"Formant FM Gold (Labyrinth)"}, {"Vocal Motion (Labyrinth)"}, {"Spectral Garden (Labyrinth)"},
 {"303 Ribbed Black"}, {"Aluminium Radial"}, {"Ivory Pointer Cap"}, {"Vintage Skirt"}, {"LED Encoder"}, {"Glass Lens"}, {"Square Cap"}, {"Concentric Face"}
 }};return all;}
inline juce::Image homeAtlas(){static const auto atlas=[] {juce::MemoryOutputStream out;juce::Base64::convertFromBase64(out,assets::knobSpriteBase64);return juce::ImageFileFormat::loadFrom(out.getData(),out.getDataSize());}();return atlas;}
// All geometry is in a 64-unit square. The native host and HTML atlas use this
// same renderer, with one transform for the requested physical size.
inline void draw(juce::Graphics& target,juce::Rectangle<float> bounds,int preset,float value){
 const auto& s=styles()[size_t(juce::jlimit(0,int(styles().size())-1,preset))];value=juce::jlimit(0.f,1.f,value);
 juce::Graphics::ScopedSaveState save(target);target.addTransform(juce::AffineTransform::scale(bounds.getWidth()/64.f,bounds.getHeight()/64.f).translated(bounds.getX(),bounds.getY()));
 auto& g=target;
 if(preset>=20 && preset<=24){drawReference(g,preset,value);return;}
 if(preset>=25){drawStudy(g,preset,value);return;}
 if(s.home){FirstSynthReusableHomeKnobLookAndFeel laf;laf.setSpriteImage(homeAtlas());auto& t=laf.getStyle();
  if(s.home==2){t.shadowMasterScale=.3f;t.blueRingGlow=0;t.rimRingAlpha=.07f;t.rimRingGlow=.4f;}
  if(s.home==3){t.shadowMasterScale=.15f;t.blueRingGlow=0;t.rimRingAlpha=0;t.spriteEdgeBlend=.6f;}
  juce::Slider slider;laf.drawRotarySlider(g,0,0,64,64,value,0,1,slider);return;}
 constexpr float cx=32,cy=32,r=21;const float start=-juce::MathConstants<float>::pi*.75f,end=-start,angle=start+(end-start)*value;
 auto colour=[](juce::uint32 argb){return juce::Colour(argb);};
 auto point=[](float a,float radius){return juce::Point<float>(cx+std::sin(a)*radius,cy-std::cos(a)*radius);};
 if(s.shadow>0){for(int i=5;i>0;--i){g.setColour(juce::Colours::black.withAlpha(s.shadow*.07f));g.fillEllipse(juce::Rectangle<float>(cx-r-i*.5f,cy-r+2-i*.5f,2*r+i,2*r+i));}}
 if(s.ticks){g.setColour(colour(s.edge).withAlpha(.75f));for(int i=0;i<=10;++i){auto a=start+(end-start)*i/10;auto p=point(a,26),q=point(a,i%5==0?29:28);g.drawLine({p,q},.8f);}}
 if(s.ring && s.arc>0){juce::Path arc;arc.addCentredArc(cx,cy,27,27,0,start,end,true);g.setColour(colour(s.edge).withAlpha(.25f));g.strokePath(arc,juce::PathStrokeType(s.arc,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  if(value>0){juce::Path lit;lit.addCentredArc(cx,cy,27,27,0,start,angle,true);g.setColour(colour(s.accent));g.strokePath(lit,juce::PathStrokeType(s.arc,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}}
 if(s.body){g.setColour(colour(s.edge));g.fillEllipse(cx-r,cy-r,2*r,2*r);
  auto face=colour(s.face);const auto top=s.bevel>=0?face.brighter(s.bevel):face.darker(-s.bevel);const auto bottom=s.bevel>=0?face.darker(s.bevel):face.brighter(-s.bevel);g.setGradientFill(juce::ColourGradient(top,cx-r,cy-r,bottom,cx+r,cy+r,false));g.fillEllipse(cx-r+1,cy-r+1,2*r-2,2*r-2);
  g.setColour(juce::Colours::white.withAlpha(.12f));g.drawEllipse(cx-r+1.5f,cy-r+1.5f,2*r-3,2*r-3,.7f);
  if(s.notches){g.setColour(colour(s.edge).withAlpha(.65f));for(int i=0;i<20;++i){const auto a=i*juce::MathConstants<float>::twoPi/20;g.drawLine({point(a,r-1),point(a,r-3)},1);}}}
 g.setColour(colour(s.pointer));const auto p=point(angle,s.body?15:27);
 if(s.dot)g.fillEllipse(p.x-1.8f,p.y-1.8f,3.6f,3.6f);
 else g.drawLine({point(angle,7),p},1.8f);
}
}
