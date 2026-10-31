#pragma once
#include "../Project/ProjectDocument.h"
#include "../Options/Appearance.h"

namespace orbit::synth
{
class AcidSwitchRows final : public juce::Component
{
public:
    std::function<int()> divisions,pixelsPerStep,scrollX,patternTicks;
    std::function<std::vector<ProjectDocument::PatternSwitch>()> getSwitches;
    std::function<void()> gestureBegin,gestureEnd;
    std::function<void(bool,int,int,bool)> changed;
    void paint(juce::Graphics& g) override
    {
        if(!divisions || !getSwitches)return;
        const auto values=getSwitches();const int span=juce::jmax(1,1024/divisions());
        const float width=pixelsPerStep()*span/64.0f;const int offset=scrollX(),ticks=patternTicks();
        g.fillAll(options::colour(27,43,54));
        g.saveState();g.reduceClipRegion(64,0,getWidth()-64,getHeight());
        const int first=juce::jmax(0,int(std::floor(offset/width))),last=int(std::ceil((offset+getWidth())/width));
        for(int cell=first;cell<=last;++cell)
            for(int row=0;row<2;++row)
            {
                const int tick=cell*span;const auto x=64+cell*width-offset;
                g.setColour(options::colour(65,88,101));g.drawRect(juce::Rectangle<float>(x,float(row*22),width,22),0.7f);
                bool on=false;for(const auto& value:values)
                    if(value.slide==(row==1) && value.startTick<tick+span && value.startTick+value.lengthTicks>tick){on=true;break;}
                if(tick<ticks)
                {
                    g.setColour(on?options::colour(76,205,224):options::colour(54,72,84));
                    const auto size=juce::jmax(1.0f,juce::jmin(8.0f,width-2));g.fillEllipse(x+(width-size)/2,row*22.0f+7,size,size);
                }
            }
        g.restoreState();g.setColour(options::colour(27,43,54));g.fillRect(0,0,64,getHeight());
        g.setColour(options::colour(214,235,243));g.setFont(12);
        g.drawText("Accent",3,0,60,22,juce::Justification::centredLeft);g.drawText("Slide",3,22,60,22,juce::Justification::centredLeft);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        if(e.x<64 || e.y<0 || e.y>=44 || !changed)return;
        dragRow=e.y>=22;const int span=juce::jmax(1,1024/divisions());
        const int cell=int((e.x-64+scrollX())*64.0/pixelsPerStep())/span;
        const int tick=cell*span;if(tick<0 || tick>=patternTicks())return;
        bool on=false;for(const auto& v:getSwitches())if(v.slide==dragRow && v.startTick<tick+span && v.startTick+v.lengthTicks>tick){on=true;break;}
        dragValue=!on;dragging=true;lastCell=cell;if(gestureBegin)gestureBegin();changed(dragRow,tick,span,dragValue);repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if(!dragging)return;const int span=juce::jmax(1,1024/divisions());
        const int cell=juce::jlimit(0,(patternTicks()-1)/span,int((e.x-64+scrollX())*64.0/pixelsPerStep())/span);
        for(int value=juce::jmin(lastCell,cell);value<=juce::jmax(lastCell,cell);++value)
            if(value!=lastCell)changed(dragRow,value*span,span,dragValue);
        lastCell=cell;repaint();
    }
    void mouseUp(const juce::MouseEvent&) override
    {if(dragging && gestureEnd)gestureEnd();dragging=false;}
private:
    bool dragging=false,dragRow=false,dragValue=false;int lastCell=0;
};
}
