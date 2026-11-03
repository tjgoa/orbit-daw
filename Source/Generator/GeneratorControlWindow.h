#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Options/Appearance.h"
#include "../Editing/EditShortcuts.h"
#include "../Workspace/WorkspaceWindow.h"
#include "PatternToolbarLayout.h"

namespace orbit::generator
{
class ControlWindow final : public orbit::workspace::Window,private juce::Timer
{
public:
    ControlWindow(const juce::String& name,std::unique_ptr<juce::Component> body,
                  std::function<int()> read,std::function<void(int)> select,
                  std::function<void(juce::Component*)> settings,std::function<void()> roll,
                  std::function<void(juce::Rectangle<int>)> persist,juce::Rectangle<int> bounds)
        :Window(name,options::colour(45,65,77)),
         instrument(std::move(body)),readPattern(std::move(read)),selectPattern(std::move(select)),saveBounds(std::move(persist))
    {
        setAllowsWorkspaceOverflow(true);
        setUsingNativeTitleBar(false);setContentNonOwned(&content,false);
        for(int i=0;i<8;++i)
        {
            auto& bank=banks[size_t(i)];bank.setButtonText(juce::String::charToString(juce::juce_wchar('A'+i)));
            bank.onClick=[this,i]{const auto current=readPattern?readPattern():-1;if(current>=0 && selectPattern)selectPattern(i*8+current%8);refreshPattern();};content.addAndMakeVisible(bank);
            auto& slot=slots[size_t(i)];slot.setButtonText(juce::String(i+1));slot.onClick=[this,i]{const auto current=readPattern?readPattern():-1;if(current>=0 && selectPattern)selectPattern(current/8*8+i);refreshPattern();};content.addAndMakeVisible(slot);
            for(auto* button:{&bank,&slot}){button->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff287da2));button->setColour(juce::TextButton::textColourOnId,juce::Colours::white);}
        }
        menu.setButtonText("v");menu.onClick=[this,settings]{if(settings)settings(&menu);};juce::Component::addAndMakeVisible(menu);
        piano.setTooltip("Piano Roll");piano.onClick=std::move(roll);content.addAndMakeVisible(piano);
        for(auto* button:std::array<juce::Component*,6>{&solo,&mute,&up,&down,&open,&save})content.addAndMakeVisible(button);
        solo.setButtonText("S");mute.setButtonText("M");solo.setClickingTogglesState(true);mute.setClickingTogglesState(true);
        solo.setTooltip("Solo this generator");mute.setTooltip("Mute this generator");
        solo.onClick=[this]{if(writeSolo)writeSolo(solo.getToggleState());};mute.onClick=[this]{if(writeMute)writeMute(mute.getToggleState());};
        steps.setInputRestrictions(8,"0123456789");steps.setSelectAllWhenFocused(true);steps.setJustification(juce::Justification::centred);
        steps.onReturnKey=[this]{commitSteps();};steps.onFocusLost=[this]{commitSteps();};content.addAndMakeVisible(steps);
        up.setButtonText("+");down.setButtonText("-");up.onClick=[this]{if(readSteps && writeSteps)writeSteps(readSteps()+1);refreshPattern();};down.onClick=[this]{if(readSteps && writeSteps)writeSteps(juce::jmax(1,readSteps()-1));refreshPattern();};
        stepLabel.setText("steps:",juce::dontSendNotification);presetLabel.setText("preset:",juce::dontSendNotification);
        for(auto* label:{&stepLabel,&presetLabel}){label->setFont(11.f);content.addAndMakeVisible(label);}
        content.addAndMakeVisible(preset);preset.onChange=[this]{if(choosePreset)choosePreset(preset.getSelectedId()-1);};
        open.setButtonText("...");open.setTooltip("Open generator preset");save.setButtonText("Save");save.setTooltip("Save generator preset");
        open.onClick=[this]{if(openPreset)openPreset();};save.onClick=[this]{if(savePreset)savePreset();};
        content.addAndMakeVisible(*instrument);content.addMouseListener(this,true);setResizable(true,true);
        setResizeLimits(juce::jmax(480,juce::jmin(760,bounds.getWidth())),juce::jmin(640,bounds.getHeight()),2000,1400);
        if(const auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(bounds))
        {
            const auto safe=display->userArea;bounds.setSize(juce::jmin(bounds.getWidth(),safe.getWidth()),juce::jmin(bounds.getHeight(),safe.getHeight()));
            bounds.setX(juce::jlimit(safe.getX(),safe.getRight()-bounds.getWidth(),bounds.getX()));bounds.setY(juce::jlimit(safe.getY(),safe.getBottom()-bounds.getHeight(),bounds.getY()));
        }
        setBounds(bounds);refreshPattern();startTimerHz(10);
    }
    ~ControlWindow() override {stopTimer();if(patternView!=nullptr)content.removeChildComponent(patternView);if(saveBounds)saveBounds(floatingBounds());}
    void showControls(bool focus=true){setVisible(true);setMinimised(false);startTimerHz(10);refreshPattern();toFront(focus);}
    void hideControls(){if(saveBounds)saveBounds(floatingBounds());setVisible(false);stopTimer();orbit::workspace::traceUI("generator hidden: "+getName());notifyWorkspace();}
    void closeButtonPressed() override {if(patternView!=nullptr && onPatternClosed){onPatternClosed();return;}if(onDeleteRequested)onDeleteRequested();else hideControls();}
    std::function<void()> onDeleteRequested;
    void resized() override
    {
        Window::resized();menu.setBounds(3,2,20,20);auto area=content.getLocalBounds();auto header=area.removeFromTop(58);
        const PatternToolbarLayout layout(header);solo.setBounds(layout.solo);mute.setBounds(layout.mute);piano.setBounds(layout.piano);
        for(int i=0;i<8;++i){banks[size_t(i)].setBounds(layout.banks.getX()+i*20,layout.banks.getY(),19,16);slots[size_t(i)].setBounds(layout.banks.getX()+i*20,layout.banks.getY()+17,19,16);}
        stepLabel.setBounds(layout.stepsLabel);steps.setBounds(layout.steps);up.setBounds(layout.up);down.setBounds(layout.down);
        presetLabel.setBounds(layout.presetLabel);preset.setBounds(layout.preset);open.setBounds(layout.open);save.setBounds(layout.save);
        instrument->setBounds(area);if(patternView!=nullptr)patternView->setBounds(area);
    }
    void attachPatternView(juce::Component& view)
    {
        if(patternView==&view){showControls();return;}
        setMinimised(false);instrumentBounds=floatingBounds();instrumentWasMaximized=isMaximized();
        patternView=&view;instrument->setVisible(false);content.addAndMakeVisible(view);
        if(!isMaximized()){setSize(juce::jmax(900,getWidth()),juce::jmax(650,getHeight()));maximiseButtonPressed();}
        resized();notifyWorkspace();
    }
    void detachPatternView(bool restore=true)
    {
        if(patternView==nullptr)return;
        auto* old=patternView;patternView=nullptr;old->setVisible(false);content.removeChildComponent(old);
        instrument->setVisible(true);
        if(restore)
        {
            setMinimised(false);const auto position=floatingBounds().getPosition();
            if(isMaximized()!=instrumentWasMaximized)maximiseButtonPressed();
            if(!instrumentWasMaximized)setBounds(instrumentBounds.withPosition(position));
            constrainToWorkspace();resized();notifyWorkspace();
        }
    }
    bool isPatternView()const{return patternView!=nullptr;}
    juce::Rectangle<int> instrumentViewBounds()const{return isPatternView()?instrumentBounds:floatingBounds();}

    std::function<void()> onPatternClosed;
    void refreshPattern()
    {
        const auto current=readPattern?readPattern():-1;
        if(auto* close=getCloseButton())close->setTooltip(isPatternView()?"Return to instrument":"Delete generator...");
        if(readSteps && !steps.hasKeyboardFocus(true))steps.setText(juce::String(readSteps()).paddedLeft('0',3),false);
        if(readSolo)solo.setToggleState(readSolo(),juce::dontSendNotification);if(readMute)mute.setToggleState(readMute(),juce::dontSendNotification);
        if(readPreset && !preset.isPopupActive())preset.setText(readPreset(),juce::dontSendNotification);
        for(int i=0;i<8;++i){banks[size_t(i)].setToggleState(current>=0 && current/8==i,juce::dontSendNotification);slots[size_t(i)].setToggleState(current>=0 && current%8==i,juce::dontSendNotification);}
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto letter=editing::shortcutLetter(key);const auto mods=key.getModifiers();
        if(key.getKeyCode()==juce::KeyPress::spaceKey && !mods.isAnyModifierKeyDown() && onShortcut){onShortcut(key);return true;}
        if((mods.isCommandDown() || mods.isCtrlDown()) && !mods.isAltDown() && juce::String("zxcva").containsChar(letter) && onShortcut)
        {onShortcut(key);return true;}return Window::keyPressed(key);
    }
    void mouseDown(const juce::MouseEvent& e) override {if(onFocus)onFocus();Window::mouseDown(e);}
    std::function<void(const juce::KeyPress&)> onShortcut;
    std::function<void()> onFocus;
    std::function<void()> onRefresh;
    void commitSteps(){const int n=steps.getText().getIntValue();if(n>0 && writeSteps)writeSteps(n);refreshPattern();}
    void setPresetChoices(const juce::StringArray& names){preset.clear(juce::dontSendNotification);for(int i=0;i<names.size();++i)preset.addItem(names[i],i+1);refreshPattern();}
    std::function<int()> readSteps;
    std::function<void(int)> writeSteps,choosePreset;
    std::function<bool()> readSolo,readMute;
    std::function<void(bool)> writeSolo,writeMute;
    std::function<juce::String()> readPreset;
    std::function<void()> openPreset,savePreset;
    std::unique_ptr<juce::Component> instrument;
    juce::TextButton solo,mute,up,down;
    PresetIconButton open{false},save{true};
    PianoIconButton piano;
    juce::TextEditor steps;
    juce::ComboBox preset;
    juce::Label stepLabel,presetLabel;
    std::array<juce::TextButton,8> banks,slots;
private:
    void timerCallback() override
    {
        if (!isVisible()) return;
        refreshPattern();
        // Do not overwrite a local pad/velocity gesture while its button is held.
        if (onRefresh && !isPatternView() && !juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown())
            onRefresh();
    }
    juce::Component* patternView=nullptr;
    juce::Rectangle<int> instrumentBounds;bool instrumentWasMaximized=false;
    class Content final:public juce::Component
    {
    public:
        Content(){setOpaque(true);}
        void paint(juce::Graphics& g)override
        {
            g.fillAll(options::colour(45,65,77));
            g.setColour(options::colour(22,32,42));g.fillRect(getLocalBounds().removeFromTop(58));
        }
    } content;
    juce::TextButton menu;
    std::function<int()> readPattern;std::function<void(int)> selectPattern;
    std::function<void(juce::Rectangle<int>)> saveBounds;
};
}
