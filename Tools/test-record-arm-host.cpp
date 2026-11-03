static void armCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::MouseEvent armMouse(juce::Component* c,juce::Point<float> p,bool drag=false,juce::Point<float> down={},int mods=0)
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|mods),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,900);
    auto& dm=host.engine.getDeviceManager();auto& device=dm.getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;params.sampleRate=48000;params.inputChannels=0;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);dm.dispatchPendingUpdates();dm.setCpuLimitBeforeMuting(1000);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto a=host.selectedProjectTrackId();host.addInstrumentTrack(true);const auto b=host.selectedProjectTrackId();
    host.project.setPatternCreated(a,0,true);host.project.appendPatternNoteTicks(a,0,36,0,64,100);const auto ap=host.project.getPatternId(a,0);
    for(int bar:{0,2})host.project.setPlaylistClip(a,ap,bar,0,true);host.refreshEnginePlaylist();host.showPlaylist();
    auto* window=host.playlistWindow.get();auto* surface=window->surface.get();auto* grid=window->grid;
    armCheck(surface->drawButton.isVisible() && surface->moveButton.isVisible() && surface->isVisible() && window->isVisible(),"tools not visible");
    armCheck(surface->drawButton.getX()<80 && surface->moveButton.getX()<80 && surface->moveButton.getY()<10,"tools are not top left");
    armCheck(surface->getComponentAt(surface->moveButton.getBounds().getCentre())==&surface->moveButton,"selector hitbox covered by timeline");
    const auto toolBounds=surface->moveButton.getBounds();surface->viewport.setViewPosition(500,100);
    armCheck(surface->moveButton.getBounds()==toolBounds && surface->getComponentAt(toolBounds.getCentre())==&surface->moveButton,"tools scrolled away or lost hit target");surface->viewport.setViewPosition(0,0);
    surface->moveButton.onClick();armCheck(!grid->pencilActive() && surface->moveButton.getToggleState() && !surface->drawButton.getToggleState(),"visible selector did not set actual tool");
    const juce::Point<float> start{float(grid->labelWidth()+4*grid->barWidth()),float(grid->headerHeight()+grid->rowHeight()+10)},end{float(grid->labelWidth()+2),float(grid->headerHeight()+2)};
    grid->mouseDown(armMouse(grid,start));grid->mouseDrag(armMouse(grid,end,true,start));grid->mouseUp(armMouse(grid,end,true,start));
    armCheck(grid->selectedClips.size()==2,"top selector did not select clips by rectangle");
    grid->setDrawMode(true);armCheck(surface->drawButton.getToggleState() && !surface->moveButton.getToggleState(),"programmatic tool changed without visible state");surface->moveButton.onClick();
    auto prefs=window->playlistViewPreferences();prefs.setProperty("generatorEvents",true,nullptr);window->setPlaylistViewPreferences(prefs);
    auto* synth=host.generatorControlPlugin(a);auto* cutoff=synth->getAutomatableParameters()[1];const auto pid=cutoff->paramID,pluginId=synth->itemID.toString();
    juce::Array<ProjectDocument::AutomationPoint> curve;curve.add({0,.2f});curve.add({8,.8f});host.project.replaceSongAutomationPoints(a,synth->getName(),pid,"Cutoff",curve,pluginId);
    // Drag through actual cell handlers after using the visible selector.
    grid->setViewportArea({0,0,1300,400});auto* cell=grid->cellButtons.at(0).get();const juce::Point<float> down{20,20},to{20+4.0f*grid->barWidth(),20};
    const auto moveUndo=host.undoHistory.size();cell->mouseDown(armMouse(cell,down));cell->mouseDrag(armMouse(cell,to,true,down));cell->mouseUp(armMouse(cell,to,true,down));
    armCheck(host.project.getPlaylistClipStartBar(a,4)==4 && host.project.getPlaylistClipStartBar(a,6)==6 && host.undoHistory.size()==moveUndo+1,"selected group did not drag from visible tool");
    bool movedEvent=false;for(auto p:host.project.getSongAutomationLane(a,0).points)movedEvent|=std::abs(p.position-64)<.001 && std::abs(p.value-.2f)<.001;armCheck(movedEvent,"automation did not follow actual selector gesture");host.undoProjectAction();
    host.setPlaybackMode(ProjectView::PlaybackMode::song);dm.dispatchPendingUpdates();
    // No Event Editor has ever existed in this session.
    armCheck(host.songEventWindow==nullptr,"fixture unexpectedly initialized Event Editor");
    host.applicationProperties.getUserSettings()->removeValue("options.recordContent");host.recordModeButton.setButtonText(host.recordContentLabel());
    armCheck(host.recordContentMode()==1 && host.recordModeButton.getButtonText()=="AUTO","fresh Song record default is not AUTO");
    const auto idleUndo=host.undoHistory.size();host.recordButton.onClick();
    armCheck(host.recordArmed && host.recordButton.getToggleState() && !host.automationTakeActive && !host.edit->getAutomationRecordManager().isWritingAutomation() && !host.edit->getTransport().isPlaying(),"Record started capture or playback instead of arming");
    auto* live=host.generatorControlPlugin(a)->getAutomatableParameters()[1];live->setNormalisedParameter(.35f,juce::sendNotificationSync);
    host.timerCallback();armCheck(host.recordArmed && host.undoHistory.size()==idleUndo,"stopped arm was consumed by a timer/knob");
    grid->setPlayPositionFromRuler(1);host.selectGeneratorControlPattern(b,1);
    armCheck(host.recordArmed && host.recordButton.getToggleState(),"stopped cursor/pattern choice disarmed recording");
    host.showPlaylist();host.openGeneratorControls(b);armCheck(host.recordArmed && host.songEventWindow==nullptr,"window browsing consumed arm or created Event Editor");
    host.recordButton.onClick();armCheck(!host.recordArmed && !host.recordButton.getToggleState() && host.undoHistory.size()==idleUndo,"second stopped Record did not cancel cleanly");
    host.recordButton.onClick();host.stopButton.onClick();armCheck(!host.recordArmed && !host.edit->getTransport().isPlaying() && host.undoHistory.size()==idleUndo,"Stop did not cancel arm");
    host.recordButton.onClick();host.playButton.onClick();
    armCheck(!host.recordArmed && host.automationTakeActive && host.edit->getAutomationRecordManager().isWritingAutomation() && host.edit->getTransport().isPlaying() && host.songEventWindow==nullptr,"armed Song Play required Event Editor");
    juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;
    auto pump=[&](int blocks){for(int i=0;i<blocks;++i){audio.clear();midi.clear();device.processBlock(audio,midi);}};
    pump(80);live=host.generatorControlPlugin(a)->getAutomatableParameters()[1];live->setNormalisedParameter(.2f,juce::sendNotificationSync);pump(80);live->setNormalisedParameter(.9f,juce::sendNotificationSync);pump(80);
    armCheck(host.patternClipPosition()>2 && !host.automationTouched.empty(),"hosted take clock or touch capture failed");
    host.stopButton.onClick();armCheck(!host.automationTakeActive && !host.recordArmed && host.undoHistory.size()==idleUndo+1 && host.songEventWindow==nullptr,"hidden-editor Song take did not commit once");
    bool recorded=false;for(int n=0;n<host.project.getSongAutomationLaneCount(a);++n){auto l=host.project.getSongAutomationLane(a,n);if(l.pluginId==pluginId && l.parameterId==pid)for(auto p:l.points)recorded|=p.position>1 && p.value>.8f;}
    armCheck(recorded,"real knob recording produced no Song points");
    host.showSongEventEditor(a);host.saveOption("recordContent",0);host.recordModeButton.setButtonText("MIDI");host.showSongEventEditor(a);armCheck(host.recordContentMode()==0,"opening editor silently switched record type");host.songEventWindow->setVisible(false);
    host.saveOption("recordContent",1);host.recordButton.onClick();host.playButton.onClick();armCheck(host.automationTakeActive && !host.songEventWindow->isVisible(),"closed-editor second take failed");pump(20);host.stopButton.onClick();
    // Pattern MIDI uses the same arm/Play lifecycle and still supports a take.
    host.setPlaybackMode(ProjectView::PlaybackMode::pattern);host.saveOption("recordContent",0);host.recordButton.onClick();armCheck(host.recordArmed && !host.midiTake.active && !host.edit->getTransport().isPlaying(),"Pattern MIDI Record did not just arm");host.playButton.onClick();armCheck(host.midiTake.active && host.edit->getTransport().isPlaying(),"Pattern MIDI armed Play failed");pump(10);host.stopButton.onClick();
    // Already-playing Record punches in immediately, without stopping playback.
    host.saveOption("recordContent",1);host.playButton.onClick();pump(10);host.recordButton.onClick();armCheck(host.automationTakeActive && host.edit->getTransport().isPlaying(),"playing Record did not punch in");host.recordButton.onClick();armCheck(!host.automationTakeActive && host.edit->getTransport().isPlaying(),"playing Record-off stopped playback");host.stopButton.onClick();
    host.showPlaylist();host.playlistWindow->toFront(false);surface->moveButton.onClick();
    {auto image=host.createComponentSnapshot(host.getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/record-arm-playlist.png"));juce::PNGImageFormat().writeImageToStream(image,out);}
    host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/record-arm.orbit"));armCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/record-arm.orbit")) && !host.recordArmed,"arm persisted into project reopen");
    std::cout<<"PASS: top-left visible selector, fixed hit targets, rectangle/cell group move with Song events and Undo; Record arm/cancel/Play; Song automation before any Event Editor exists and with it closed; actual hosted knob capture; Pattern MIDI and playing punch-in; explicit mode preservation and reopen.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
