#pragma once
#include "GeneratorProcessor.h"

namespace orbit::generator
{
class ArpeggiatorPanel final : public juce::Component
{
public:
    explicit ArpeggiatorPanel(Settings initial):saved(initial)
    {
        on.setButtonText("On");on.setToggleState(initial.arp,juce::dontSendNotification);addAndMakeVisible(on);
        reset.setButtonText("Reset on Chord Trigger");reset.setToggleState(initial.resetOnChord,juce::dontSendNotification);addAndMakeVisible(reset);
        order.addItemList({"Up","Down","Up / Down","Alternate Up","Alternate Down","User note order"},1);order.setSelectedItemIndex(initial.order);addAndMakeVisible(order);
        setup(octaves,initial.octaves,1,5);setup(distance,initial.distance,0,24);setup(gate,initial.gate,-90,100);
        setup(retrigger,initial.retrigger,0,64);setup(repeats,initial.repeats,0,64);setup(shuffle,initial.shuffle,0,75);
        rate.addItemList(rateNames(),1);rate.setSelectedItemIndex(initial.rate);addAndMakeVisible(rate);
        shuffleRate.addItemList(rateNames(),1);shuffleRate.setSelectedItemIndex(initial.shuffleRate);addAndMakeVisible(shuffleRate);
        gate.textFromValueFunction=[](double v){return v==0?juce::String("Off"):juce::String(int(v))+"%";};
        retrigger.textFromValueFunction=[](double v){return v==0?juce::String("Off"):juce::String(int(v));};
        repeats.textFromValueFunction=[](double v){return v==0?juce::String("Unlimited"):juce::String(int(v));};
        shuffle.textFromValueFunction=[](double v){return v==0?juce::String("Off"):juce::String(int(v))+"%";};
        gate.updateText();retrigger.updateText();repeats.updateText();shuffle.updateText();
        for(int i=0;i<16;++i)
        {
            auto& step=rhythm[size_t(i)];step.setButtonText(juce::String(i+1));step.setClickingTogglesState(true);
            step.setToggleState((initial.rhythm&(1<<i))!=0,juce::dontSendNotification);step.setTooltip("Play or rest on rhythm step "+juce::String(i+1));addAndMakeVisible(step);
        }
        addAndMakeVisible(custom);custom.setTooltip("User note order: semitone offsets from the lowest held note, separated by spaces (up to 32).");setCustom(initial);
        for(auto* b:{&load,&render,&apply,&cancel})addAndMakeVisible(b);
        render.setTooltip("Write the current arp/shuffle into this pattern, then disable both playback effects. One undo restores everything.");
        for(size_t i=0;i<labels.size();++i){labels[i].setText(labelText[i],juce::dontSendNotification);addAndMakeVisible(labels[i]);}
        message.setText("Generator-wide playback settings. The pattern notes stay editable; Render writes the result into the current pattern.",juce::dontSendNotification);
        message.setJustificationType(juce::Justification::topLeft);addAndMakeVisible(message);setSize(600,580);
    }
    Settings getSettings()const
    {
        auto s=saved;s.arp=on.getToggleState();s.resetOnChord=reset.getToggleState();s.order=order.getSelectedItemIndex();
        s.octaves=int(octaves.getValue());s.distance=int(distance.getValue());s.gate=int(gate.getValue());
        s.retrigger=int(retrigger.getValue());s.repeats=int(repeats.getValue());s.shuffle=int(shuffle.getValue());
        s.rate=rate.getSelectedItemIndex();s.shuffleRate=shuffleRate.getSelectedItemIndex();s.rhythm=0;
        for(int i=0;i<16;++i)if(rhythm[size_t(i)].getToggleState())s.rhythm|=1<<i;
        const auto values=juce::StringArray::fromTokens(custom.getText()," ,;\t\n",{});
        s.customSize=juce::jlimit(1,32,values.size());s.custom={};
        for(int i=0;i<juce::jmin(32,values.size());++i)s.custom[size_t(i)]=juce::jlimit(-48,48,values[i].getIntValue());return s;
    }
    void setCustom(const Settings& s)
    {juce::StringArray a;for(int i=0;i<s.customSize;++i)a.add(juce::String(s.custom[size_t(i)]));custom.setText(a.joinIntoString(" "),false);}
    void setMessage(const juce::String& text){message.setText(text,juce::dontSendNotification);}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
        g.setColour(findColour(juce::Label::textColourId).withAlpha(.3f));g.drawRoundedRectangle(12,12,float(getWidth()-24),408,4,1);g.drawRoundedRectangle(12,432,float(getWidth()-24),76,4,1);
    }
    void resized() override
    {
        on.setBounds(24,20,62,28);reset.setBounds(90,20,250,28);render.setBounds(getWidth()-122,20,98,28);
        labels[0].setBounds(24,57,150,22);order.setBounds(24,82,346,28);load.setBounds(380,82,getWidth()-404,28);
        const int column=(getWidth()-60)/2;
        juce::Component* fields[]{&octaves,&gate,&distance,&retrigger,&rate,&repeats};
        for(int i=0;i<6;++i){const int x=24+(i%2)*(column+12),y=122+(i/2)*62;labels[size_t(i+1)].setBounds(x,y,column,22);fields[i]->setBounds(x,y+22,column,30);}
        labels[7].setBounds(24,309,250,22);
        const auto width=(getWidth()-48)/16.0;for(int i=0;i<16;++i)rhythm[size_t(i)].setBounds(24+juce::roundToInt(i*width),335,juce::roundToInt(width)-1,30);
        labels[8].setBounds(24,371,170,28);custom.setBounds(194,371,getWidth()-218,28);
        labels[9].setBounds(24,438,column,22);shuffle.setBounds(24,460,column,30);
        labels[10].setBounds(36+column,438,column,22);shuffleRate.setBounds(36+column,460,column,30);
        message.setBounds(24,516,getWidth()-242,54);apply.setBounds(getWidth()-212,532,88,28);cancel.setBounds(getWidth()-116,532,92,28);
    }
    juce::ToggleButton on,reset;
    juce::ComboBox order,rate,shuffleRate;
    juce::Slider octaves,distance,gate,retrigger,repeats,shuffle;
    std::array<juce::TextButton,16> rhythm;
    juce::TextEditor custom;
    juce::TextButton load{"Load MIDI..."},render{"Render..."},apply{"Apply"},cancel{"Cancel"};
private:
    void setup(juce::Slider& s,int value,int low,int high)
    {s.setSliderStyle(juce::Slider::LinearHorizontal);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,78,24);s.setRange(low,high,1);s.setValue(value);addAndMakeVisible(s);}
    Settings saved;
    std::array<juce::Label,11> labels;
    const std::array<const char*,11> labelText{{"Note Order","Range (octaves)","Gate","Distance (semitones)","Retrigger (notes)","Tempo","Repeats","Arpeggiator Rhythm","User note offsets","Shuffle","Shuffle Tempo"}};
    juce::Label message;
};
}
