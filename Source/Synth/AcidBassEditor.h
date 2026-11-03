#pragma once
#include "AcidBassPlugin.h"
#include "../Options/Appearance.h"
#include "../Automation/ParameterEventSlider.h"
#include "../UI/AcidSilverLookAndFeel.h"

namespace orbit::synth
{
class AcidBassEditor final : public tracktion::engine::Plugin::EditorComponent,private juce::Timer
{
public:
    AcidBassEditor(AcidBassPlugin::Ptr pluginIn,std::function<void()> begin,std::function<void()> changed,std::function<void()> demo)
        :pluginOwner(pluginIn),plugin(dynamic_cast<AcidBassPlugin*>(pluginIn.get()))
    {
        setLookAndFeel(&silverLook);beginEdit=begin;changedEdit=changed;
        for(std::size_t i=0;i<knobs.size();++i)
        {
            labels[i].setText(AcidBassPlugin::names[parameterIndex(i)],juce::dontSendNotification);
            labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);
            auto& knob=knobs[i];knob.onEvents=[this,i](juce::Component* c){if(onEditEvents)onEditEvents(int(parameterIndex(i)),c);};knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
            knob.setPopupDisplayEnabled(true,false,this);
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
        effects.setTooltip("Show the original Drive row plus Tube and Two-Band distortion. Each added stage has its own enable and mix.");
        advanced.setButtonText("Advanced  +");advanced.setClickingTogglesState(true);
        advanced.onClick=[this]{setAdvancedExpanded(advanced.getToggleState());};addAndMakeVisible(advanced);
        modulation.setButtonText("LFO  +");modulation.setClickingTogglesState(true);
        modulation.setTooltip("Show the two independent LFO modulation routes.");
        modulation.onClick=[this]{setLfoExpanded(modulation.getToggleState());};addAndMakeVisible(modulation);
        spreadEnabled.setButtonText("Enable filter bank");
        spreadEnabled.setTooltip("One oscillator feeding 2, 4 or 8 independent filters, without detune. More filters increase CPU use; Off restores the original single-filter path.");
        spreadEnabled.onClick=[this,begin,changed]{if(begin)begin();auto& p=plugin->parameters[AcidBassPlugin::spreadEnableIndex];p->parameterChangeGestureBegin();p->setParameter(spreadEnabled.getToggleState()?1.f:0.f,juce::sendNotification);p->parameterChangeGestureEnd();if(changed)changed();refreshAdvanced();};
        addChildComponent(spreadEnabled);voices.addItemList({"2 filters","4 filters","8 filters"},1);
        voices.setTooltip("Independent filters sharing the same oscillator, accent, envelope and slide. Eight filters use more CPU.");
        voices.onChange=[this,begin,changed]{if(voices.getSelectedId()<1)return;if(begin)begin();auto& p=plugin->parameters[AcidBassPlugin::spreadVoicesIndex];p->parameterChangeGestureBegin();p->setParameter(float(voices.getSelectedId()-1),juce::sendNotification);p->parameterChangeGestureEnd();if(changed)changed();};addChildComponent(voices);
        for(size_t n=0;n<spreadKnobs.size();++n)
        {
            const auto index=AcidBassPlugin::spreadAmountIndex+n;auto& k=spreadKnobs[n];
            spreadLabels[n].setText(n==0?"Filter Spread":"Stereo Width",juce::dontSendNotification);spreadLabels[n].setJustificationType(juce::Justification::centred);addChildComponent(spreadLabels[n]);
            k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);k.setPopupDisplayEnabled(true,false,this);k.setRange(0,100);k.setDoubleClickReturnValue(true,0);k.setTextValueSuffix(" %");
            k.onEvents=[this,index](juce::Component* c){if(onEditEvents)onEditEvents(int(index),c);};
            k.onDragStart=[this,n,index,begin]{spreadGestures[n]=true;if(begin)begin();plugin->parameters[index]->parameterChangeGestureBegin();};
            k.onDragEnd=[this,n,index]{spreadGestures[n]=false;plugin->parameters[index]->parameterChangeGestureEnd();};
            k.onValueChange=[this,n,index,begin,changed]{if(!spreadGestures[n] && begin)begin();plugin->parameters[index]->setParameter(float(spreadKnobs[n].getValue()),juce::sendNotification);if(changed)changed();};
            k.setTooltip(n==0?"Spread filter cutoffs logarithmically around Cutoff. 100% spans four octaves before the filter's frequency limits. Pitch is unchanged.":"Pan the filter voices from left to right. 0% is mono. With zero Filter Spread the identical voices remain mono.");addChildComponent(k);
        }
        const std::array<const char*,12> extraNames{"Drive","Lo Cut","Hi Damp","Shape","Level","Dry/Wet","Breakpoint","Low Drive","High Drive","Hi Tone","Level","Dry/Wet"};
        for(size_t i=0;i<extraKnobs.size();++i)
        {
            const size_t parameter=i<6?22+i:29+i-6;
            bindKnob(extraKnobs[i],parameter);
            extraLabels[i].setText(extraNames[i],juce::dontSendNotification);extraLabels[i].setJustificationType(juce::Justification::centred);addChildComponent(extraLabels[i]);addChildComponent(extraKnobs[i]);
        }
        for(size_t row=0;row<2;++row)
        {
            auto& toggle=stageEnabled[row];toggle.setButtonText(row==0?"Tube":"Two-Band");
            toggle.onClick=[this,row]{setParameter(row==0?21:28,stageEnabled[row].getToggleState()?1.f:0.f);};addChildComponent(toggle);
            const size_t base=35+row*5;
            lfoEnabled[row].setButtonText("LFO "+juce::String(int(row+1)));
            lfoEnabled[row].onClick=[this,row,base]{setParameter(base,lfoEnabled[row].getToggleState()?1.f:0.f);};addChildComponent(lfoEnabled[row]);
            lfoWave[row].addItemList({"Sine","Triangle","Saw","Square"},1);
            lfoTarget[row].addItemList(AcidBassPlugin::modulationNames(),1);
            lfoWave[row].onChange=[this,row,base]{if(lfoWave[row].getSelectedId()>0)setParameter(base+3,float(lfoWave[row].getSelectedId()-1));};
            lfoTarget[row].onChange=[this,row,base]{if(lfoTarget[row].getSelectedId()>0)setParameter(base+4,float(lfoTarget[row].getSelectedId()-1));};
            lfoTrigger[row].addItemList({"Free-running","Retrigger on note"},1);
            lfoTrigger[row].setTooltip("Free-running shares one sweep across notes and loops. Retrigger restarts this LFO on every MIDI note-on, including slides and legato.");
            lfoTrigger[row].onChange=[this,row]{if(lfoTrigger[row].getSelectedId()>0)setParameter(AcidBassPlugin::lfoRetriggerIndex+row,float(lfoTrigger[row].getSelectedId()-1));};addChildComponent(lfoTrigger[row]);
            lfoWave[row].setTooltip("Choose Free-running to span notes/loops, or Retrigger on note. Play/Stop or seeking restarts phase. The two routes add at a shared destination.");
            lfoTarget[row].setTooltip("Modulates around the saved knob/automation value. Off disables this route.");
            addChildComponent(lfoWave[row]);addChildComponent(lfoTarget[row]);
            for(size_t k=0;k<2;++k){const size_t n=row*2+k;bindKnob(lfoKnobs[n],base+1+k);lfoLabels[n].setText(k==0?"Rate Hz":"Depth +/-",juce::dontSendNotification);lfoLabels[n].setJustificationType(juce::Justification::centred);addChildComponent(lfoLabels[n]);addChildComponent(lfoKnobs[n]);}
        }
        advancedHint.setText("Shared pitch; no detune. More filters increase CPU use.",juce::dontSendNotification);addChildComponent(advancedHint);
        lfoHint.setText("Free-running or note retrigger per route. Depth 100% spans +/-50% of the control range.",juce::dontSendNotification);addChildComponent(lfoHint);
        equalizer.setButtonText("EQ  +");equalizer.setClickingTogglesState(true);equalizer.onClick=[this]{setEqExpanded(equalizer.getToggleState());};addAndMakeVisible(equalizer);
        eqEnabled.setButtonText("Enable EQ");eqEnabled.onClick=[this]{setParameter(AcidBassPlugin::eqIndex,eqEnabled.getToggleState()?1.f:0.f);};addChildComponent(eqEnabled);
        const std::array<const char*,8> eqNames{"Low Freq","Low Gain","Mid Freq","Mid Gain","Mid Q","High Freq","High Gain","Output"};
        for(size_t n=0;n<8;++n){bindKnob(eqKnobs[n],AcidBassPlugin::eqIndex+1+n);eqLabels[n].setText(eqNames[n],juce::dontSendNotification);eqLabels[n].setJustificationType(juce::Justification::centred);addChildComponent(eqKnobs[n]);addChildComponent(eqLabels[n]);}
        timerCallback();setEqExpanded(false);setEffectsExpanded(false);setAdvancedExpanded(false);setLfoExpanded(false);setSize(880,190);startTimerHz(20);
    }
    ~AcidBassEditor() override{stopTimer();setLookAndFeel(nullptr);}
    bool allowWindowResizing() override{return false;}
    juce::ComponentBoundsConstrainer* getBoundsConstrainer() override{return nullptr;}
    void setEffectsExpanded(bool expanded)
    {
        const bool change=effectsExpanded!=expanded;effectsExpanded=expanded;effects.setToggleState(expanded,juce::dontSendNotification);
        effects.setButtonText(expanded?"Effects / Drive  -":"Effects / Drive  +");
        for(size_t i=8;i<knobs.size();++i){knobs[i].setVisible(expanded);labels[i].setVisible(expanded);}mode.setVisible(expanded);
        for(size_t i=0;i<extraKnobs.size();++i){extraKnobs[i].setVisible(expanded);extraLabels[i].setVisible(expanded);}for(auto& toggle:stageEnabled)toggle.setVisible(expanded);
        if(onEffectsExpanded){if(change)onEffectsExpanded(effectsExpanded || advancedExpanded || lfoExpanded || eqExpanded);}else setSize(getWidth(),preferredHeight());
        resized();repaint();
    }
    void setAdvancedExpanded(bool expanded)
    {
        const bool change=advancedExpanded!=expanded;advancedExpanded=expanded;advanced.setToggleState(expanded,juce::dontSendNotification);advanced.setButtonText(expanded?"Advanced  -":"Advanced  +");
        for(size_t n=0;n<spreadKnobs.size();++n){spreadKnobs[n].setVisible(expanded);spreadLabels[n].setVisible(expanded);}voices.setVisible(expanded);spreadEnabled.setVisible(expanded);advancedHint.setVisible(expanded);
        if(onEffectsExpanded){if(change)onEffectsExpanded(effectsExpanded || advancedExpanded || lfoExpanded || eqExpanded);}else setSize(getWidth(),preferredHeight());
        resized();repaint();
    }
    void setLfoExpanded(bool expanded)
    {
        const bool change=lfoExpanded!=expanded;lfoExpanded=expanded;modulation.setToggleState(expanded,juce::dontSendNotification);modulation.setButtonText(expanded?"LFO  -":"LFO  +");lfoHint.setVisible(expanded);
        for(size_t i=0;i<lfoKnobs.size();++i){lfoKnobs[i].setVisible(expanded);lfoLabels[i].setVisible(expanded);}for(size_t i=0;i<2;++i){lfoEnabled[i].setVisible(expanded);lfoWave[i].setVisible(expanded);lfoTarget[i].setVisible(expanded);lfoTrigger[i].setVisible(expanded);}
        if(onEffectsExpanded){if(change)onEffectsExpanded(effectsExpanded || advancedExpanded || lfoExpanded || eqExpanded);}else setSize(getWidth(),preferredHeight());
        resized();repaint();
    }
    void setEqExpanded(bool expanded)
    {
        const bool change=eqExpanded!=expanded;eqExpanded=expanded;equalizer.setToggleState(expanded,juce::dontSendNotification);equalizer.setButtonText(expanded?"EQ  -":"EQ  +");eqEnabled.setVisible(expanded);
        for(size_t n=0;n<8;++n){eqKnobs[n].setVisible(expanded);eqLabels[n].setVisible(expanded);}
        if(onEffectsExpanded){if(change)onEffectsExpanded(effectsExpanded || advancedExpanded || lfoExpanded || eqExpanded);}else setSize(getWidth(),preferredHeight());resized();repaint();
    }
    int preferredHeight()const{return 190+(effectsExpanded?350:0)+(advancedExpanded?130:0)+(lfoExpanded?200:0)+(eqExpanded?140:0);}
    void paint(juce::Graphics& g) override
    {
        if(appearance==0)g.setGradientFill(juce::ColourGradient(juce::Colour(248,249,251),0,0,juce::Colour(202,209,219),0,float(getHeight()),false));else g.setColour(juce::Colour(30,44,54));
        g.fillAll();g.setColour(appearance==0?juce::Colour(35,45,58):juce::Colour(173,221,232));g.setFont(17);
        g.drawText("ACID BASS",juce::Rectangle<int>(14,juce::jmin(152,getHeight()-38),140,30),juce::Justification::centredLeft);
        if(effectsExpanded){g.setColour(appearance==0?juce::Colour(158,169,183):juce::Colour(70,101,114));g.drawHorizontalLine(190,8.f,float(getWidth()-8));}
    }
    void resized() override
    {
        auto area=getLocalBounds().reduced(8);auto synth=area.removeFromTop(174);
        auto bottom=synth.removeFromBottom(30);
        demoButton.setBounds(bottom.removeFromRight(130).reduced(2));wave.setBounds(bottom.removeFromRight(115).reduced(2));
        effects.setBounds(bottom.removeFromRight(150).reduced(2));equalizer.setBounds(bottom.removeFromRight(70).reduced(2));modulation.setBounds(bottom.removeFromRight(90).reduced(2));advanced.setBounds(bottom.removeFromRight(115).reduced(2));
        const int width=synth.getWidth()/8;
        for(size_t i=0;i<8;++i){auto column=synth.removeFromLeft(width);auto title=column.removeFromTop(24);knobs[i].setBounds(column.reduced(3,0));const bool white=ui::effectiveKnobStyle(knobs[i],appearance==1?1:2)==2;labels[i].setBounds(white?title.translated(0,28):title);}
        if(effectsExpanded)
        {
            auto fx=getLocalBounds().withTrimmedTop(198).withHeight(114).reduced(8,0);mode.setBounds(fx.removeFromRight(168).withHeight(30).translated(0,50).reduced(2));
            const int fxWidth=fx.getWidth()/5;
            for(size_t i=8;i<knobs.size();++i){auto column=fx.removeFromLeft(fxWidth);labels[i].setBounds(column.removeFromTop(24));knobs[i].setBounds(column.reduced(3,0));}
            for(size_t row=0;row<2;++row)
            {
                auto area=getLocalBounds().withTrimmedTop(320+int(row)*108).withHeight(104).reduced(8,0);
                stageEnabled[row].setBounds(area.removeFromLeft(96).withHeight(30).translated(0,34));const int cell=area.getWidth()/6;
                for(size_t col=0;col<6;++col){const size_t i=row*6+col;auto box=area.removeFromLeft(cell);extraLabels[i].setBounds(box.removeFromTop(22));extraKnobs[i].setBounds(box.reduced(10,0));}
            }
        }
        if(advancedExpanded)
        {
            auto row=getLocalBounds().withTrimmedTop(198+(effectsExpanded?350:0)).reduced(10,0).withHeight(112);
            auto controls=row.removeFromLeft(juce::jmin(400,row.getWidth()/2));
            spreadEnabled.setBounds(controls.removeFromTop(28));voices.setBounds(controls.removeFromTop(30).withWidth(150));advancedHint.setBounds(controls.removeFromTop(50));
            const int width=row.getWidth()/2;
            for(size_t n=0;n<2;++n){auto column=row.removeFromLeft(width);spreadLabels[n].setBounds(column.removeFromTop(24));spreadKnobs[n].setBounds(column.reduced(4,0));}
        }
        if(lfoExpanded)
        {
            const int top=190+(effectsExpanded?350:0)+(advancedExpanded?130:0);
            lfoHint.setBounds(12,top+4,getWidth()-24,18);
            for(size_t l=0;l<2;++l)
            {
                auto strip=getLocalBounds().withTrimmedTop(top+24+int(l)*84).withHeight(82).reduced(12,0);
                lfoEnabled[l].setBounds(strip.removeFromLeft(72).withHeight(28).translated(0,18));
                lfoWave[l].setBounds(strip.removeFromLeft(108).withHeight(28).translated(0,18));strip.removeFromLeft(8);
                lfoTarget[l].setBounds(strip.removeFromLeft(174).withHeight(28).translated(0,18));strip.removeFromLeft(8);
                lfoTrigger[l].setBounds(strip.removeFromLeft(150).withHeight(28).translated(0,18));
                const int cell=strip.getWidth()/2;
                for(size_t k=0;k<2;++k){const size_t n=l*2+k;auto box=strip.removeFromLeft(cell);lfoLabels[n].setBounds(box.removeFromTop(16));lfoKnobs[n].setBounds(box.withSizeKeepingCentre(76,64));}
            }
        }
        if(eqExpanded)
        {
            auto row=getLocalBounds().withTrimmedTop(198+(effectsExpanded?350:0)+(advancedExpanded?130:0)+(lfoExpanded?200:0)).withHeight(118).reduced(8,0);
            eqEnabled.setBounds(row.removeFromLeft(80).withHeight(28).translated(0,38));const int cell=row.getWidth()/8;
            for(size_t n=0;n<8;++n){auto box=row.removeFromLeft(cell);eqLabels[n].setBounds(box.removeFromTop(22));eqKnobs[n].setBounds(box.reduced(1,0));}
        }
    }
    void refreshAppearance()
    {
        getProperties().set("orbitKnobOverride",juce::jlimit(-1,2,int(plugin->state.getProperty("orbitKnobOverride",-1))));
        const int next=juce::jlimit(0,2,int(plugin->state.getProperty("orbitAcidSkin",0)));
        if(next!=appearance){appearance=next;silverLook.skin=next;silverLook.setDark(next!=0);sendLookAndFeelChange();repaint();}
        const int style=ui::effectiveKnobStyle(knobs[0],next==1?1:2);if(style!=labelKnobStyle){labelKnobStyle=style;resized();}
    }
    std::function<void(int,juce::Component*)> onEditEvents;
    std::function<void(bool)> onEffectsExpanded;
    std::array<automation::ParameterEventSlider,13> knobs;juce::TextButton wave,demoButton,effects;juce::ComboBox mode;
    juce::TextButton advanced,modulation,equalizer;juce::ToggleButton eqEnabled;std::array<automation::ParameterEventSlider,8> eqKnobs;juce::ToggleButton spreadEnabled;juce::ComboBox voices;
    std::array<automation::ParameterEventSlider,2> spreadKnobs;
    std::array<automation::ParameterEventSlider,12> extraKnobs;
    std::array<automation::ParameterEventSlider,4> lfoKnobs;
    std::array<juce::ToggleButton,2> stageEnabled,lfoEnabled;
    std::array<juce::ComboBox,2> lfoWave,lfoTarget,lfoTrigger;
private:
    int appearance=-1,labelKnobStyle=-1;
    ui::AcidSilverLookAndFeel silverLook;
    std::function<void()> beginEdit,changedEdit;
    std::array<juce::Label,8> eqLabels;std::array<juce::Label,12> extraLabels;std::array<juce::Label,4> lfoLabels;
    void setParameter(size_t index,float value)
    {if(beginEdit)beginEdit();auto& p=plugin->parameters[index];p->parameterChangeGestureBegin();p->setParameter(value,juce::sendNotification);p->parameterChangeGestureEnd();if(changedEdit)changedEdit();}
    void bindKnob(automation::ParameterEventSlider& k,size_t index)
    {
        auto& p=plugin->parameters[index];k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);k.setPopupDisplayEnabled(true,false,this);
        k.setRange(AcidBassPlugin::minima[index],AcidBassPlugin::maxima[index],0);k.setSkewFactor(p->valueRange.skew);k.setDoubleClickReturnValue(true,AcidBassPlugin::defaults[index]);k.setValue(p->getCurrentValue(),juce::dontSendNotification);
        k.textFromValueFunction=[p,index](double v){return index==25?juce::String(v,2):p->valueToString(float(v))+p->getLabel();};
        if(index==25)k.setTooltip("Continuous shape morph: 0 = Symmetric, 1 = Tube A, 2 = Tube B. Values between them blend the curves.");
        k.onEvents=[this,index](juce::Component* c){if(onEditEvents)onEditEvents(int(index),c);};
        k.onDragStart=[this,index]{if(beginEdit)beginEdit();plugin->parameters[index]->parameterChangeGestureBegin();};
        k.onDragEnd=[this,index]{plugin->parameters[index]->parameterChangeGestureEnd();};
        k.onValueChange=[this,index,&k]{if(!k.isMouseButtonDown() && beginEdit)beginEdit();plugin->parameters[index]->setParameter(float(k.getValue()),juce::sendNotification);if(changedEdit)changedEdit();};
    }
    void refreshAdvanced()
    {
        const bool enabled=plugin->parameters[AcidBassPlugin::spreadEnableIndex]->getCurrentValue()>=.5f;
        spreadEnabled.setToggleState(enabled,juce::dontSendNotification);voices.setEnabled(enabled);voices.setSelectedId(juce::jlimit(1,3,1+juce::roundToInt(plugin->parameters[AcidBassPlugin::spreadVoicesIndex]->getCurrentValue())),juce::dontSendNotification);
        for(size_t n=0;n<2;++n){spreadKnobs[n].setEnabled(enabled);if(!spreadKnobs[n].isMouseButtonDown())spreadKnobs[n].setValue(plugin->parameters[AcidBassPlugin::spreadAmountIndex+n]->getCurrentValue(),juce::dontSendNotification);}
    }
    void refreshMode(){mode.setSelectedId(plugin->parameters[AcidBassPlugin::triangleFoldIndex]->getCurrentValue()>=.5f?3:plugin->parameters[14]->getCurrentValue()>=.5f?2:1,juce::dontSendNotification);}
    void timerCallback() override
    {
        refreshAppearance();
        for(std::size_t i=0;i<knobs.size();++i)if(!knobs[i].isMouseButtonDown())knobs[i].setValue(plugin->parameters[parameterIndex(i)]->getCurrentValue(),juce::dontSendNotification);
        wave.setToggleState(plugin->parameters[8]->getCurrentValue()>=0.5f,juce::dontSendNotification);
        wave.setButtonText(wave.getToggleState()?"Square":"Saw");
        for(size_t i=0;i<extraKnobs.size();++i)if(!extraKnobs[i].isMouseButtonDown())extraKnobs[i].setValue(plugin->parameters[i<6?22+i:29+i-6]->getCurrentValue(),juce::dontSendNotification);
        for(size_t row=0;row<2;++row){stageEnabled[row].setToggleState(plugin->parameters[row==0?21:28]->getCurrentValue()>=.5f,juce::dontSendNotification);const size_t base=35+row*5;lfoTrigger[row].setSelectedId(1+juce::roundToInt(plugin->parameters[AcidBassPlugin::lfoRetriggerIndex+row]->getCurrentValue()),juce::dontSendNotification);lfoEnabled[row].setToggleState(plugin->parameters[base]->getCurrentValue()>=.5f,juce::dontSendNotification);lfoWave[row].setSelectedId(1+juce::roundToInt(plugin->parameters[base+3]->getCurrentValue()),juce::dontSendNotification);lfoTarget[row].setSelectedId(1+juce::roundToInt(plugin->parameters[base+4]->getCurrentValue()),juce::dontSendNotification);for(size_t k=0;k<2;++k)if(!lfoKnobs[row*2+k].isMouseButtonDown())lfoKnobs[row*2+k].setValue(plugin->parameters[base+1+k]->getCurrentValue(),juce::dontSendNotification);}
        eqEnabled.setToggleState(plugin->parameters[AcidBassPlugin::eqIndex]->getCurrentValue()>=.5f,juce::dontSendNotification);
        for(size_t n=0;n<8;++n)if(!eqKnobs[n].isMouseButtonDown())eqKnobs[n].setValue(plugin->parameters[AcidBassPlugin::eqIndex+1+n]->getCurrentValue(),juce::dontSendNotification);
        refreshMode();refreshAdvanced();
    }
    tracktion::engine::Plugin::Ptr pluginOwner;
    AcidBassPlugin* plugin;
    static constexpr size_t parameterIndex(size_t knob){return knob<8?knob:knob+1;}
    bool effectsExpanded=false,advancedExpanded=false,lfoExpanded=false,eqExpanded=false;
    juce::Label advancedHint,lfoHint;std::array<juce::Label,2> spreadLabels;std::array<bool,2> spreadGestures{};
    std::array<juce::Label,13> labels;std::array<bool,13> gestures{};
};
}
