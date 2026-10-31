#pragma once
#include "../Project/ProjectDocument.h"
#include "../Workspace/WorkspaceWindow.h"
#include "../Editing/PatternEditorTools.h"

namespace orbit::automation
{
using Points=juce::Array<ProjectDocument::AutomationPoint>;
struct LfoSettings
{
    int shape=0;bool synced=true;double amount=1,speed=1,offset=.5,phase=0,fadeIn=0,fadeOut=0,curvature=0;
};
inline Points lfoCurve(double start,int steps,double bpm,LfoSettings s)
{
    Points points;if(steps<=0)return points;steps=juce::jmin(65536,steps);
    const double cycles=s.synced?s.speed:s.speed*(steps*.25*60/juce::jlimit(20.,300.,bpm));
    for(int i=0;i<steps;++i)
    {
        const double position=steps>1?double(i)/(steps-1):0;
        const double turn=position*cycles+s.phase,p=turn-std::floor(turn);
        double wave=s.shape==0?std::sin(turn*juce::MathConstants<double>::twoPi):s.shape==1?1-4*std::abs(p-.5):s.shape==2?2*p-1:s.shape==3?1-2*p:s.shape==4?(p<.5?1:-1):2*(std::sin(std::floor(turn*16)*127.1+311.7)*43758.5453-std::floor(std::sin(std::floor(turn*16)*127.1+311.7)*43758.5453))-1;
        wave=std::copysign(std::pow(std::abs(wave),std::pow(4.,s.curvature)),wave);
        double envelope=1;if(s.fadeIn>0)envelope=juce::jmin(envelope,position/s.fadeIn);if(s.fadeOut>0)envelope=juce::jmin(envelope,(1-position)/s.fadeOut);
        points.add({start+i,float(juce::jlimit(0.,1.,s.offset+.5*s.amount*wave*envelope))});
    }
    return points;
}
class LfoPanel final:public juce::Component
{
public:
    LfoPanel()
    {
        shapes.addItemList({"Sine","Triangle","Rising saw","Falling saw","Square","Random / hold"},1);shapes.setSelectedId(1);addAndMakeVisible(shapes);
        sync.setButtonText("Sync tempo");sync.setToggleState(true,juce::dontSendNotification);addAndMakeVisible(sync);
        const char* names[]={"Amount","Speed","Offset","Phase","Fade in","Fade out","Curvature"};
        for(int i=0;i<7;++i)
        {
            auto& label=labels[size_t(i)];label.setText(names[i],juce::dontSendNotification);addAndMakeVisible(label);
            auto& slider=values[size_t(i)];slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
            slider.setRange(i==1?.125:i==6?-1:0,i==1?32:1,i==1?.125:.01);slider.setValue(i<=1?1:i==2?.5:0);slider.onValueChange=[this]{changed();};addAndMakeVisible(slider);
        }
        shapes.onChange=[this]{changed();};sync.onClick=[this]{changed();};apply.setButtonText("Apply");cancel.setButtonText("Cancel");addAndMakeVisible(apply);addAndMakeVisible(cancel);
        apply.onClick=[this]{if(onApply)onApply(settings());};cancel.onClick=[this]{if(onCancel)onCancel();};setSize(480,330);
    }
    LfoSettings settings()const
    {return {shapes.getSelectedId()-1,sync.getToggleState(),values[0].getValue(),values[1].getValue(),values[2].getValue(),values[3].getValue(),values[4].getValue(),values[5].getValue(),values[6].getValue()};}
    std::function<void(LfoSettings)> onPreview,onApply;std::function<void()> onCancel;
    void changed(){values[1].setTooltip(sync.getToggleState()?"Cycles across the target range":"Frequency in Hz");if(onPreview)onPreview(settings());}
    void resized()override
    {auto r=getLocalBounds().reduced(8);auto top=r.removeFromTop(32);sync.setBounds(top.removeFromRight(130));shapes.setBounds(top);for(int i=0;i<7;++i){auto row=r.removeFromTop(35);labels[size_t(i)].setBounds(row.removeFromLeft(86));values[size_t(i)].setBounds(row);}auto bottom=r.removeFromBottom(30);cancel.setBounds(bottom.removeFromRight(90));apply.setBounds(bottom.removeFromRight(90));}
    juce::ComboBox shapes;juce::ToggleButton sync;std::array<juce::Slider,7> values;std::array<juce::Label,7> labels;juce::TextButton apply,cancel;
};
class LfoWindow final:public orbit::workspace::Window
{
public:
    LfoPanel panel;
    LfoWindow():Window("LFO Tool",juce::Colour(0xff203340)){setContentNonOwned(&panel,false);setBounds(120,120,500,370);setResizeLimits(400,360,650,500);}
    ~LfoWindow()override{clearContentComponent();}
    void closeButtonPressed()override{if(panel.onCancel)panel.onCancel();else Window::closeButtonPressed();}
};
}
