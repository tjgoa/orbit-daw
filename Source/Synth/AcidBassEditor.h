#pragma once
#include "AcidBassPlugin.h"
#include "../Options/Appearance.h"
#include "../Automation/ParameterEventSlider.h"

namespace orbit::synth
{
class AcidBassEditor final : public tracktion::engine::Plugin::EditorComponent,private juce::Timer
{
public:
    AcidBassEditor(AcidBassPlugin::Ptr pluginIn,std::function<void()> begin,std::function<void()> changed,std::function<void()> demo)
        :pluginOwner(pluginIn),plugin(dynamic_cast<AcidBassPlugin*>(pluginIn.get()))
    {
        for(std::size_t i=0;i<knobs.size();++i)
        {
            labels[i].setText(AcidBassPlugin::names[parameterIndex(i)],juce::dontSendNotification);
            labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);
            auto& knob=knobs[i];knob.onEvents=[this,i](juce::Component* c){if(onEditEvents)onEditEvents(int(parameterIndex(i)),c);};knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
            knob.setPopupDisplayEnabled(false,true,this);
            knob.setRange(AcidBassPlugin::minima[parameterIndex(i)],AcidBassPlugin::maxima[parameterIndex(i)],i==0?1:0);
            if(i==1 || i==4 || i==7 || i==10 || i==11)knob.setSkewFactorFromMidPoint(AcidBassPlugin::defaults[parameterIndex(i)]);
            knob.setDoubleClickReturnValue(true,AcidBassPlugin::defaults[parameterIndex(i)]);
            knob.textFromValueFunction=[this,i](double value){return juce::String(juce::roundToInt(value))+plugin->parameters[parameterIndex(i)]->getLabel();};
            knob.valueFromTextFunction=[this,i](const juce::String& text){return plugin->parameters[parameterIndex(i)]->stringToValue(text);};
            knob.setValue(plugin->parameters[parameterIndex(i)]->getCurrentValue(),juce::dontSendNotification);
            knob.onDragStart=[this,i,begin]{gestures[i]=true;if(begin)begin();plugin->parameters[parameterIndex(i)]->parameterChangeGestureBegin();};
            knob.onDragEnd=[this,i]{gestures[i]=false;plugin->parameters[parameterIndex(i)]->parameterChangeGestureEnd();};
            knob.onValueChange=[this,i,begin,changed]{if(!gestures[i] && begin)begin();plugin->parameters[parameterIndex(i)]->setParameter(static_cast<float>(knobs[i].getValue()),juce::sendNotification);if(changed)changed();};
            knob.setTooltip(i==5?"Accent adds volume and filter emphasis on accented steps":i==6?"Accent filter recovery: left is faster":i==7?"Glide time for steps marked Slide":juce::String(AcidBassPlugin::names[parameterIndex(i)]));
            addAndMakeVisible(knob);
        }
        wave.setButtonText("Saw");wave.setClickingTogglesState(true);
        wave.setToggleState(plugin->parameters[8]->getCurrentValue()>=0.5f,juce::dontSendNotification);
        wave.setButtonText(wave.getToggleState()?"Square":"Saw");
        wave.onClick=[this,begin,changed]{wave.setButtonText(wave.getToggleState()?"Square":"Saw");if(begin)begin();plugin->parameters[8]->setParameter(wave.getToggleState()?1:0,juce::sendNotification);if(changed)changed();};
        addAndMakeVisible(wave);demoButton.setButtonText("Demo pattern");demoButton.onClick=std::move(demo);addAndMakeVisible(demoButton);
        effects.setButtonText("Effects / Drive  +");effects.setClickingTogglesState(true);
        effects.onClick=[this]{setEffectsExpanded(effects.getToggleState());};addAndMakeVisible(effects);
        mode.addItemList({"Soft","Sine Fold","Triangle Foldback"},1);refreshMode();
        mode.setTooltip("Choose soft saturation, sine folding or angular triangle foldback.");
        mode.onChange=[this,begin,changed]
        {
            const int choice=mode.getSelectedId();if(choice<1 || choice>3)return;
            const float triangle=choice==3?1.f:0.f;
            const bool oldChanged=choice!=3 && plugin->parameters[14]->getCurrentValue()!=float(choice-1);
            const bool newChanged=plugin->parameters[AcidBassPlugin::triangleFoldIndex]->getCurrentValue()!=triangle;
            if(!oldChanged && !newChanged)return;if(begin)begin();
            auto set=[this](size_t index,float value){auto& p=plugin->parameters[index];p->parameterChangeGestureBegin();p->setParameter(value,juce::sendNotification);p->parameterChangeGestureEnd();};
            if(oldChanged)set(14,float(choice-1));if(newChanged)set(AcidBassPlugin::triangleFoldIndex,triangle);
            if(changed)changed();
        };addChildComponent(mode);
        effects.setTooltip("Show Drive, Distort blend, Low Cut, Tone and Output. Distort 0% keeps the original sound.");
        setEffectsExpanded(false);setSize(880,190);startTimerHz(20);
    }
    bool allowWindowResizing() override{return false;}
    juce::ComponentBoundsConstrainer* getBoundsConstrainer() override{return nullptr;}
    void setEffectsExpanded(bool expanded)
    {
        const bool change=effectsExpanded!=expanded;effectsExpanded=expanded;effects.setToggleState(expanded,juce::dontSendNotification);
        effects.setButtonText(expanded?"Effects / Drive  -":"Effects / Drive  +");
        for(size_t i=8;i<knobs.size();++i){knobs[i].setVisible(expanded);labels[i].setVisible(expanded);}mode.setVisible(expanded);
        if(onEffectsExpanded){if(change)onEffectsExpanded(expanded);}else setSize(getWidth(),preferredHeight());
        resized();repaint();
    }
    int preferredHeight()const{return effectsExpanded?320:190;}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(options::colour(30,44,54));g.setColour(options::colour(173,221,232));g.setFont(17);
        g.drawText("ACID BASS",juce::Rectangle<int>(14,juce::jmin(152,getHeight()-38),140,30),juce::Justification::centredLeft);
        if(effectsExpanded){g.setColour(options::colour(70,101,114));g.drawHorizontalLine(190,8.f,float(getWidth()-8));}
    }
    void resized() override
    {
        auto area=getLocalBounds().reduced(8);auto synth=area.removeFromTop(174);
        auto bottom=synth.removeFromBottom(30);
        demoButton.setBounds(bottom.removeFromRight(130).reduced(2));wave.setBounds(bottom.removeFromRight(115).reduced(2));
        effects.setBounds(bottom.removeFromRight(170).reduced(2));
        const int width=synth.getWidth()/8;
        for(size_t i=0;i<8;++i){auto column=synth.removeFromLeft(width);labels[i].setBounds(column.removeFromTop(24));knobs[i].setBounds(column.reduced(3,0));}
        if(effectsExpanded)
        {
            auto fx=getLocalBounds().withTrimmedTop(198).reduced(8,0);mode.setBounds(fx.removeFromRight(168).withHeight(30).translated(0,50).reduced(2));
            const int fxWidth=fx.getWidth()/5;
            for(size_t i=8;i<knobs.size();++i){auto column=fx.removeFromLeft(fxWidth);labels[i].setBounds(column.removeFromTop(24));knobs[i].setBounds(column.reduced(3,0));}
        }
    }
    std::function<void(int,juce::Component*)> onEditEvents;
    std::function<void(bool)> onEffectsExpanded;
    std::array<automation::ParameterEventSlider,13> knobs;juce::TextButton wave,demoButton,effects;juce::ComboBox mode;
private:
    void refreshMode(){mode.setSelectedId(plugin->parameters[AcidBassPlugin::triangleFoldIndex]->getCurrentValue()>=.5f?3:plugin->parameters[14]->getCurrentValue()>=.5f?2:1,juce::dontSendNotification);}
    void timerCallback() override
    {
        for(std::size_t i=0;i<knobs.size();++i)if(!knobs[i].isMouseButtonDown())knobs[i].setValue(plugin->parameters[parameterIndex(i)]->getCurrentValue(),juce::dontSendNotification);
        wave.setToggleState(plugin->parameters[8]->getCurrentValue()>=0.5f,juce::dontSendNotification);
        wave.setButtonText(wave.getToggleState()?"Square":"Saw");
        refreshMode();
    }
    tracktion::engine::Plugin::Ptr pluginOwner;
    AcidBassPlugin* plugin;
    static constexpr size_t parameterIndex(size_t knob){return knob<8?knob:knob+1;}
    bool effectsExpanded=false;
    std::array<juce::Label,13> labels;std::array<bool,13> gestures{};
};
}
