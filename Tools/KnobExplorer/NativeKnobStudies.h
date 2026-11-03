#pragma once
#include <JuceHeader.h>
#include "References/MonozoticStudio.h"
#include "References/FormantFM.h"
#include "References/VocalMotion.h"
#include "References/Garden.h"
namespace orbit::knobs {
inline void drawReference(juce::Graphics& g,int style,float value)
{
    juce::Slider slider;const float start=juce::MathConstants<float>::pi*1.25f,end=juce::MathConstants<float>::pi*2.75f;
    static hf2::StudioStyle studio;static formantfm::Style fm;static vocalmotion::Style vocal;static garden::GardenStyle gardenLook;
    juce::LookAndFeel* look=style==20?static_cast<juce::LookAndFeel*>(&studio):style<=22?static_cast<juce::LookAndFeel*>(&fm):style==23?static_cast<juce::LookAndFeel*>(&vocal):static_cast<juce::LookAndFeel*>(&gardenLook);
    slider.setLookAndFeel(look);if(style==20)slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff52e2c5));if(style==22)slider.getProperties().set("gold",true);
    look->drawRotarySlider(g,0,0,64,64,value,start,end,slider);slider.setLookAndFeel(nullptr);
}
// Native procedural studies: actual JUCE drawing, also used for the HTML frames.
inline void drawStudy(juce::Graphics& g,int style,float value)
{
    constexpr float pi=juce::MathConstants<float>::pi;const float a=-pi*.75f+value*pi*1.5f;
    const juce::Point<float> c(32,32);auto point=[&](float angle,float radius){return c+juce::Point<float>(std::sin(angle),-std::cos(angle))*radius;};
    auto line=[&](float angle,float r1,float r2,float width,juce::Colour colour){g.setColour(colour);g.drawLine({point(angle,r1),point(angle,r2)},width);};
    const auto white=juce::Colour(0xffedf0ec),ink=juce::Colour(0xff121922),cyan=juce::Colour(0xff71dbe7);
    auto face=[&](float radius,juce::Colour top,juce::Colour bottom){auto b=juce::Rectangle<float>(radius*2,radius*2).withCentre(c);g.setGradientFill(juce::ColourGradient(top,b.getTopLeft(),bottom,b.getBottomRight(),false));g.fillEllipse(b);};
    if(style==25){
        for(int n=0;n<=10;++n)line(-pi*.75f+n*pi*1.5f/10,27,29,.8f,juce::Colour(0xff86949c));
        juce::Path ribs;for(int n=0;n<96;++n){float t=2*pi*n/96.f,r=22+(n%8<4?1.5f:-1.f);auto p=point(t,r);if(n==0)ribs.startNewSubPath(p);else ribs.lineTo(p);}ribs.closeSubPath();g.setColour(ink);g.fillPath(ribs);face(18,juce::Colour(0xff4b4b48),juce::Colour(0xff161a1b));line(a,4,17,2.6f,white);return;
    }
    if(style==26){
        face(24,juce::Colour(0xffe5eaed),juce::Colour(0xff727d89));for(int n=0;n<100;++n){float t=n*2*pi/100;line(t,3,22,.45f,juce::Colours::white.withAlpha(.13f+.15f*std::abs(std::sin(t*2))));}face(14,juce::Colour(0xffd8e0e3),juce::Colour(0xff8798a3));g.setColour(juce::Colour(0xff4f6370));g.drawEllipse(8,8,48,48,.8f);line(a,4,19,2,ink);return;
    }
    if(style==27){
        face(18,juce::Colour(0xff3b4145),ink);juce::Path cap;cap.startNewSubPath(-7,12);cap.quadraticTo(-11,-1,-4,-21);cap.quadraticTo(0,-28,4,-21);cap.quadraticTo(11,-1,7,12);cap.closeSubPath();cap.applyTransform(juce::AffineTransform::rotation(a).translated(32,32));g.setGradientFill(juce::ColourGradient(white,23,9,juce::Colour(0xffa8ada7),42,52,false));g.fillPath(cap);g.setColour(juce::Colour(0xff535e62));g.strokePath(cap,juce::PathStrokeType(.8f));return;
    }
    if(style==28){
        face(26,juce::Colour(0xff535252),juce::Colour(0xff131b22));for(int n=0;n<28;++n)line(a+n*2*pi/28,22,25,.9f,white.withAlpha(.7f));face(18,juce::Colour(0xff625548),juce::Colour(0xff24292e));face(10,juce::Colour(0xffced5d6),juce::Colour(0xff73828c));line(a,3,17,2.1f,white);return;
    }
    if(style==29){
        for(int n=0;n<25;++n){float t=-pi*.75f+n*pi*1.5f/24;g.setColour(n<=juce::roundToInt(value*24)?cyan:juce::Colour(0xff40545c));auto p=point(t,26);g.fillRoundedRectangle(juce::Rectangle<float>(3,5).withCentre(p),1);}
        face(19,juce::Colour(0xff37414b),ink);g.setColour(juce::Colour(0xff596873));g.drawEllipse(13,13,38,38,.8f);line(a,10,16,2,white);return;
    }
    if(style==30){
        face(24,juce::Colour(0xff315f75),juce::Colour(0xff07131e));g.setColour(cyan.withAlpha(.55f));g.drawEllipse(8,8,48,48,1);juce::Path sheen;sheen.addArc(12,11,39,30,pi,pi*2,true);g.setColour(juce::Colours::white.withAlpha(.15f));g.strokePath(sheen,juce::PathStrokeType(5));line(a,2,19,2.5f,cyan);g.setColour(juce::Colours::white.withAlpha(.5f));g.fillEllipse(21,17,5,3);return;
    }
    if(style==31){
        juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::rotation(a,32,32));auto b=juce::Rectangle<float>(13,13,38,38);g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe8ede9),b.getTopLeft(),juce::Colour(0xff939eaa),b.getBottomRight(),false));g.fillRoundedRectangle(b,8);g.setColour(ink.withAlpha(.65f));g.drawRoundedRectangle(b,8,1);g.setColour(juce::Colour(0xff344f64));g.fillRoundedRectangle(29,17,6,17,2);return;
    }
    face(25,juce::Colour(0xff4d5967),ink);for(int n=0;n<24;++n)line(n*2*pi/24,23,25,.8f,juce::Colour(0xffa0adb5));face(17,juce::Colour(0xfff2efdf),juce::Colour(0xffadb7b9));g.setColour(juce::Colour(0xff657680));g.drawEllipse(15,15,34,34,.8f);line(a,3,13,2,juce::Colour(0xff273f54));line(a,20,24,2,cyan);
}
}
