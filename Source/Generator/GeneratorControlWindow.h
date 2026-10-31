#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Options/Appearance.h"
#include "../Editing/EditShortcuts.h"
#include "../Workspace/WorkspaceWindow.h"

namespace orbit::generator
{
class ControlWindow final : public orbit::workspace::Window,private juce::Timer
{
public:
    ControlWindow(const juce::String& name,std::unique_ptr<juce::Component> body,
                  std::function<int()> read,std::function<void(int)> select,
                  std::function<void(juce::Component*)> settings,std::function<void()> roll,
                  std::function<void(juce::Rectangle<int>)> save,juce::Rectangle<int> bounds)
        :Window(name,options::colour(23,38,48)),
         instrument(std::move(body)),readPattern(std::move(read)),selectPattern(std::move(select)),saveBounds(std::move(save))
    {
        setUsingNativeTitleBar(false);setContentNonOwned(&content,false);
        for(int i=0;i<8;++i)
        {
            auto& bank=banks[size_t(i)];bank.setButtonText(juce::String::charToString(juce::juce_wchar('A'+i)));
            bank.onClick=[this,i]{const auto current=readPattern?readPattern():-1;if(current>=0 && selectPattern)selectPattern(i*8+current%8);refreshPattern();};content.addAndMakeVisible(bank);
            auto& slot=slots[size_t(i)];slot.setButtonText(juce::String(i+1));slot.onClick=[this,i]{const auto current=readPattern?readPattern():-1;if(current>=0 && selectPattern)selectPattern(current/8*8+i);refreshPattern();};content.addAndMakeVisible(slot);
        }
        menu.setButtonText("v");menu.onClick=[this,settings]{if(settings)settings(&menu);};content.addAndMakeVisible(menu);
        piano.setButtonText("Piano Roll");piano.onClick=std::move(roll);content.addAndMakeVisible(piano);
        content.addAndMakeVisible(*instrument);content.addMouseListener(this,true);setResizable(true,true);
        setResizeLimits(juce::jmin(760,bounds.getWidth()),juce::jmin(640,bounds.getHeight()),2000,1400);
        if(const auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(bounds))
        {
            const auto safe=display->userArea;bounds.setSize(juce::jmin(bounds.getWidth(),safe.getWidth()),juce::jmin(bounds.getHeight(),safe.getHeight()));
            bounds.setX(juce::jlimit(safe.getX(),safe.getRight()-bounds.getWidth(),bounds.getX()));bounds.setY(juce::jlimit(safe.getY(),safe.getBottom()-bounds.getHeight(),bounds.getY()));
        }
        setBounds(bounds);refreshPattern();startTimerHz(10);
    }
    ~ControlWindow() override {stopTimer();if(saveBounds)saveBounds(floatingBounds());}
    void showControls(bool focus=true){setVisible(true);setMinimised(false);startTimerHz(10);refreshPattern();toFront(focus);}
    void closeButtonPressed() override {if(saveBounds)saveBounds(floatingBounds());setVisible(false);stopTimer();notifyWorkspace();}
    void resized() override
    {
        Window::resized();auto area=content.getLocalBounds();auto header=area.removeFromTop(58);
        menu.setBounds(header.getX()+4,header.getY()+4,25,48);piano.setBounds(header.getRight()-110,header.getY()+15,104,28);
        const int width=juce::jlimit(20,32,(header.getWidth()-160)/8);
        for(int i=0;i<8;++i){banks[size_t(i)].setBounds(34+i*width,4,width-2,23);slots[size_t(i)].setBounds(34+i*width,30,width-2,23);}
        instrument->setBounds(area);
    }
    void refreshPattern()
    {
        const auto current=readPattern?readPattern():-1;
        for(int i=0;i<8;++i){banks[size_t(i)].setToggleState(current>=0 && current/8==i,juce::dontSendNotification);slots[size_t(i)].setToggleState(current>=0 && current%8==i,juce::dontSendNotification);}
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        const auto letter=editing::shortcutLetter(key);const auto mods=key.getModifiers();
        if((mods.isCommandDown() || mods.isCtrlDown()) && !mods.isAltDown() && juce::String("zxcva").containsChar(letter) && onShortcut)
        {onShortcut(key);return true;}return Window::keyPressed(key);
    }
    void mouseDown(const juce::MouseEvent& e) override {if(onFocus)onFocus();Window::mouseDown(e);}
    std::function<void(const juce::KeyPress&)> onShortcut;
    std::function<void()> onFocus;
    std::function<void()> onRefresh;
    std::unique_ptr<juce::Component> instrument;
    std::array<juce::TextButton,8> banks,slots;
private:
    void timerCallback() override
    {
        if (!isVisible()) return;
        refreshPattern();
        // Do not overwrite a local pad/velocity gesture while its button is held.
        if (onRefresh && !juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown())
            onRefresh();
    }
    juce::Component content;juce::TextButton menu,piano;
    std::function<int()> readPattern;std::function<void(int)> selectPattern;
    std::function<void(juce::Rectangle<int>)> saveBounds;
};
}
