// Included inside PlaylistGrid. Selection belongs to the arrangement, not its
// per-bar hit-target buttons, which may be recycled while scrolling.
using ClipSelection=orbit::playlist::Selection;
std::function<std::vector<orbit::playlist::SelectionItem>()> selectableItems;
std::function<std::optional<ClipSelection>(const ClipSelection&,int,double,bool)> transformSelection;
std::function<bool(const ClipSelection&)> deleteSelection;
std::function<void(const orbit::playlist::ClipKey&)> renameClip;
std::optional<juce::Point<float>> hoveredClipPoint;
ClipSelection selectedClips;
std::vector<orbit::playlist::SelectionItem> selectionItems;
ClipSelection selectionBeforeMarquee;
bool selectionGesture=false,selectionRectangle=false,selectionMoved=false,selectionCopy=false;
juce::Point<float> selectionOrigin,selectionEnd;
int selectionRowDelta=0;double selectionBarDelta=0,selectionAnchorBar=0;
std::optional<orbit::playlist::ClipKey> selectionShiftToggle,selectionInitialHit;

juce::Rectangle<float> selectionItemBounds(const orbit::playlist::SelectionItem& item,int rows=0,double bars=0) const
{return {float(labelWidth()+(item.key.startBar+bars)*barWidth()),float(headerHeight()+(item.key.row+rows)*rowHeight()),float(item.lengthBars*barWidth()),float(rowHeight())};}

juce::Rectangle<float> selectedItemBounds(const orbit::playlist::SelectionItem& item,const orbit::playlist::ClipKey& key,int rows=0,double bars=0)const
{
    if(key.rangeStart>=0 && key.rangeEnd>key.rangeStart)
        return {float(labelWidth()+(key.rangeStart+bars)*barWidth()),float(headerHeight()+(item.key.row+rows)*rowHeight()),float((key.rangeEnd-key.rangeStart)*barWidth()),float(rowHeight())};
    return selectionItemBounds(item,rows,bars);
}

void clearClipSelection()
{selectedClips.clear();selectionGesture=false;selectionItems.clear();hoveredClipPoint.reset();repaint();}

bool selectionMouseDown(const juce::MouseEvent& e)
{
    if((automationMode) || (isPencilToolActive && isPencilToolActive()) || !e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()
        || e.x<labelWidth() || e.y<headerHeight())return false;
    grabKeyboardFocus();selectionItems=selectableItems?selectableItems():std::vector<orbit::playlist::SelectionItem>{};
    selectionOrigin=selectionEnd=e.position;selectionMoved=false;selectionRowDelta=0;selectionBarDelta=0;selectionShiftToggle.reset();
    selectionCopy=e.mods.isCtrlDown() || e.mods.isCommandDown();selectionGesture=true;
    const auto hit=std::find_if(selectionItems.rbegin(),selectionItems.rend(),[&](const auto& i){return selectionItemBounds(i).contains(e.position);});
    selectionInitialHit=hit==selectionItems.rend()?std::optional<orbit::playlist::ClipKey>{}:hit->key;
    const auto selected=hit==selectionItems.rend()?selectedClips.end():selectedClips.find(hit->key);
    const bool selectedHere=selected!=selectedClips.end() && selectedItemBounds(*hit,*selected).contains(e.position);
    selectionRectangle=hit==selectionItems.rend() || (isAudioTrack && isAudioTrack(hit->key.row) && !selectedHere);
    if(selectionRectangle)
    {selectionBeforeMarquee=e.mods.isShiftDown()?selectedClips:ClipSelection{};selectedClips=selectionBeforeMarquee;}
    else
    {
        selectionAnchorBar=selected!=selectedClips.end() && selected->rangeStart>=0?selected->rangeStart:hit->key.startBar;
        if(e.mods.isShiftDown()){if(selectedClips.contains(hit->key))selectionShiftToggle=hit->key;else selectedClips.insert(hit->key);}
        else if(!selectedClips.contains(hit->key))selectedClips={hit->key};
    }
    repaint();return true;
}

bool selectionMouseDrag(const juce::MouseEvent& e)
{
    if(!selectionGesture)return false;
    selectionEnd=e.position;selectionMoved|=e.getDistanceFromDragStart()>=5;
    if(selectionRectangle)
    {
        selectedClips=selectionBeforeMarquee;auto box=juce::Rectangle<float>(selectionOrigin,selectionEnd);box.setWidth(juce::jmax(1.f,box.getWidth()));box.setHeight(juce::jmax(1.f,box.getHeight()));
        if(selectionMoved)for(const auto& item:selectionItems)if(box.intersects(selectionItemBounds(item)))
        {
            auto key=item.key;
            if(isAudioTrack && isAudioTrack(key.row))
            {
                key.rangeStart=juce::jmax(key.startBar,snappedPosition(timelineBar(box.getX()),e.mods,true));
                key.rangeEnd=juce::jmin(key.startBar+item.lengthBars,snappedPosition(timelineBar(box.getRight()),e.mods,true));
                if(key.rangeEnd<=key.rangeStart+timelineTick()*.5)continue;
            }
            selectedClips.erase(key);selectedClips.insert(key);
        }
    }
    else
    {
        selectionRowDelta=int(std::floor((e.y-headerHeight())/double(rowHeight())))-int(std::floor((selectionOrigin.y-headerHeight())/rowHeight()));
        const bool audioOnly=isAudioTrack && std::all_of(selectedClips.begin(),selectedClips.end(),[this](const auto& key){return isAudioTrack(key.row);});
        selectionBarDelta=snappedMoveDelta(selectionAnchorBar,(e.position.x-selectionOrigin.x)/barWidth(),e.mods,audioOnly);
    }
    repaint();return true;
}

bool selectionMouseUp(const juce::MouseEvent& e)
{
    if(!selectionGesture)return false;
    selectionMouseDrag(e);const bool apply=!selectionRectangle && selectionMoved && (selectionRowDelta!=0 || selectionBarDelta!=0);
    const bool copy=selectionCopy || e.mods.isCtrlDown() || e.mods.isCommandDown();
    if(!selectionMoved && selectionRectangle && selectionInitialHit)selectedClips.insert(*selectionInitialHit);
    if(!selectionMoved && selectionShiftToggle)selectedClips.erase(*selectionShiftToggle);
    const auto selected=selectedClips;const int rows=selectionRowDelta;const double bars=selectionBarDelta;
    selectionGesture=false;selectionRowDelta=0;selectionBarDelta=0;
    if(apply && transformSelection)
        if(auto result=transformSelection(selected,rows,bars,copy))selectedClips=std::move(*result);
    repaint();return true;
}

void paintOverChildren(juce::Graphics& g) override
{
    paintTimelineMovePreview(g);
    if(selectedClips.empty() && !selectionGesture)return;
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(getLocalBounds().withTrimmedLeft(labelWidth()).withTrimmedTop(headerHeight()));
    const auto items=selectionGesture?selectionItems:(selectableItems?selectableItems():std::vector<orbit::playlist::SelectionItem>{});
    for(const auto& item:items)if(selectedClips.contains(item.key))
    {
        const auto& key=*selectedClips.find(item.key);const auto selectedBounds=selectedItemBounds(item,key);
        g.setColour(juce::Colours::gold.withAlpha(key.rangeStart>=0?.16f:0.f));g.fillRect(selectedBounds);
        g.setColour(juce::Colours::gold);g.drawRect(selectedBounds.reduced(1.0f),2.0f);
        if(selectionGesture && !selectionRectangle && selectionMoved)
        {g.setColour(juce::Colours::gold.withAlpha(0.32f));g.fillRect(selectedItemBounds(item,key,selectionRowDelta,selectionBarDelta).reduced(2.0f));}
    }
    if(selectionGesture && selectionRectangle && selectionMoved)
    {auto box=juce::Rectangle<float>(selectionOrigin,selectionEnd);box.setWidth(juce::jmax(1.f,box.getWidth()));box.setHeight(juce::jmax(1.f,box.getHeight()));g.setColour(juce::Colours::skyblue.withAlpha(0.15f));g.fillRect(box);g.setColour(juce::Colours::skyblue);g.drawRect(box,1.0f);}
}

bool keyPressed(const juce::KeyPress& key) override
{
    if(automationMode)return false;
    if(!key.getModifiers().isCtrlDown() && !key.getModifiers().isCommandDown() && !key.getModifiers().isAltDown()
        && (key.getTextCharacter()=='n' || key.getTextCharacter()=='N') && hoveredClipPoint && selectableItems && renameClip)
    {
        const auto items=selectableItems();
        const auto hit=std::find_if(items.rbegin(),items.rend(),[&](const auto& i){return selectionItemBounds(i).contains(*hoveredClipPoint);});
        if(hit!=items.rend()){renameClip(hit->key);return true;}
    }
    if(key.getModifiers().isShiftDown() && !key.getModifiers().isCtrlDown() && !key.getModifiers().isCommandDown()
        && (key.getTextCharacter()=='S' || key.getTextCharacter()=='s'))
    {followPlayCursor=!followPlayCursor;if(onViewOptionsChanged)onViewOptionsChanged();return true;}
    if(key==juce::KeyPress::escapeKey){clearClipSelection();return true;}
    if((key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown()) && !key.getModifiers().isAltDown() && orbit::editing::shortcutLetter(key)=='a')
    {selectedClips.clear();if(selectableItems)for(const auto& item:selectableItems())selectedClips.insert(item.key);repaint();return true;}
    if(key==juce::KeyPress::deleteKey || key==juce::KeyPress::backspaceKey)
    {if(!selectedClips.empty() && deleteSelection && deleteSelection(selectedClips))clearClipSelection();return true;}
    return false;
}
