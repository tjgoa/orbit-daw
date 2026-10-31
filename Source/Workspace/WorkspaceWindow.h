#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

namespace orbit::workspace
{
// A child document window: never owns an OS peer or minimizes the parent app.
class Window : public juce::DocumentWindow
{
public:
    class TitleButton:public juce::Button
    {
    public:
        explicit TitleButton(int t):Button("Workspace window control"),type(t){}
        void paintButton(juce::Graphics& g,bool over,bool down)override
        {
            auto r=getLocalBounds().toFloat().reduced(.5f);g.setColour(juce::Colour(over||down?0xff38677b:0xff203440));g.fillRoundedRectangle(r,2);g.setColour(juce::Colour(0xff7ca7ba));g.drawRoundedRectangle(r,2,1);g.setColour(juce::Colour(0xffd9ecf4));auto c=r.getCentre();
            if(type==juce::DocumentWindow::closeButton){g.drawLine(c.x-4,c.y-4,c.x+4,c.y+4,1.4f);g.drawLine(c.x-4,c.y+4,c.x+4,c.y-4,1.4f);}
            else if(type==juce::DocumentWindow::minimiseButton)g.drawLine(c.x-4,c.y+2,c.x+4,c.y+2,1.4f);
            else{g.drawRect(c.x-4,c.y-4,8.f,8.f,1.2f);g.drawLine(c.x-2,c.y-2,c.x+6,c.y-2,1);}
        }
    private:int type;
    };
    class Theme:public juce::LookAndFeel_V4
    {
    public:
        juce::Button* createDocumentWindowButton(int type)override{return new TitleButton(type);}
        void positionDocumentWindowButtons(juce::DocumentWindow&,int x,int y,int w,int h,juce::Button* min,juce::Button* max,juce::Button* close,bool)override
        {int right=x+w-3;for(auto* b:{close,max,min})if(b!=nullptr){right-=21;b->setBounds(right,y+(h-20)/2,20,20);right-=3;}}
    };
    Window(const juce::String& title,juce::Colour colour)
        :DocumentWindow(title,colour,allButtons,false)
    {
        setLookAndFeel(&theme);setUsingNativeTitleBar(false);setTitleBarHeight(24);
        setTitleBarButtonsRequired(allButtons,false);setResizable(true,true);
        setResizeLimits(320,180,4096,4096);
        getConstrainer()->setMinimumOnscreenAmounts(24,64,24,64);
    }
    ~Window()override{onWorkspaceChanged={};setLookAndFeel(nullptr);}
    std::function<void()> onWorkspaceChanged;
    juce::Rectangle<int> workspaceArea()const
    {return getParentComponent()!=nullptr?getParentComponent()->getLocalBounds().withTrimmedTop(82).withTrimmedBottom(int(getParentComponent()->getProperties().getWithDefault("orbitWorkspaceBottomInset",22))):juce::Rectangle<int>(0,82,1280,700);}
    void constrainToWorkspace()
    {
        if(adjusting || getParentComponent()==nullptr)return;
        juce::ScopedValueSetter<bool> guard(adjusting,true);
        auto area=workspaceArea();auto b=getBounds();
        if(maximized && !minimized)b=area;
        else {b.setSize(juce::jmin(b.getWidth(),area.getWidth()),juce::jmin(b.getHeight(),area.getHeight()));b.setPosition(juce::jlimit(area.getX(),area.getRight()-b.getWidth(),b.getX()),juce::jlimit(area.getY(),area.getBottom()-b.getHeight(),b.getY()));}
        if(b!=getBounds())setBounds(b);
    }
    void setMinimised(bool minimize)
    {
        if(minimize==minimized)return;
        if(minimize){beforeMinimize=getBounds();minimized=true;if(auto* c=getContentComponent())c->setVisible(false);setSize(getWidth(),24);}
        else {minimized=false;if(auto* c=getContentComponent())c->setVisible(true);setBounds(maximized?workspaceArea():beforeMinimize);}
        constrainToWorkspace();notifyWorkspace();
    }
    bool isMinimised()const{return minimized;}
    bool isMaximized()const{return maximized;}
    juce::Rectangle<int> floatingBounds()const{return maximized?restoreBounds:minimized?beforeMinimize:getBounds();}
    void minimiseButtonPressed()override{setMinimised(!minimized);}
    void maximiseButtonPressed()override
    {
        if(minimized)setMinimised(false);
        if(!maximized){restoreBounds=getBounds();maximized=true;setBounds(workspaceArea());}
        else {maximized=false;setBounds(restoreBounds);constrainToWorkspace();}
        notifyWorkspace();
    }
    void parentSizeChanged()override{constrainToWorkspace();notifyWorkspace();}
    void moved()override{DocumentWindow::moved();constrainToWorkspace();notifyWorkspace();}
    void resized()override{DocumentWindow::resized();constrainToWorkspace();notifyWorkspace();}
    void mouseDown(const juce::MouseEvent& e)override
    {
        toFront(false);dragging=!maximized && e.eventComponent==this && e.y<getTitleBarHeight() && e.mods.isLeftButtonDown();
        dragScreen=e.getScreenPosition();dragOrigin=getPosition();notifyWorkspace();
    }
    void mouseDrag(const juce::MouseEvent& e)override
    {if(dragging){setTopLeftPosition(dragOrigin+e.getScreenPosition()-dragScreen);constrainToWorkspace();notifyWorkspace();}}
    void mouseUp(const juce::MouseEvent&)override{dragging=false;}
    void closeButtonPressed()override{setVisible(false);notifyWorkspace();}
protected:
    void notifyWorkspace(){if(onWorkspaceChanged)onWorkspaceChanged();}
private:
    Theme theme;
    bool maximized=false,minimized=false,adjusting=false,dragging=false;
    juce::Point<int> dragScreen,dragOrigin;
    juce::Rectangle<int> restoreBounds,beforeMinimize;
};
}
