// Orbit Geonkick audition adapter. GPL-3.0-or-later; see ThirdParty/Geonkick/UPSTREAM.md.
#pragma once
#include "GeonkickPlugin.h"
#include "../Options/Appearance.h"
#include "../Automation/ParameterEventSlider.h"
namespace orbit::synth
{
class GeonkickEditor final : public tracktion::engine::Plugin::EditorComponent, private juce::Timer
{
public:
    GeonkickEditor(GeonkickPlugin::Ptr owner,std::function<void()> begin,std::function<void()> changed,std::function<void()> hit,std::function<void()> demo)
        :pluginOwner(owner),plugin(dynamic_cast<GeonkickPlugin*>(owner.get()))
    {
        for(size_t i=0;i<knobs.size();++i)
        {
            const size_t p=i+1;auto& k=knobs[i];labels[i].setText(GeonkickPlugin::names[p],juce::dontSendNotification);labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);
            k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);k.setPopupDisplayEnabled(true,false,this);
            k.setRange(GeonkickPlugin::minima[p],GeonkickPlugin::maxima[p],p==1?1:0);if(p==2 || p==5)k.setSkewFactorFromMidPoint(GeonkickPlugin::defaults[p]);
            k.setValue(plugin->parameters[p]->getCurrentValue(),juce::dontSendNotification);k.setDoubleClickReturnValue(true,GeonkickPlugin::defaults[p]);
            k.textFromValueFunction=[this,p](double x){return juce::String(juce::roundToInt(x))+plugin->parameters[p]->getLabel();};
            k.onDragStart=[this,i,p,begin]{gestures[i]=true;if(begin)begin();plugin->parameters[p]->parameterChangeGestureBegin();};
            k.onDragEnd=[this,i,p]{gestures[i]=false;plugin->parameters[p]->parameterChangeGestureEnd();};
            k.onValueChange=[this,i,p,begin,changed]{if(!gestures[i] && begin)begin();plugin->parameters[p]->setParameter(float(knobs[i].getValue()),juce::sendNotification);if(changed)changed();};
            k.onEvents=[this,p](juce::Component* c){if(onEditEvents)onEditEvents(int(p),c);};addAndMakeVisible(k);
        }
        for(int i=0;i<6;++i)sound.addItem(GeonkickPlugin::sounds[size_t(i)],i+1);
        sound.setSelectedId(juce::roundToInt(plugin->parameters[0]->getCurrentValue())+1,juce::dontSendNotification);
        sound.onChange=[this,begin,changed]{if(begin)begin();plugin->parameters[0]->setParameter(float(sound.getSelectedId()-1),juce::sendNotification);if(changed)changed();};addAndMakeVisible(sound);
        hitButton.setButtonText("Hear hit");hitButton.onClick=std::move(hit);addAndMakeVisible(hitButton);
        demoButton.setButtonText("Demo pattern");demoButton.onClick=std::move(demo);addAndMakeVisible(demoButton);
        setSize(800,210);startTimerHz(20);
    }
    bool allowWindowResizing() override{return false;}
    juce::ComponentBoundsConstrainer* getBoundsConstrainer() override{return nullptr;}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(options::colour(30,44,54));g.setColour(options::colour(173,221,232));g.setFont(15);
        g.drawText(plugin->engineReady()?"GEONKICK AUDITION":"Geonkick engine unavailable",14,4,300,28,juce::Justification::centredLeft);
        g.setFont(12);g.drawText("8 voices | C2 original pitch | Choose a sound, then Hear hit or Demo pattern",14,getHeight()-26,getWidth()-28,20,juce::Justification::centredLeft);
    }
    void resized() override
    {
        auto area=getLocalBounds().reduced(10);auto top=area.removeFromTop(32);demoButton.setBounds(top.removeFromRight(130).reduced(2));hitButton.setBounds(top.removeFromRight(90).reduced(2));sound.setBounds(top.removeFromRight(140).reduced(2));area.removeFromBottom(24);
        const int width=area.getWidth()/7;for(size_t i=0;i<knobs.size();++i){auto col=area.removeFromLeft(width);labels[i].setBounds(col.removeFromTop(24));knobs[i].setBounds(col.reduced(2));}
    }
    std::array<automation::ParameterEventSlider,7> knobs;
    juce::ComboBox sound;juce::TextButton hitButton,demoButton;
    std::function<void(int,juce::Component*)> onEditEvents;
private:
    void timerCallback() override
    {for(size_t i=0;i<knobs.size();++i)if(!gestures[i])knobs[i].setValue(plugin->parameters[i+1]->getCurrentValue(),juce::dontSendNotification);sound.setSelectedId(juce::roundToInt(plugin->parameters[0]->getCurrentValue())+1,juce::dontSendNotification);}
    tracktion::engine::Plugin::Ptr pluginOwner;GeonkickPlugin* plugin;
    std::array<juce::Label,7> labels;std::array<bool,7> gestures{};
};
}
