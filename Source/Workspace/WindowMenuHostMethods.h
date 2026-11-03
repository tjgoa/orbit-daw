// Included inside ProjectView. Menu actions retain IDs or safe component pointers.
std::map<int,std::function<void()>> windowMenuActions;
juce::PopupMenu makeWindowMenu()
{
    juce::PopupMenu menu;windowMenuActions.clear();menu.addItem(606,"Close/Minimize All");menu.addItem(607,"Cascade");menu.addSeparator();
    int next=6200;
    auto add=[&](const juce::String& name,std::function<void()> action){const int key=next++;menu.addItem(key,name);windowMenuActions.emplace(key,std::move(action));};
    add("Playlist",[this]{showPlaylist();});add("Mixing Desk",[this]{showMixer();});
    const auto projectId=project.getProjectId();
    for(int row=0;row<project.getTrackCount();++row)
    {
        const auto id=project.getTrackId(row);
        if(canOpenGeneratorControls(id))add(project.getTrackName(row),[this,id,projectId]{if(project.getProjectId()==projectId && canOpenGeneratorControls(id)){selectGeneratorControlTrack(id);openGeneratorControls(id);}});
    }
    if(songEventWindow!=nullptr)add(songEventWindow->getName(),[weak=juce::Component::SafePointer<orbit::automation::SongEventWindow>(songEventWindow.get())]{if(weak!=nullptr){weak->setMinimised(false);weak->setVisible(true);weak->toFront(true);}});
    if(mixerWindow!=nullptr)for(const auto& [id,window]:mixerWindow->openEffectWindows())
        add(window->getName(),[weak=juce::Component::SafePointer<PluginParameterWindow>(window.get())]{if(weak!=nullptr)weak->restoreEditor();});
    return menu;
}
void closeMinimizeAllWindows()
{
    for(auto& [id,entry]:generatorControlWindows)if(entry.window->isVisible())entry.window->setMinimised(true);
    if(mixerWindow!=nullptr)for(const auto& [id,window]:mixerWindow->openEffectWindows())if(window->isVisible() && !window->isMinimised())window->minimiseButtonPressed();
    if(songEventWindow!=nullptr && songEventWindow->isVisible())songEventWindow->setMinimised(true);
    if(pianoRollWindow!=nullptr && hostedPianoRollTrackId.isEmpty())pianoRollWindow->setVisible(false);
    generatorCollapsed=true;allGeneratorsMinimized=true;
    for(auto* panel:{static_cast<juce::DocumentWindow*>(equaliserWindow.get()),static_cast<juce::DocumentWindow*>(audioSettingsWindow.get())})
        if(panel!=nullptr && panel->isVisible())panel->setMinimised(true);
    if(playlistWindow!=nullptr && playlistWindow->isVisible())playlistWindow->setMinimized(true);
    if(mixerWindow!=nullptr && mixerWindow->isVisible()){if(!mixerWindow->isMinimized())mixerBeforeMinimizeBounds=mixerPanelBounds;mixerWindow->setMinimized(true);}
    refreshGeneratorDock();syncGeneratorNativeViews();
}
