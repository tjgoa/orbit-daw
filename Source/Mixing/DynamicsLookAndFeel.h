#pragma once
// Original vector controls for Orbit's dynamics panels. No external artwork.
namespace orbit::mixing
{
class DynamicsKnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,
                          float start,float end,juce::Slider& slider) override
    {
        const float side=float(juce::jmin(width,height))-14.0f;
        if(side<=0)return;
        const auto b=juce::Rectangle<float>(0,0,side,side).withCentre({float(x)+width*.5f,float(y)+height*.5f});
        const auto c=b.getCentre();const float r=side*.5f;
        for(int i=0;i<=10;++i)
        {
            const float angle=start+(end-start)*float(i)/10;
            g.setColour(juce::Colour(0xff8c969c));
            g.drawLine(c.x+std::sin(angle)*(r+2),c.y-std::cos(angle)*(r+2),
                       c.x+std::sin(angle)*(r+5),c.y-std::cos(angle)*(r+5),1);
        }
        g.setColour(juce::Colour(0xff080d10));g.fillEllipse(b.translated(0,2));
        juce::ColourGradient metal(juce::Colour(0xffe0e5e7),b.getX(),b.getY(),juce::Colour(0xff4c5962),b.getRight(),b.getBottom(),false);
        metal.addColour(.38,juce::Colour(0xffaeb8bf));metal.addColour(.5,juce::Colour(0xff5a6872));metal.addColour(.8,juce::Colour(0xffc0c9ce));
        g.setGradientFill(metal);g.fillEllipse(b);
        g.setColour(juce::Colour(0xff0b141b));g.drawEllipse(b,1.5f);
        g.setColour(juce::Colour(0xffc1cbd1).withAlpha(.7f));g.drawEllipse(b.reduced(3),.8f);
        const float angle=start+(end-start)*position;
        g.setColour(slider.isMouseOverOrDragging()?juce::Colour(0xffffce78):juce::Colour(0xff192c37));
        g.drawLine(c.x+std::sin(angle)*r*.26f,c.y-std::cos(angle)*r*.26f,
                   c.x+std::sin(angle)*r*.79f,c.y-std::cos(angle)*r*.79f,3);
    }
};
class DynamicsMeterPanel final : public juce::Component
{
public:
    float inputDb=-100,outputDb=-100,reductionDb=0;
    void setLevels(float input,float output,float reduction)
    {inputDb=input;outputDb=output;reductionDb=juce::jlimit(0.0f,60.0f,reduction);repaint();}
    void paint(juce::Graphics& g) override
    {
        auto b=getLocalBounds().toFloat();g.fillAll(juce::Colour(0xff17252f));
        auto vu=b.reduced(8).withTrimmedRight(b.getWidth()*.44f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe7d8a5),vu.getTopLeft(),juce::Colour(0xffa38c55),vu.getBottomRight(),false));
        g.fillRoundedRectangle(vu,3);g.setColour(juce::Colour(0xff060b0e));g.drawRoundedRectangle(vu,3,2);
        g.setFont(juce::FontOptions(10,juce::Font::bold));g.drawText("GAIN REDUCTION - dB",vu.removeFromTop(18),juce::Justification::centred);
        const auto pivot=juce::Point<float>{vu.getCentreX(),vu.getBottom()+12};
        const float radius=juce::jmin(vu.getWidth()*.47f,vu.getHeight()+8);
        for(int i=0;i<=6;++i)
        {
            const float angle=-.95f+1.9f*i/6;
            const auto tip=pivot+juce::Point<float>{std::sin(angle)*radius,-std::cos(angle)*radius};
            const auto tail=pivot+juce::Point<float>{std::sin(angle)*(radius-6),-std::cos(angle)*(radius-6)};
            g.setColour(juce::Colour(i>4?0xff934831:0xff3b3930));g.drawLine({tail,tip},1);
            const auto label=pivot+juce::Point<float>{std::sin(angle)*(radius-17),-std::cos(angle)*(radius-17)};
            g.drawText(juce::String(i*5),juce::Rectangle<float>(label.x-10,label.y-6,20,12),juce::Justification::centred);
        }
        const float angle=-.95f+1.9f*juce::jlimit(0.0f,1.0f,reductionDb/30);
        g.saveState();g.reduceClipRegion(vu.toNearestInt());g.setColour(juce::Colour(0xffa73427));
        g.drawLine({pivot,pivot+juce::Point<float>{std::sin(angle)*(radius-3),-std::cos(angle)*(radius-3)}},1.8f);g.restoreState();
        auto readouts=b.removeFromRight(b.getWidth()*.43f).reduced(7,8);
        for(int i=0;i<3;++i)
        {
            auto row=readouts.removeFromTop(readouts.getHeight()/float(3-i));
            g.setColour(juce::Colour(0xffa9bac4));g.drawText(i==0?"INPUT":i==1?"OUTPUT":"REDUCTION",row.removeFromLeft(row.getWidth()*.48f),juce::Justification::centredLeft);
            g.setColour(i==2?juce::Colour(0xffffce78):juce::Colour(0xffd4ecf4));
            const float db=i==0?inputDb:i==1?outputDb:reductionDb;
            g.drawText((db<=-99?juce::String("-inf"):juce::String(db,1))+(i==2?" dB":" dBFS"),row,juce::Justification::centredRight);
        }
    }
};
}
