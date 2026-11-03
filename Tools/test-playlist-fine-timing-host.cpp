static void fineCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool fineNear(double a,double b){return std::abs(a-b)<1.e-7;}
static juce::MouseEvent fineMouse(juce::Component* c,juce::Point<float> p,int modifiers=0,juce::Point<float> down={},bool drag=false)
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|modifiers),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};}
static void fineImage(juce::Component& c,const char* name)
{auto im=c.createComponentSnapshot(c.getLocalBounds());juce::FileOutputStream f(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+name));juce::PNGImageFormat().writeImageToStream(im,f);}
static void fineRenderOnset(ProjectView& host)
{
    juce::WavAudioFormat format;te::Renderer::Parameters parameters(*host.edit);
    parameters.destFile=juce::File("OPTIONS_CHECK_FOLDER/fine-onset.wav");parameters.audioFormat=&format;parameters.sampleRateForAudio=48000;parameters.bitDepth=32;parameters.usePlugins=true;parameters.useMasterPlugins=true;
    parameters.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1.5)};
    te::Renderer::RenderTask task("Fine placement onset",parameters,nullptr,nullptr);int iterations=0;
    while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++iterations<100000){}
    fineCheck(iterations<100000 && task.errorMessage.isEmpty(),"fine placement offline render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(parameters.destFile),true));fineCheck(reader!=nullptr,"fine onset WAV unreadable");
    juce::AudioBuffer<float> samples(2,int(reader->lengthInSamples));reader->read(&samples,0,samples.getNumSamples(),0,true,true);
    const float before=samples.getMagnitude(0,0,19200),after=samples.getMagnitude(0,24000,7200);
    std::cout<<"Fine placement onset peaks before="<<before<<" after="<<after<<'\n';
    fineCheck(before<.000001f && after>.0001f,"rendered notes did not begin at the fractional clip origin");
}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,960);
    const auto saved=juce::File("OPTIONS_CHECK_FOLDER/fine-timing.orbit");
    if(argc>1)
    {
        fineCheck(host.loadProjectFile(saved),"fresh reopen failed");const auto id=host.project.getTrackId(0);auto* grid=host.playlistWindow->grid;
        fineCheck(!grid->isSnapToGrid() && grid->snapIntervalSteps()==4,"snap interval/enabled preference lost in fresh process");
        fineCheck(fineNear(host.project.getPlaylistClipStartBar(id,2.4),2.375),"fresh fractional origin lost");
        std::cout<<"PASS: fractional placement, automation and snap preferences reopen in a fresh process.\n";return 0;
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);host.addInstrumentTrack(true);host.showPlaylist();
    host.setPlaybackMode(ProjectView::PlaybackMode::song);auto* grid=host.playlistWindow->grid;
    grid->setHorizontalZoom(2.f);grid->setViewportArea({0,0,1400,700});grid->setDrawMode(true);grid->setSnapIntervalSteps(4);grid->setSnapToGrid(true);
    const auto id=host.project.getTrackId(0),other=host.project.getTrackId(1),pattern=host.project.getPatternId(id,0);
    host.selectGeneratorControlPattern(id,0);host.project.setPatternCreated(id,0,true);host.project.appendPatternNoteTicks(id,0,48,0,64,100);
    auto point=[&](double bar,int row=0){return juce::Point<float>{float(grid->labelWidth()+bar*grid->barWidth()),float(grid->headerHeight()+row*grid->rowHeight()+grid->rowHeight()/2)};};
    auto click=[&](double bar,int mods=0){auto e=fineMouse(grid,point(bar),mods);grid->mouseDown(e);grid->mouseUp(e);};
    auto drag=[&](double from,double to,int mods=0,int row=0,int toRow=0)
    {auto d=fineMouse(grid,point(from,row),mods);grid->mouseDown(d);auto e=fineMouse(grid,point(to,toRow),mods,d.position,true);grid->mouseDrag(e);grid->mouseUp(e);};
    auto placed=[&](double at){return fineNear(host.project.getPlaylistClipStartBar(id,at+1.e-7),at);};
    auto native=[&](double at)->te::MidiClip*
    {for(auto* c:host.getGeneratorTracks()[0]->getClips())if(auto* m=dynamic_cast<te::MidiClip*>(c);m && fineNear(m->getEditTimeRange().getStart().inSeconds(),at*host.playlistSecondsPerBar()))return m;return nullptr;};
    click(.32);fineCheck(placed(.25) && host.project.getPlaylistClips(id).size()==1,"Draw still quantizes to bars rather than chosen steps");
    fineCheck(grid->getComponentAt(point(.6).toInt())==grid,"bar adapter intercepts precise timeline hit testing");
    auto* midi=native(.25);fineCheck(midi && fineNear(midi->getEditTimeRange().getEnd().inSeconds(),2.5) && midi->getSequence().getNotes().size()==1,"native MIDI origin/duration/notes truncated");
    fineRenderOnset(host);
    auto* synth=host.generatorControlPlugin(id);auto* parameter=synth->getAutomatableParameters()[1];
    juce::Array<ProjectDocument::AutomationPoint> points;points.add({4,.2f});points.add({4.5,.37f});points.add({14,.8f});
    host.project.replaceSongAutomationPoints(id,synth->getName(),parameter->paramID,parameter->getParameterName(),points,synth->itemID.toString());host.refreshEnginePlaylist();
    const auto before=host.undoHistory.size();drag(.6,1.1);
    fineCheck(placed(.75) && native(.75) && host.undoHistory.size()==before+1,"fine drag failed or used multiple Undos");
    auto lane=host.project.getSongAutomationLane(id,0);bool movedPoint=false;for(auto p:lane.points)movedPoint|=fineNear(p.position,12.5) && fineNear(p.value,.37f);
    fineCheck(movedPoint,"fractional move did not carry linked Song events");
    host.undoProjectAction();fineCheck(placed(.25) && native(.25),"Undo lost fractional source/native timing");host.redoProjectAction();fineCheck(placed(.75),"Redo lost fractional destination");
    grid=host.playlistWindow->grid;grid->setDrawMode(false);grid->setSnapToGrid(false);
    drag(1.1,2.725,juce::ModifierKeys::commandModifier); // delta 1.625, free copy.
    fineCheck(placed(.75) && placed(2.375) && native(2.375),"Cmd free-copy changed origin or lost original");
    lane=host.project.getSongAutomationLane(id,0);bool copiedPoint=false;for(auto p:lane.points)copiedPoint|=fineNear(p.position,38.5) && fineNear(p.value,.37f);
    fineCheck(copiedPoint,"free copy detached Song automation");
    const auto selected=grid->selectedClips;const auto collisionUndo=host.undoHistory.size();
    fineCheck(!host.transformPlaylistSelection(selected,0,-1.5,false).has_value() && host.undoHistory.size()==collisionUndo && placed(2.375),"fine collision edit partially committed");
    fineCheck(fineNear(grid->snappedMoveDelta(2.375,.37,juce::ModifierKeys(juce::ModifierKeys::altModifier)),.625),"Option/Alt did not target nearest bar");
    fineCheck(fineNear(grid->snappedMoveDelta(2.375,.37,juce::ModifierKeys(juce::ModifierKeys::shiftModifier)),.375),"Shift did not temporarily enable chosen grid with Snap off");
    grid->setSnapToGrid(true);fineCheck(fineNear(grid->snappedMoveDelta(2.375,.37,juce::ModifierKeys(juce::ModifierKeys::shiftModifier)),host.project.normalisePlaylistBar(.37)),"Shift did not temporarily disable grid");
    // The actual group gesture preserves fractional spacing and moves all lanes once.
    using Selection=orbit::playlist::Selection;grid->selectedClips=Selection{{0,.75,pattern},{0,2.375,pattern}};grid->setSnapToGrid(false);
    drag(1.1,5.35);fineCheck(placed(5.) && placed(6.625) && host.project.getPlaylistClips(id).size()==2,"group free move changed relative spacing");host.undoProjectAction();fineCheck(placed(.75) && placed(2.375),"group Undo lost source positions");
    grid=host.playlistWindow->grid;grid->setDrawMode(false);grid->setSnapToGrid(false);grid->setSnapIntervalSteps(4);
    drag(2.725,3.095,juce::ModifierKeys::altModifier);fineCheck(placed(3.),"actual Option drag missed nearest bar");host.undoProjectAction();
    grid=host.playlistWindow->grid;grid->setDrawMode(false);grid->setSnapToGrid(false);drag(2.725,3.095,juce::ModifierKeys::shiftModifier);
    fineCheck(placed(2.75),"actual Shift drag did not toggle snapping");host.undoProjectAction();
    grid=host.playlistWindow->grid;grid->setDrawMode(true);grid->setSnapToGrid(true);grid->setSnapIntervalSteps(2);
    // Footer menus anchor at the clicked clip, including when scrolled, not
    // at the bottom of the entire (very wide/tall) timeline component.
    auto showMenu=grid->showPatternMenu;bool anchored=false;
    grid->showPatternMenu=[&](int row,double start,juce::Component* anchor){anchored=row==0 && fineNear(start,2.375) && anchor!=grid && anchor->getWidth()==1 && anchor->getParentComponent()==grid;};
    auto footer=fineMouse(grid,{point(2.5).x,float(grid->headerHeight()+grid->rowHeight()-4)});grid->mouseDown(footer);grid->mouseUp(footer);
    fineCheck(anchored,"fractional clip chooser anchors to entire timeline");grid->showPatternMenu=showMenu;
    // A right edge can extend by a grid step, without rounding back to a bar.
    const auto edge=point(3.375);auto down=fineMouse(grid,{edge.x-2,edge.y});grid->mouseDown(down);
    auto end=fineMouse(grid,{edge.x+float(.25*grid->barWidth())-2,edge.y},0,down.position,true);grid->mouseDrag(end);grid->mouseUp(end);
    fineCheck(fineNear(host.project.getPlaylistClipLengthBars(id,2.4),1.25) && native(2.375) && fineNear(native(2.375)->getEditTimeRange().getEnd().inSeconds(),7.25),"fine right-edge resize rounded duration");
    host.undoProjectAction();grid=host.playlistWindow->grid;
    // Snap-inverted Draw stamps at the native tick lattice, not at integer bars.
    grid->setDrawMode(true);grid->setSnapToGrid(true);click(8.37,juce::ModifierKeys::shiftModifier);
    fineCheck(host.project.getPlaylistClips(id).size()==3,"free Draw failed");
    const auto free=host.project.getPlaylistClips(id).back().startBar;fineCheck(std::abs(free-8.37)<grid->timelineTick()*2 && !fineNear(free,std::floor(free)),"free Draw still snapped to a step/bar");
    fineCheck(!host.project.setPlaylistClipExactBars(id,pattern,std::numeric_limits<double>::quiet_NaN(),1),"NaN clip accepted");
    // Adjacent short exact clips in the same bar remain distinct in rendering/hits/playback.
    host.project.setPlaylistClipExactBars(other,host.project.getPatternId(other,0),.125,.25);host.project.setPatternCreated(other,0,true);host.refreshEnginePlaylist();
    const auto items=host.selectablePlaylistItems();fineCheck(std::any_of(items.begin(),items.end(),[](const auto& i){return i.key.row==1 && fineNear(i.key.startBar,.125) && fineNear(i.lengthBars,.25);}),"precise selection metadata lost a short placement");
    // Existing bar-based recorders refuse a partial-bar gap instead of taking
    // ownership of the fine clip that starts later in that same bar.
    host.selectGeneratorControlPattern(other,0);host.loopButton.setToggleState(false,juce::dontSendNotification);
    const auto recordUndo=host.undoHistory.size();
    fineCheck(!host.preparePatternClipRecording(0) && !host.patternClipSnapshot && host.undoHistory.size()==recordUndo,"CLIPS recording ignored a fractional occupied range");
    fineCheck(!host.prepareMidiRecording(0,juce::Time::getMillisecondCounterHiRes()) && !host.midiTake.active,"MIDI recording ignored a fractional occupied range");
    // Native audio keeps an arbitrary source offset while its precise extent,
    // selected identity and track mixer events follow a fine mouse gesture.
    auto* audio=host.edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*host.edit),nullptr).get();host.project.addTrack(ProjectDocument::TrackKind::audio,"Fine audio");
    auto wave=audio->insertWaveClip("Missing source",juce::File{},{{tracktion::TimePosition::fromSeconds(2.351234),tracktion::TimePosition::fromSeconds(2.851234)},tracktion::TimeDuration::fromSeconds(.07)},false);
    fineCheck(wave!=nullptr,"audio fixture failed");wave->setIsReversed(true);
    auto* volume=audio->getVolumePlugin();auto* level=volume->getAutomatableParameters()[0];
    juce::Array<ProjectDocument::AutomationPoint> audioPoints;audioPoints.add({18.8,.2f});audioPoints.add({18.9,.47f});audioPoints.add({22.8,.8f});
    host.project.replaceSongAutomationPoints(host.project.getTrackId(2),volume->getName(),level->paramID,level->getParameterName(),audioPoints,volume->itemID.toString());
    host.playlistWindow->refresh();grid->setDrawMode(false);grid->setSnapIntervalSteps(1);grid->setSnapToGrid(true);drag(1.2,1.275,0,2,2);
    auto* movedWave=dynamic_cast<te::WaveAudioClip*>(host.getGeneratorTracks()[2]->getClips()[0]);
    fineCheck(movedWave && fineNear(movedWave->getPosition().getStart().inSeconds(),2.5) && fineNear(movedWave->getPosition().getOffset().inSeconds(),.07) && movedWave->getIsReversed(),"fine native audio move slipped source offset/state");
    grid->setSnapToGrid(false);drag(1.3,3.3,juce::ModifierKeys::commandModifier,2,2);
    fineCheck(host.getGeneratorTracks()[2]->getClips().size()==2,"fine native audio Cmd-copy failed");
    bool audioEvent=false;for(auto point:host.project.getSongAutomationLane(host.project.getTrackId(2),0).points)audioEvent|=fineNear(point.position,18.9+(1.25-2.351234/host.playlistSecondsPerBar())*16+32) && fineNear(point.value,.47f);
    fineCheck(audioEvent,"fine audio copy detached track mixer automation");
    // Canonical native ticks survive XML and repeated move/delete/reinsert cycles.
    auto model=host.project;const double origin=model.normalisePlaylistBar(10.1234567);const auto roundtrip=juce::File("OPTIONS_CHECK_FOLDER/fine-model.orbit");
    model.setPlaylistClip(id,pattern,origin,0,true);juce::String error;fineCheck(model.saveToFile(roundtrip,error),"model save failed");
    auto loaded=ProjectDocument::loadFromFile(roundtrip,error);fineCheck(loaded.has_value(),"model reload failed");model=*loaded;
    for(int pass=0;pass<50;++pass)
    {
        model.setPlaylistClip(id,{},origin,1,false);model.setPlaylistClip(id,pattern,origin+1./3840,0,true);
        model.setPlaylistClip(id,{},origin+1./3840,1,false);model.setPlaylistClip(id,pattern,origin,0,true);
    }
    fineCheck(model.getPlaylistClips(id).size()==4 && fineNear(model.getPlaylistClipStartBar(id,origin),origin),"canonical fine origins drifted or left duplicate source placements");
    grid->setDrawMode(false);host.playlistWindow->refresh();grid->setSnapIntervalSteps(4);grid->setSnapToGrid(false);fineImage(*host.playlistWindow,"fine-placement-playlist.png");
    grid->mouseDoubleClick(fineMouse(grid,point(2.7)));
    fineCheck(host.generatorControlWindows.at(id).window->isPatternView() && host.hostedPianoRollTrackId==id,"fractional body double-click opened the wrong Piano Roll");
    host.closePianoRollView();fineCheck(host.playlistWindow->isVisible(),"Piano Roll action closed Playlist");
    // Release fixture-owned clip references before replacing their owning Edit.
    wave=nullptr;
    host.saveToFile(saved);fineCheck(host.loadProjectFile(saved),"native save/reopen failed");fineCheck(placed(2.375),"fractional placement lost on native reopen");
    std::cout<<"PASS: actual grid step/free Draw, native MIDI timing, fine single/group move and Cmd-copy with linked Song automation, collisions, Shift/Option snapping, fractional resize, short clip extents, protected whole-bar recording, native audio offsets/reversal and mixer-event copy, canonical XML/tick stability, Undo/Redo and native save/reopen.\n";
    return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
