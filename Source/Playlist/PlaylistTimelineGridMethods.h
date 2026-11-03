// Included inside PlaylistGrid. All pointer hit tests use actual clip extents,
// never the whole-bar accessibility adapters.
std::function<double()> stepsPerBarProvider;
double stepsInBar()const{return stepsPerBarProvider?stepsPerBarProvider():16.;}
int snapIntervalSteps()const{return 1 << juce::jlimit(0,6,snapDivision-1);}
void setSnapIntervalSteps(int steps)
{int index=0;while(index<6 && (1<<index)<steps)++index;snapDivision=index+1;if(onViewOptionsChanged)onViewOptionsChanged();repaint();}
double timelineTick()const{return 1.0/(ProjectDocument::ticksPerQuarterNote*stepsInBar()/4.0);}
double roundTimeline(double bar)const{return std::round(bar/timelineTick())*timelineTick();}
double snappedPosition(double bar,const juce::ModifierKeys& mods,bool nearest=false)const
{
    const bool snap=snapToGrid!=mods.isShiftDown();
    const double interval=mods.isAltDown()?1.0:snap?snapIntervalSteps()/stepsInBar():timelineTick();
    return (nearest || !snap || mods.isAltDown()?std::round(bar/interval):std::floor(bar/interval+1.e-8))*interval;
}
double snappedMoveDelta(double anchor,double delta,const juce::ModifierKeys& mods,bool audioOnly=false)const
{
    if(audioOnly)
    {
        if(!mods.isAltDown() && !(snapToGrid!=mods.isShiftDown()))return delta;
        return snappedPosition(anchor+delta,mods,true)-anchor;
    }
    return roundTimeline(snappedPosition(anchor+delta,mods,true)-anchor);
}
double timelineBar(float x)const{return (x-labelWidth())/barWidth();}
std::optional<orbit::playlist::SelectionItem> timelineHit(juce::Point<float> point)const
{
    if(!selectableItems || point.x<labelWidth() || point.y<headerHeight())return {};
    const auto items=selectableItems();
    for(auto i=items.rbegin();i!=items.rend();++i)if(selectionItemBounds(*i).contains(point))return *i;
    return {};
}
juce::Component timelinePopupAnchor;
juce::Component* timelineMenuAnchor(juce::Point<float> point)
{
    if(timelinePopupAnchor.getParentComponent()!=this){addAndMakeVisible(timelinePopupAnchor);timelinePopupAnchor.setInterceptsMouseClicks(false,false);}
    timelinePopupAnchor.setBounds(juce::roundToInt(point.x),juce::roundToInt(point.y),1,1);return &timelinePopupAnchor;
}
std::function<std::pair<double,double>(const orbit::playlist::ClipKey&)> audioTrimLimits;
std::function<bool(const orbit::playlist::ClipKey&,double,double)> trimAudio;
enum class TimelineGesture {none,paint,move,resize,trimLeft,trimRight,erase,cycle,automation};
TimelineGesture timelineGesture=TimelineGesture::none;
std::optional<orbit::playlist::SelectionItem> timelineSource;
juce::Point<float> timelineOrigin;
int timelineRow=0,timelineCycleStep=0,timelineMoveRows=0;
double timelinePaintStart=0,timelineEnd=0,timelineMoveBars=0,timelineTrimStart=0;
bool timelineMoved=false,timelineCopy=false;

void timelineAutomation(const juce::MouseEvent& e)
{
    const int row=int(std::floor((e.position.y-headerHeight())/rowHeight()));
    if(!setSongAutomationPoint || !trackCount || row<0 || row>=trackCount() || e.position.x<labelWidth())return;
    const int step=int(std::floor(timelineBar(e.position.x)*stepsInBar()));
    const float value=1.f-juce::jlimit(0.f,1.f,float(std::fmod(e.position.y-headerHeight(),rowHeight())/rowHeight()));
    if(step>=0 && step<barCount*stepsInBar())setSongAutomationPoint(row,step,value);
    repaint();
}
bool timelineMouseDown(const juce::MouseEvent& e)
{
    if(!selectableItems || e.position.x<labelWidth() || e.position.y<headerHeight())return false;
    const int row=int(std::floor((e.position.y-headerHeight())/rowHeight()));
    if(!trackCount || row<0 || row>=trackCount())return false;
    grabKeyboardFocus();timelineSource=timelineHit(e.position);timelineOrigin=e.position;
    timelineRow=row;timelineMoved=false;timelineCopy=e.mods.isCtrlDown() || e.mods.isCommandDown();timelineMoveRows=0;timelineMoveBars=0;timelineCycleStep=0;
    if(e.mods.isRightButtonDown())
    {
        if(isAudioTrack && isAudioTrack(row)){if(audioContextMenu)audioContextMenu(row,timelineBar(e.position.x),timelineMenuAnchor(e.position));return true;}
        timelineGesture=TimelineGesture::erase;
        if(timelineSource && cellDeleted)cellDeleted(row,timelineSource->key.startBar);
        return true;
    }
    if(!e.mods.isLeftButtonDown())return false;
    if(automationMode){timelineGesture=TimelineGesture::automation;timelineAutomation(e);return true;}
    if(timelineSource)
    {
        const auto bounds=selectionItemBounds(*timelineSource);const float edge=juce::jmax(1.f,juce::jmin(8.f,bounds.getWidth()/4));
        const bool audio=isAudioTrack && isAudioTrack(row);
        if((audio && e.position.x<=bounds.getX()+edge) || e.position.x>=bounds.getRight()-edge)
        {
            timelineGesture=audio?(e.position.x<=bounds.getX()+edge?TimelineGesture::trimLeft:TimelineGesture::trimRight):TimelineGesture::resize;
            timelineTrimStart=timelineSource->key.startBar;timelineEnd=timelineTrimStart+timelineSource->lengthBars;return true;
        }
    }
    if(selectionMouseDown(e))return true;
    if(timelineSource)
    {
        const auto bounds=selectionItemBounds(*timelineSource);
        const bool audio=isAudioTrack && isAudioTrack(row);
        const float resizeWidth=juce::jmax(1.f,juce::jmin(8.f,bounds.getWidth()/4));
        if(!audio && showPatternLabels && e.position.y>=bounds.getBottom()-14 && e.position.x<bounds.getRight()-resizeWidth && !e.mods.isAnyModifierKeyDown())
        {if(showPatternMenu)showPatternMenu(row,timelineSource->key.startBar,timelineMenuAnchor(e.position));return true;}
        timelineGesture=!audio && e.position.x>=bounds.getRight()-resizeWidth?TimelineGesture::resize:TimelineGesture::move;
        timelineEnd=timelineSource->lengthBars;
    }
    else
    {
        timelineGesture=TimelineGesture::paint;
        timelinePaintStart=juce::jmax(0.,snappedPosition(timelineBar(e.position.x),e.mods));timelineEnd=timelinePaintStart;
    }
    return true;
}
bool timelineMouseDrag(const juce::MouseEvent& e)
{
    if(timelineGesture==TimelineGesture::none)return false;
    const float dx=e.position.x-timelineOrigin.x,dy=e.position.y-timelineOrigin.y;
    timelineMoved|=e.getDistanceFromDragStart()>=5;
    if(timelineGesture==TimelineGesture::automation){timelineAutomation(e);return true;}
    if(timelineGesture==TimelineGesture::erase)
    {if(auto hit=timelineHit(e.position);hit && cellDeleted)cellDeleted(hit->key.row,hit->key.startBar);return true;}
    if(timelineGesture==TimelineGesture::paint)
    {
        timelineEnd=juce::jlimit(0.,double(barCount)-timelineTick(),snappedPosition(timelineBar(e.position.x),e.mods));
        if(timelineMoved)setPatternRangePaintPreview(timelineRow,timelinePaintStart,timelineEnd);
    }
    else if((timelineGesture==TimelineGesture::trimLeft || timelineGesture==TimelineGesture::trimRight) && timelineSource)
    {
        const auto limits=audioTrimLimits?audioTrimLimits(timelineSource->key):std::pair<double,double>{0,double(barCount)};
        const double minimum=juce::jmax(timelineTick(),.001),start=timelineSource->key.startBar,end=start+timelineSource->lengthBars;
        if(timelineGesture==TimelineGesture::trimLeft)timelineTrimStart=juce::jlimit(limits.first,juce::jmax(limits.first,end-minimum),snappedPosition(start+dx/barWidth(),e.mods,true));
        else timelineEnd=juce::jlimit(juce::jmin(start+minimum,limits.second),limits.second,snappedPosition(end+dx/barWidth(),e.mods,true));
    }
    else if(timelineGesture==TimelineGesture::resize && timelineSource)
    {
        const double end=snappedPosition(timelineSource->key.startBar+timelineSource->lengthBars+dx/barWidth(),e.mods,true);
        const double minimum=juce::jmin(timelineSource->lengthBars,1.0);
        timelineEnd=juce::jlimit(minimum,juce::jmax(minimum,barCount-timelineSource->key.startBar),end-timelineSource->key.startBar);
        setPatternResizePreview(timelineRow,timelineSource->key.startBar,timelineEnd);
    }
    else if(timelineSource)
    {
        const bool audio=isAudioTrack && isAudioTrack(timelineRow);
        if(!audio && pencilActive() && !timelineCopy && timelineGesture!=TimelineGesture::cycle && std::abs(dy)>=8 && std::abs(dy)>std::abs(dx))
        {timelineGesture=TimelineGesture::cycle;if(onPatternCycleGesture)onPatternCycleGesture(true);}
        if(timelineGesture==TimelineGesture::cycle)
        {
            const int next=-int(dy)/12,difference=next-timelineCycleStep;
            if(scrollPattern)for(int n=0;n<juce::jmin(64,std::abs(difference));++n)scrollPattern(timelineRow,timelineSource->key.startBar,difference>0?1:-1);
            timelineCycleStep=next;
        }
        else
        {
            timelineMoveRows=int(std::floor((e.position.y-headerHeight())/rowHeight()))-timelineRow;
            timelineMoveBars=snappedMoveDelta(timelineSource->key.startBar,dx/barWidth(),e.mods,audio);
        }
    }
    repaint();return true;
}
bool timelineMouseUp(const juce::MouseEvent& e)
{
    if(timelineGesture==TimelineGesture::none)return false;
    timelineMouseDrag(e);const auto gesture=timelineGesture;timelineGesture=TimelineGesture::none;
    clearPatternResizePreview();clearPatternRangePaintPreview();
    if(gesture==TimelineGesture::paint)
    {if(timelineMoved && paintPatternRange)paintPatternRange(timelineRow,timelinePaintStart,timelineEnd);else if(cellPainted)cellPainted(timelineRow,timelinePaintStart);if(onPaintFinished)onPaintFinished();}
    else if((gesture==TimelineGesture::trimLeft || gesture==TimelineGesture::trimRight) && timelineSource)
    {if(timelineMoved && trimAudio)trimAudio(timelineSource->key,timelineTrimStart,timelineEnd);}
    else if(gesture==TimelineGesture::resize && timelineSource)
    {if(timelineMoved && resizePatternClip)resizePatternClip(timelineRow,timelineSource->key.startBar,timelineEnd);}
    else if(gesture==TimelineGesture::move && timelineSource)
    {
        if(timelineMoved && (timelineMoveRows!=0 || timelineMoveBars!=0) && transformSelection)
        {if(auto result=transformSelection({timelineSource->key},timelineMoveRows,timelineMoveBars,timelineCopy || e.mods.isCtrlDown() || e.mods.isCommandDown()))selectedClips=*result;}
        else if(!timelineMoved && cellClicked)cellClicked(timelineRow,timelineSource->key.startBar);
    }
    else if(gesture==TimelineGesture::cycle){if(onPatternCycleGesture)onPatternCycleGesture(false);}
    else if(gesture==TimelineGesture::automation){if(onPaintFinished)onPaintFinished();}
    timelineSource.reset();repaint();return true;
}
void paintTimelineMovePreview(juce::Graphics& g)
{
    if((timelineGesture==TimelineGesture::trimLeft || timelineGesture==TimelineGesture::trimRight) && timelineSource && timelineMoved)
    {
        g.setColour(juce::Colours::skyblue.withAlpha(.25f));const auto source=*timelineSource;
        g.fillRect(float(labelWidth()+timelineTrimStart*barWidth()),float(headerHeight()+source.key.row*rowHeight()),float((timelineEnd-timelineTrimStart)*barWidth()),float(rowHeight()));return;
    }
    if(timelineGesture!=TimelineGesture::move || !timelineSource || !timelineMoved)return;
    juce::Graphics::ScopedSaveState state(g);g.reduceClipRegion(getLocalBounds().withTrimmedLeft(labelWidth()).withTrimmedTop(headerHeight()));
    g.setColour(juce::Colours::gold.withAlpha(.32f));g.fillRect(selectionItemBounds(*timelineSource,timelineMoveRows,timelineMoveBars));
}
