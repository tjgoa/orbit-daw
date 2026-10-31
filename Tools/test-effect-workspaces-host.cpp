// Exercise the visible workspace contract through production window/actions.
static void fxWorkspaceCheck(ProjectView& host)
{
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto acid=host.selectedProjectTrackId();host.addDrumRackTrack();const auto drum=host.selectedProjectTrackId();
    host.showPlaylist();host.showPianoRoll();host.showSongEventEditor();host.showMixer(false);
    auto* mixer=host.mixerWindow.get();
    const auto mixerBounds=mixer->getBounds(),playlistBounds=host.playlistWindow->getBounds();
    for(int bus=0;bus<4;++bus)
    {const int row=host.getMixerTracks().indexOf(host.getFxReturnTrack(bus));fxCheck(row>=0 && host.getInsertPlugins(row).isEmpty(),"new project FX return has an automatic processor");}
    host.addInsert(0,8);const auto firstId=host.getInsertPlugins(0)[0]->itemID.toString();auto first=mixer->openEffectWindows().at(firstId);
    fxCheck(first->isVisible() && first->getParentComponent()==&host && !first->isOnDesktop(),"insert did not automatically open an embedded effect editor");
    fxCheck(first->getMinimiseButton()!=nullptr && first->getCloseButton()!=nullptr && first->getMaximiseButton()==nullptr,"effect title does not have exactly minus and X");
    first->setBounds(10,100,720,480);const auto firstBounds=first->getBounds();
    host.addInsert(1,9);const auto secondId=host.getInsertPlugins(1)[0]->itemID.toString();auto second=mixer->openEffectWindows().at(secondId);
    second->setBounds(350,220,650,400);
    fxCheck(first->isVisible() && second->isVisible() && first->getBounds()==firstBounds,"new effect replaced or repositioned another effect");
    fxCheck(host.playlistWindow->isVisible() && host.pianoRollWindow->isVisible() && host.songEventWindow->isVisible() && mixer->getBounds()==mixerBounds && host.playlistWindow->getBounds()==playlistBounds,"effect insertion closed or moved another panel");
    host.openGeneratorForRow(0);host.openGeneratorForRow(1);
    fxCheck(host.pianoRollWindow->isVisible() && first->isVisible() && second->isVisible(),"changing generator closed a Piano Roll/effect");
    second->minimiseButtonPressed();fxCheck(second->isMinimised() && !second->isVisible() && first->isVisible(),"minimize affected another window or left a floating strip");
    bool restored=false;
    for(auto& button:host.generatorDockButtons)if(button->getTooltip().contains("Restore effect window") && button->getButtonText().contains("Tube Distortion"))
        {button->onClick();restored=true;break;}
    fxCheck(restored && second->isVisible() && !second->isMinimised() && second->getBounds()==juce::Rectangle<int>(350,220,650,400),"minimized FX has no working dock restore or lost its position");
    host.togglePlaylistMaximized();
    fxCheck(host.playlistWindow->getBottom()<=host.generatorDockViewport.getY(),"maximized Playlist covers FX restore tabs");host.togglePlaylistMaximized();
    host.showMixer(true);host.showMixer(false);host.stopPlayback();
    fxCheck(first->isVisible() && second->isVisible(),"Mixer view/Stop hid an FX editor");
    second->minimiseButtonPressed();host.rememberUndo("Workspace fixture");host.setMasterGainDb(-6);host.undoProjectAction();
    mixer=host.mixerWindow.get();first=mixer->openEffectWindows().at(firstId);second=mixer->openEffectWindows().at(secondId);
    fxCheck(first->isVisible() && first->getBounds()==firstBounds && second->isMinimised() && !second->isVisible(),"Undo lost FX layout/minimized state");
    const auto before=host.undoHistory.size();second->restoreEditor(false);second->closeButtonPressed();
    fxCheck(host.getInsertPlugins(1).isEmpty() && host.getInsertPlugins(0).size()==1 && first->isVisible() && host.undoHistory.size()==before+1,"FX X failed to delete only its plugin with one Undo");
    host.undoProjectAction();fxCheck(host.getInsertPlugins(1).size()==1,"Undo did not restore deleted FX");
    const auto external=host.knownVst3Effects();const auto owner=host.mixerOwnerId(0),projectId=host.project.getProjectId();
    auto menu=host.makeMixerSendMenu(0,0,external);
    fxCheck(fxMenuContains(menu,"Built-in Effects") && fxMenuContains(menu,"Peak Compressor") && fxMenuContains(menu,"VST3 Effects"),"send menu hides effect choices");
    const auto sendsBefore=host.getGeneratorTracks()[0]->getAuxSendPlugin(0);host.handleMixerSendChoice(owner,projectId,0,external,0);
    fxCheck(host.getGeneratorTracks()[0]->getAuxSendPlugin(0)==sendsBefore,"cancelling send menu changed routing");
    host.handleMixerSendChoice(owner,projectId,0,external,108);const int row=host.getMixerTracks().indexOf(host.getFxReturnTrack(0));
    fxCheck(host.getInsertPlugins(row).size()==1 && dynamic_cast<orbit::mixing::PeakCompressorPlugin*>(host.getInsertPlugins(row)[0])!=nullptr,"right-click send assignment did not populate its return");
    host.setTrackSendGainDb(0,0,-12);fxCheck(host.getInsertPlugins(row).size()==1,"send adjustment silently added a processor");
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/workspaces-return.orbit");host.saveToFile(file);host.loadProjectFile(file);
    const int reloaded=host.getMixerTracks().indexOf(host.getFxReturnTrack(0));
    fxCheck(host.getInsertPlugins(reloaded).size()==1 && dynamic_cast<orbit::mixing::PeakCompressorPlugin*>(host.getInsertPlugins(reloaded)[0])!=nullptr,"project reopen lost the manually assigned return effect");
    std::cout<<"PASS: independent embedded FX auto-open, minus/dock restore, delete-X/Undo, generator/Piano Roll/Event coexistence, view/Stop/Undo layout, empty returns, direct send choices and manual return persistence.\n";
}
