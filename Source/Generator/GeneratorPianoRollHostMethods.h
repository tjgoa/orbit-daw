// One active note editor is physically parented to its owning generator window.
juce::String hostedPianoRollTrackId;
bool changingPianoRollHost=false;
juce::String pianoEditingTrackId()const
{return hostedPianoRollTrackId.isNotEmpty()?hostedPianoRollTrackId:selectedProjectTrackId();}
int pianoEditingPatternIndex()const
{return hostedPianoRollTrackId.isNotEmpty()?project.getTrackSelectedPatternIndex(hostedPianoRollTrackId):selectedPatternIndex();}
juce::String pianoRollTitle()const
{
    if(hostedPianoRollTrackId.isEmpty())return getSelectedPatternTitle();
    const int slot=pianoEditingPatternIndex();juce::String name;
    for(int row=0;row<project.getTrackCount();++row)if(project.getTrackId(row)==hostedPianoRollTrackId)name=project.getTrackName(row);
    return name+"  ·  Pattern "+juce::String::charToString(juce::juce_wchar('A'+slot/8))+juce::String(slot%8+1);
}
void activatePianoRollOwner()
{
    if(changingPianoRollHost || hostedPianoRollTrackId.isEmpty() || selectedProjectTrackId()==hostedPianoRollTrackId)return;
    const juce::ScopedValueSetter<bool> guard(changingPianoRollHost,true);selectGeneratorControlTrack(hostedPianoRollTrackId);
}
void releaseHostedPianoRoll(bool restore=true)
{
    if(hostedPianoRollTrackId.isEmpty() || pianoRollWindow==nullptr)return;
    const auto id=hostedPianoRollTrackId;hostedPianoRollTrackId.clear();
    if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end())found->second.window->detachPatternView(restore);
    pianoRollWindow->setVisible(false);pianoRollWindow->setGeneratorHosted(false);generatorBody.addChildComponent(*pianoRollWindow);
    independentControlsMode=true;pianoRollMaximized=false;
    orbit::workspace::traceUI("generator view: instrument "+id);syncGeneratorNativeViews();
}
void openHostedPianoRoll(const juce::String& id)
{
    if(!requireUnfrozenGenerator(id))return;
    if(pianoRollWindow==nullptr || !canOpenGeneratorControls(id))return;
    if(hostedPianoRollTrackId==id)
    {selectGeneratorControlTrack(id);pianoRollWindow->setPatternContext(id,pianoEditingPatternIndex());pianoRollWindow->refresh(pianoRollTitle());if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end())found->second.window->showControls();pianoRollWindow->focusGrid();return;}
    releaseHostedPianoRoll();selectGeneratorControlTrack(id);openGeneratorControls(id,false);
    auto found=generatorControlWindows.find(id);if(found==generatorControlWindows.end())return;
    hostedPianoRollTrackId=id;independentControlsMode=true;allGeneratorsMinimized=false;generatorCollapsed=false;
    pianoRollWindow->setGeneratorHosted(true);pianoRollWindow->setPatternContext(id,pianoEditingPatternIndex());
    loadSelectedPianoPreferences();refreshPianoRollAutomationTargets();pianoRollWindow->refresh(pianoRollTitle());
    found->second.window->attachPatternView(*pianoRollWindow);pianoRollMaximized=found->second.window->isMaximized();
    generatorSurface.setVisible(false);generatorBody.setVisible(false);found->second.window->toFront(true);pianoRollWindow->focusGrid();
    orbit::workspace::traceUI("generator view: piano roll "+id);syncGeneratorNativeViews();
}
void toggleHostedPianoRoll(const juce::String& id)
{if(hostedPianoRollTrackId==id)closePianoRollView();else openHostedPianoRoll(id);}
void closeProjectViews()
{
    releaseHostedPianoRoll(false);
    if(playlistWindow!=nullptr){playlistWindow->setMinimized(false);playlistWindow->setVisible(false);}
    if(mixerWindow!=nullptr){mixerWindow->setMinimized(false);mixerWindow->setVisible(false);}
    if(songEventWindow!=nullptr){songEventWindow->setMinimised(false);songEventWindow->setVisible(false);}
    if(lfoWindow!=nullptr){lfoWindow->setMinimised(false);lfoWindow->setVisible(false);}
    if(equaliserWindow!=nullptr)equaliserWindow->setVisible(false);
    if(audioSettingsWindow!=nullptr)audioSettingsWindow->setVisible(false);
    if(samplePathsWindow!=nullptr)samplePathsWindow->setVisible(false);
    if(vstPathsWindow!=nullptr)vstPathsWindow->setVisible(false);
    if(vst3InstrumentBrowser!=nullptr)vst3InstrumentBrowser->setVisible(false);
    if(vst3EffectBrowser!=nullptr)vst3EffectBrowser->setVisible(false);
    if(drumRackWindow!=nullptr)drumRackWindow->setVisible(false);
    if(virtualKeyboard!=nullptr){virtualKeyboard->release();virtualKeyboard->setVisible(false);}
    generatorSurface.setVisible(false);generatorBody.setVisible(false);layoutMixerHandles();
    for(auto& button:generatorDockButtons)generatorDockStrip.removeChildComponent(button.get());
    generatorDockButtons.clear();generatorDockViewport.setVisible(false);
    if(auto* prefs=applicationProperties.getUserSettings())
    {for(const auto* name:{"playlistWindowOpen","mixerWindowOpen","pianoRollWindowOpen"})prefs->setValue(name,false);prefs->saveIfNeeded();}
    repaint();
}
