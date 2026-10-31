// Included inside PlaylistGrid. Selection belongs to the arrangement, not its
// per-bar hit-target buttons, which may be recycled while scrolling.
using ClipSelection=orbit::playlist::Selection;
std::function<std::vector<orbit::playlist::SelectionItem>()> selectableItems;
std::function<std::optional<ClipSelection>(const ClipSelection&,int,int,bool)> transformSelection;
std::function<bool(const ClipSelection&)> deleteSelection;
std::function<void(const orbit::playlist::ClipKey&)> renameClip;
std::optional<juce::Point<float>> hoveredClipPoint;
ClipSelection selectedClips;
std::vector<orbit::playlist::SelectionItem> selectionItems;
ClipSelection selectionBeforeMarquee;
bool selectionGesture=false,selectionRectangle=false,selectionMoved=false,selectionCopy=false;
juce::Point<float> selectionOrigin,selectionEnd;
int selectionRowDelta=0,selectionBarDelta=0;

juce::Rectangle<float> selectionItemBounds(const orbit::playlist::SelectionItem& item,int rows=0,int bars=0) const
{return {float(labelWidth()+(item.key.startBar+bars)*barWidth()),float(headerHeight()+(item.key.row+rows)*rowHeight()),float(item.lengthBars*barWidth()),float(rowHeight())};}

void clearClipSelection()
{selectedClips.clear();selectionGesture=false;selectionItems.clear();hoveredClipPoint.reset();repaint();}

bool selectionMouseDown(const juce::MouseEvent& e)
{
    if((automationMode) || (isPencilToolActive && isPencilToolActive()) || !e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()
        || e.x<labelWidth() || e.y<headerHeight())return false;
    grabKeyboardFocus();selectionItems=selectableItems?selectableItems():std::vector<orbit::playlist::SelectionItem>{};
    selectionOrigin=selectionEnd=e.position;selectionMoved=false;selectionRowDelta=selectionBarDelta=0;
    selectionCopy=e.mods.isCtrlDown() || e.mods.isCommandDown();selectionGesture=true;
    const auto hit=std::find_if(selectionItems.rbegin(),selectionItems.rend(),[&](const auto& i){return selectionItemBounds(i).contains(e.position);});
    selectionRectangle=hit==selectionItems.rend();
    if(selectionRectangle)
    {selectionBeforeMarquee=e.mods.isShiftDown()?selectedClips:ClipSelection{};selectedClips=selectionBeforeMarquee;}
    else if(e.mods.isShiftDown())
    {if(selectedClips.contains(hit->key))selectedClips.erase(hit->key);else selectedClips.insert(hit->key);selectionGesture=false;}
    else if(!selectedClips.contains(hit->key))selectedClips={hit->key};
    repaint();return true;
}

bool selectionMouseDrag(const juce::MouseEvent& e)
{
    if(!selectionGesture)return false;
    selectionEnd=e.position;selectionMoved|=e.getDistanceFromDragStart()>=5;
    if(selectionRectangle)
    {
        selectedClips=selectionBeforeMarquee;const juce::Rectangle<float> box(selectionOrigin,selectionEnd);
        if(selectionMoved)for(const auto& item:selectionItems)if(box.intersects(selectionItemBounds(item)))selectedClips.insert(item.key);
    }
    else
    {
        selectionRowDelta=int(std::floor((e.y-headerHeight())/double(rowHeight())))-int(std::floor((selectionOrigin.y-headerHeight())/rowHeight()));
        selectionBarDelta=int(std::floor((e.x-labelWidth())/double(barWidth())))-int(std::floor((selectionOrigin.x-labelWidth())/barWidth()));
    }
    repaint();return true;
}

bool selectionMouseUp(const juce::MouseEvent& e)
{
    if(!selectionGesture)return false;
    selectionMouseDrag(e);const bool apply=!selectionRectangle && selectionMoved && (selectionRowDelta!=0 || selectionBarDelta!=0);
    const bool copy=selectionCopy || e.mods.isCtrlDown() || e.mods.isCommandDown();
    const auto selected=selectedClips;const int rows=selectionRowDelta,bars=selectionBarDelta;
    selectionGesture=false;selectionRowDelta=selectionBarDelta=0;
    if(apply && transformSelection)
        if(auto result=transformSelection(selected,rows,bars,copy))selectedClips=std::move(*result);
    repaint();return true;
}

void paintOverChildren(juce::Graphics& g) override
{
    if(selectedClips.empty() && !selectionGesture)return;
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(getLocalBounds().withTrimmedLeft(labelWidth()).withTrimmedTop(headerHeight()));
    const auto items=selectionGesture?selectionItems:(selectableItems?selectableItems():std::vector<orbit::playlist::SelectionItem>{});
    for(const auto& item:items)if(selectedClips.contains(item.key))
    {
        g.setColour(juce::Colours::gold);g.drawRect(selectionItemBounds(item).reduced(2.0f),2.0f);
        if(selectionGesture && !selectionRectangle && selectionMoved)
        {g.setColour(juce::Colours::gold.withAlpha(0.32f));g.fillRect(selectionItemBounds(item,selectionRowDelta,selectionBarDelta).reduced(2.0f));}
    }
    if(selectionGesture && selectionRectangle && selectionMoved)
    {const juce::Rectangle<float> box(selectionOrigin,selectionEnd);g.setColour(juce::Colours::skyblue.withAlpha(0.15f));g.fillRect(box);g.setColour(juce::Colours::skyblue);g.drawRect(box,1.0f);}
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
