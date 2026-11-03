#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
namespace orbit::workspace
{
inline juce::Component::SafePointer<juce::Component> dialogHost;
inline juce::File uiTraceFile;
inline void traceUI(const juce::String& message)
{
    if(uiTraceFile==juce::File{})return;
    uiTraceFile.getParentDirectory().createDirectory();
    uiTraceFile.appendText(juce::Time::getCurrentTime().toISO8601(true)+" "+message+"\n");
}
inline juce::Component* alertHost(juce::Component* associated)
{
    for(auto* c=associated;c!=nullptr;c=c->getParentComponent())if(bool(c->getProperties().getWithDefault("orbitDialogHost",false)))return c;
    return dialogHost.getComponent();
}
// An application-owned modal is a workspace child, not an unrelated desktop peer.
// Native plugin views are occluded by the same child ordering as other panels.
class OwnedAlert:public juce::AlertWindow
{
public:
    OwnedAlert(const juce::String& title,const juce::String& message,juce::MessageBoxIconType icon,juce::Component* associated=nullptr)
        :AlertWindow(title,message,icon,associated),host(alertHost(associated))
    {
        if(host!=nullptr){removeFromDesktop();host->addChildComponent(this);}
    }
    void prepare()
    {
        if(host!=nullptr){host->getTopLevelComponent()->toFront(true);centreAroundComponent(host,getWidth(),getHeight());setVisible(true);toFront(true);}
        traceUI("modal shown: "+getName()+" embedded="+juce::String(int(getParentComponent()!=nullptr)));
        if(onPresent)onPresent();
        toFront(true);
    }
    int runModalLoop()
    {
        prepare();const int result=juce::AlertWindow::runModalLoop();traceUI("modal result: "+getName()+" result="+juce::String(result));return result;
    }
    void enterModalState(bool focus=true,juce::ModalComponentManager::Callback* callback=nullptr,bool deleteOnExit=false)
    {
        prepare();const auto name=getName();
        juce::AlertWindow::enterModalState(focus,juce::ModalCallbackFunction::create([name,owned=std::shared_ptr<juce::ModalComponentManager::Callback>(callback)](int result)
        {traceUI("modal result: "+name+" result="+juce::String(result));if(owned)owned->modalStateFinished(result);}),deleteOnExit);
    }
    static void showAsync(juce::MessageBoxIconType icon,const juce::String& title,const juce::String& message)
    {
        auto* alert=new OwnedAlert(title,message,icon);alert->addButton("OK",1,juce::KeyPress(juce::KeyPress::returnKey));alert->enterModalState(true,nullptr,true);
    }
    inline static std::function<void()> onPresent;
private:juce::Component::SafePointer<juce::Component> host;
};
inline juce::DialogWindow* ownDialog(juce::DialogWindow* window,juce::Component* associated=nullptr)
{
    if(window==nullptr)return nullptr;
    if(auto* host=alertHost(associated))
    {
        window->removeFromDesktop();window->setUsingNativeTitleBar(false);window->setAlwaysOnTop(false);host->addAndMakeVisible(window);
        host->getTopLevelComponent()->toFront(true);window->centreAroundComponent(host,window->getWidth(),window->getHeight());window->toFront(true);
        traceUI("dialog shown: "+window->getName()+" embedded=1");if(OwnedAlert::onPresent)OwnedAlert::onPresent();window->toFront(true);
    }
    return window;
}

}
