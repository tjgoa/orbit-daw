// Modified Orbit port. GPL-3.0 + additional terms in ThirdParty/9W9/NOTICE.
// 9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9
#pragma once
#include "NineKitPlugin.h"
#include "../Options/Appearance.h"
#include "../Automation/ParameterEventSlider.h"
namespace orbit::synth
{
class NineKitEditor final : public tracktion::engine::Plugin::EditorComponent,private juce::Timer
{
    class StepButton final:public juce::TextButton
    {
    public:std::function<void()> onVelocity;
        void mouseDown(const juce::MouseEvent& e)override{if(e.mods.isPopupMenu()){if(onVelocity)onVelocity();return;}juce::TextButton::mouseDown(e);}
    };
public:
    struct Actions
    {
        std::function<void()> begin,changed,demo;
        std::function<void(int)> preview,length;
        std::function<int()> readLength;
        std::function<int(int,int)> velocity;
        std::function<void(int,int,int)> step;
        std::function<bool(int)> readAccent;
        std::function<void(int,bool)> accent;
    };
    NineKitEditor(NineKitPlugin::Ptr owner,Actions actions):pluginOwner(owner),plugin(dynamic_cast<NineKitPlugin*>(owner.get())),act(std::move(actions))
    {
        for(size_t i=0;i<tabs.size();++i)
        {
            styleSwitch(tabs[i],options::colour(37,112,143));
            tabs[i].setButtonText(nine::pages[i].name);tabs[i].onClick=[this,i]{setPage(int(i));if(i<11 && act.preview)act.preview(int(i));};addAndMakeVisible(tabs[i]);
        }
        for(size_t i=0;i<knobs.size();++i)
        {
            auto& k=knobs[i];k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);k.setPopupDisplayEnabled(true,false,this);
            labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);addAndMakeVisible(k);
            k.textFromValueFunction=[this,i](double value){int p=parameterAt(i);if(p<0)return juce::String();const auto& s=nine::specs[size_t(p)];if(s.choices[0]){auto choices=juce::StringArray::fromTokens(s.choices,"|","");return choices[juce::jlimit(0,choices.size()-1,juce::roundToInt(value))];}return juce::String(juce::roundToInt(value));};
            k.onDragStart=[this,i]{int p=parameterAt(i);if(p<0)return;gestures[i]=true;if(act.begin)act.begin();plugin->parameters[size_t(p)]->parameterChangeGestureBegin();};
            k.onDragEnd=[this,i]{int p=parameterAt(i);if(p>=0)plugin->parameters[size_t(p)]->parameterChangeGestureEnd();gestures[i]=false;};
            k.onValueChange=[this,i]{int p=parameterAt(i);if(p<0)return;if(!gestures[i] && act.begin)act.begin();plugin->parameters[size_t(p)]->setParameter(float(knobs[i].getValue()),juce::sendNotification);if(act.changed)act.changed();};
            k.onEvents=[this,i](juce::Component* c){int p=parameterAt(i);if(p>=0 && onEditEvents)onEditEvents(p,c);};
        }
        for(int i=0;i<16;++i)
        {
            styleSwitch(steps[size_t(i)],options::colour(37,132,174));
            styleSwitch(accents[size_t(i)],options::colour(177,105,42));
            steps[size_t(i)].onClick=[this,i]{int tick=page*16+i;const int v=act.velocity?act.velocity(drum,tick):0;if(act.step)act.step(drum,tick,v?0:100);if(!v && act.preview)act.preview(drum);refresh();};
            steps[size_t(i)].onVelocity=[this,i]{showVelocity(i);};addAndMakeVisible(steps[size_t(i)]);
            accents[size_t(i)].setButtonText("A");accents[size_t(i)].onClick=[this,i]{int tick=page*16+i;if(act.accent)act.accent(tick,!(act.readAccent && act.readAccent(tick)));refresh();};addAndMakeVisible(accents[size_t(i)]);
        }
        for(int n:{16,32,64,128,256})length.addItem(juce::String(n)+" steps",n);length.onChange=[this]{if(act.length)act.length(length.getSelectedId());refresh();};addAndMakeVisible(length);
        previous.setButtonText("<");next.setButtonText(">");previous.onClick=[this]{page=juce::jmax(0,page-1);refresh();};next.onClick=[this]{++page;refresh();};addAndMakeVisible(previous);addAndMakeVisible(next);
        demo.setButtonText("Demo beat");demo.onClick=act.demo;addAndMakeVisible(demo);hit.setButtonText("Hear hit");hit.onClick=[this]{if(act.preview)act.preview(drum);};addAndMakeVisible(hit);
        credit.setButtonText("Credits");credit.onClick=[] {juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon,"9W9 credits","9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9\n\nModified for Orbit: native host/UI, sequencer integration, compiled sample resources and sample-rate correction. This version is not the original and is not made or endorsed by A Thousand Details. GPL-3.0 and additional terms: see included NOTICE.");};addAndMakeVisible(credit);
        setSize(900,430);setPage(0);startTimerHz(15);
    }
    bool allowWindowResizing()override{return true;}
    juce::ComponentBoundsConstrainer* getBoundsConstrainer()override{return nullptr;}
    int parameterAt(size_t i)const{const auto& p=nine::pages[size_t(selectedPage)].parameters;return i<p.size()?p[i]:-1;}
    void setPage(int p)
    {
        selectedPage=juce::jlimit(0,13,p);if(selectedPage<11)drum=selectedPage;
        for(size_t i=0;i<knobs.size();++i)
        {
            int index=parameterAt(i);bool show=index>=0;knobs[i].setVisible(show);labels[i].setVisible(show);if(!show)continue;
            const auto& s=nine::specs[size_t(index)];labels[i].setText(s.name,juce::dontSendNotification);knobs[i].setRange(0,s.maximum,1);knobs[i].setDoubleClickReturnValue(true,plugin->defaults[size_t(index)]);
            knobs[i].setValue(plugin->parameters[size_t(index)]->getCurrentValue(),juce::dontSendNotification);knobs[i].setTooltip(s.choices[0]?juce::String(s.choices):"0-127 panel control. Right-click for events; double-click resets.");
        }
        refresh();resized();repaint();
    }
    void paint(juce::Graphics& g)override
    {
        g.fillAll(options::colour(30,44,54));g.setColour(options::colour(173,221,232));g.setFont(16);g.drawText("9W9 909 AUDITION",12,4,260,28,juce::Justification::centredLeft);
        g.setFont(12);g.drawText("Click a drum to preview. Steps edit that drum; right-click a step for velocity. Accent applies to the whole kit.",12,getHeight()-23,getWidth()-24,20,juce::Justification::centredLeft);
        g.drawText(nine::pages[size_t(drum)].name,12,stepsY,82,30,juce::Justification::centredLeft);g.drawText("Accent",12,stepsY+34,82,28,juce::Justification::centredLeft);
        g.drawText("MIDI "+juce::String(plugin->noteForVoice(drum))+" | Steps "+juce::String(page*16+1)+"-"+juce::String(juce::jmin(page*16+16,stepCount())),12,stepsY-30,300,24,juce::Justification::centredLeft);
    }
    void resized()override
    {
        auto area=getLocalBounds().reduced(10);auto top=area.removeFromTop(32);credit.setBounds(top.removeFromRight(80).reduced(2));demo.setBounds(top.removeFromRight(110).reduced(2));hit.setBounds(top.removeFromRight(100).reduced(2));
        auto row=area.removeFromTop(60);int tw=row.getWidth()/7;for(int i=0;i<14;++i)tabs[size_t(i)].setBounds(row.getX()+(i%7)*tw,row.getY()+(i/7)*29,tw-3,27);
        auto controls=area.removeFromTop(140);int width=controls.getWidth()/8;for(size_t i=0;i<knobs.size();++i){auto col=controls.removeFromLeft(width);labels[i].setBounds(col.removeFromTop(24));knobs[i].setBounds(col);}
        area.removeFromBottom(24);auto nav=area.removeFromTop(32);next.setBounds(nav.removeFromRight(32).reduced(2));previous.setBounds(nav.removeFromRight(32).reduced(2));length.setBounds(nav.removeFromRight(120).reduced(2));
        stepsY=area.getY();int sw=juce::jmax(1,(area.getWidth()-86)/16);for(int i=0;i<16;++i){steps[size_t(i)].setBounds(96+i*sw,stepsY,sw-2,30);accents[size_t(i)].setBounds(96+i*sw,stepsY+34,sw-2,26);}repaint();
    }
    std::function<void(int,juce::Component*)> onEditEvents;
    std::array<automation::ParameterEventSlider,8> knobs;
    std::array<juce::TextButton,14> tabs;std::array<StepButton,16> steps;std::array<juce::TextButton,16> accents;
    juce::TextButton demo,hit,previous,next,credit;juce::ComboBox length;
private:
    static void styleSwitch(juce::TextButton& button,juce::Colour active)
    {
        button.setColour(juce::TextButton::buttonColourId,options::colour(38,56,68));
        button.setColour(juce::TextButton::buttonOnColourId,active);
        button.setColour(juce::TextButton::textColourOffId,options::colour(225,239,247));
        button.setColour(juce::TextButton::textColourOnId,juce::Colours::white);
        button.setWantsKeyboardFocus(false);
    }
    int stepCount()const{return act.readLength?juce::jmax(1,act.readLength()):16;}
    void timerCallback()override{if(isShowing())refresh();}
    void refresh()
    {
        const int total=stepCount();page=juce::jlimit(0,(total-1)/16,page);length.setSelectedId(total,juce::dontSendNotification);previous.setEnabled(page>0);next.setEnabled((page+1)*16<total);
        for(size_t i=0;i<tabs.size();++i)tabs[i].setToggleState(int(i)==selectedPage,juce::dontSendNotification);
        for(size_t i=0;i<knobs.size();++i){int p=parameterAt(i);if(p>=0 && !gestures[i])knobs[i].setValue(plugin->parameters[size_t(p)]->getCurrentValue(),juce::dontSendNotification);}
        for(int i=0;i<16;++i){int step=page*16+i;bool valid=step<total;int velocity=valid && act.velocity?act.velocity(drum,step):0;auto& b=steps[size_t(i)];b.setButtonText(juce::String(step+1));b.setEnabled(valid);b.setToggleState(velocity>0,juce::dontSendNotification);b.setTooltip("MIDI "+juce::String(plugin->noteForVoice(drum))+" | Velocity "+juce::String(velocity)+". Right-click to set velocity.");accents[size_t(i)].setEnabled(valid);accents[size_t(i)].setToggleState(valid && act.readAccent && act.readAccent(step),juce::dontSendNotification);}repaint();
    }
    void showVelocity(int cell)
    {
        int step=page*16+cell,voice=drum;juce::PopupMenu menu;for(int v:{32,64,96,127})menu.addItem(v,"Velocity "+juce::String(v));menu.addItem(128,"Remove note");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&steps[size_t(cell)]),[weak=juce::Component::SafePointer<NineKitEditor>(this),step,voice](int result){if(weak && result>0){if(weak->act.step)weak->act.step(voice,step,result==128?0:result);weak->refresh();}});
    }
    tracktion::engine::Plugin::Ptr pluginOwner;NineKitPlugin* plugin;Actions act;
    std::array<juce::Label,8> labels;std::array<bool,8> gestures{};int selectedPage=0,drum=0,page=0,stepsY=0;
};
}
