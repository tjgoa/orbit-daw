#pragma once
#include <JuceHeader.h>

namespace orbit::mixing
{
struct Layout
{
    int horizontal=0,vertical=0,labelOffset=0,stripWidth=132,knobPercent=200,faderWidth=38,faderStyle=0,knobFinish=0;
    bool operator==(const Layout&)const=default;
    void constrain()
    {horizontal=juce::jlimit(-20,24,horizontal);vertical=juce::jlimit(-12,32,vertical);labelOffset=juce::jlimit(-8,12,labelOffset);stripWidth=juce::jlimit(112,260,stripWidth);knobPercent=juce::jlimit(100,500,knobPercent);faderWidth=juce::jlimit(18,58,faderWidth);faderStyle=juce::jlimit(0,2,faderStyle);knobFinish=juce::jlimit(0,3,knobFinish);}
    juce::String toJson()const
    {
        auto* o=new juce::DynamicObject();o->setProperty("version",1);
        for(auto [key,value]:std::initializer_list<std::pair<const char*,int>>{{"horizontal",horizontal},{"vertical",vertical},{"labelOffset",labelOffset},{"stripWidth",stripWidth},{"knobPercent",knobPercent},{"faderWidth",faderWidth},{"faderStyle",faderStyle},{"knobFinish",knobFinish}})o->setProperty(key,value);
        return juce::JSON::toString(juce::var(o),true);
    }
    static Layout fromJson(const juce::String& text,int legacyKnobSize=200)
    {
        Layout r;r.knobPercent=legacyKnobSize;auto v=juce::JSON::parse(text);
        if(v.isObject() && int(v["version"])==1)
        {
            auto number=[&](const char* key,int fallback){auto p=v[key];if(!(p.isInt()||p.isInt64()||p.isDouble()))return fallback;const double d=double(p);return std::isfinite(d)?int(juce::jlimit(-10000.,10000.,d)):fallback;};
            r.horizontal=number("horizontal",0);r.vertical=number("vertical",0);r.labelOffset=number("labelOffset",0);r.stripWidth=number("stripWidth",132);r.knobPercent=number("knobPercent",legacyKnobSize);r.faderWidth=number("faderWidth",38);r.faderStyle=number("faderStyle",0);r.knobFinish=number("knobFinish",0);
        }
        r.constrain();return r;
    }
};
// A row's spacing and its available face size are independent. Tight rows cap
// knob diameter rather than letting adjacent controls overlap.
inline juce::Rectangle<int> knobCell(juce::Rectangle<int> row,int count,int index,int spacing)
{
    const float fraction=count==3?.31f:.42f;
    const float distance=juce::jlimit(12.f,float(row.getWidth()-16)/(count-1),row.getWidth()*(count==3?.34f:.44f)+spacing);
    const float centre=row.getCentreX()+(index-(count-1)*.5f)*distance;
    const int width=juce::jmax(1,juce::jmin(juce::roundToInt(row.getWidth()*fraction),juce::roundToInt(distance)-3,juce::roundToInt(row.getWidth()-distance*(count-1))));
    return juce::Rectangle<int>(juce::roundToInt(centre)-width/2,row.getY(),width,row.getHeight()).constrainedWithin(row);
}
class LayoutPanel final:public juce::Component,private juce::Timer
{
public:
    explicit LayoutPanel(Layout initial):opening(initial)
    {
        description.setText("Preview Mixer and Master layouts here. Changes save automatically. Spacing is a pixel offset from the original layout; tight rows limit knob diameter. Taller layouts scroll inside the mixer.",juce::dontSendNotification);description.setFont(juce::FontOptions(13));description.setColour(juce::Label::textColourId,juce::Colour(0xff202731));addAndMakeVisible(description);
        const std::array<juce::String,8> names{"Horizontal knob spacing","Vertical row spacing","Label offset","Channel strip width","Knob size","Fader cap width","Fader finish","Knob finish"};
        const std::array<int,6> low{-20,-12,-8,112,100,18},high{24,32,12,260,500,58};
        for(int i=0;i<8;++i){labels[size_t(i)].setText(names[size_t(i)],juce::dontSendNotification);labels[size_t(i)].setColour(juce::Label::textColourId,juce::Colour(0xff202731));addAndMakeVisible(labels[size_t(i)]);}
        for(int i=0;i<6;++i){auto& s=sliders[size_t(i)];s.setSliderStyle(juce::Slider::LinearHorizontal);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,24);s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xff202731));s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::white);s.setRange(low[size_t(i)],high[size_t(i)],i==4?25:1);s.setTextValueSuffix(i==4?"%":" px");addAndMakeVisible(s);s.onValueChange=[this]{changed();};s.onDragEnd=[this]{commit();};}
        fader.addItemList({"Original","Studio Silver","Studio Dark"},1);finish.addItemList({"Palette default","White / blue indicator","White / charcoal indicator","Silver / charcoal indicator"},1);
        for(auto* c:{&fader,&finish}){addAndMakeVisible(*c);c->onChange=[this]{changed();};}
        for(auto* b:{&reset,&revert,&close})addAndMakeVisible(*b);
        reset.onClick=[this]{setSettings({});changed();};revert.onClick=[this]{setSettings(opening);changed();};close.onClick=[this]{commit();if(onClose)onClose();};setSettings(initial);setSize(550,410);
    }
    ~LayoutPanel()override{stopTimer();}
    std::function<void(Layout)> onPreview,onSave;
    std::function<void()> onClose;
    std::array<juce::Slider,6> sliders;
    juce::ComboBox fader,finish;
    juce::TextButton reset{"Reset layout"},revert{"Revert"},close{"Close"};
    Layout settings()const{return {int(sliders[0].getValue()),int(sliders[1].getValue()),int(sliders[2].getValue()),int(sliders[3].getValue()),int(sliders[4].getValue()),int(sliders[5].getValue()),fader.getSelectedItemIndex(),finish.getSelectedItemIndex()};}
    void setSettings(Layout value)
    {value.constrain();const std::array<int,6> vals{value.horizontal,value.vertical,value.labelOffset,value.stripWidth,value.knobPercent,value.faderWidth};for(size_t i=0;i<6;++i)sliders[i].setValue(vals[i],juce::dontSendNotification);fader.setSelectedItemIndex(value.faderStyle,juce::dontSendNotification);finish.setSelectedItemIndex(value.knobFinish,juce::dontSendNotification);}
    void beginSession(Layout value){commit();opening=value;setSettings(value);}
    void commit(){stopTimer();if(pending){pending=false;if(onSave)onSave(settings());}}
    void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xffedf0f2));}
    void resized()override
    {auto a=getLocalBounds().reduced(16);description.setBounds(a.removeFromTop(63));for(int i=0;i<8;++i){auto row=a.removeFromTop(34);labels[size_t(i)].setBounds(row.removeFromLeft(195));if(i<6)sliders[size_t(i)].setBounds(row);else(i==6?fader:finish).setBounds(row.reduced(2,3));}auto row=a.removeFromBottom(30);reset.setBounds(row.removeFromLeft(110));row.removeFromLeft(8);revert.setBounds(row.removeFromLeft(84));close.setBounds(row.removeFromRight(84));}
private:
    void changed(){pending=true;if(onPreview)onPreview(settings());startTimer(350);}
    void timerCallback()override{commit();}
    Layout opening;bool pending=false;juce::Label description;std::array<juce::Label,8> labels;
};
inline int compactVariant(int skin){return skin==5?3:skin==6?2:skin>=3?skin-2:0;}
}
