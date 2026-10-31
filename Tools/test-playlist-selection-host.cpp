static juce::MouseEvent selectionMouse(juce::Component* c,float x,float y,int modifiers=0,juce::Point<float> down={},bool drag=false)
{
    const juce::Point<float> p{x,y};return {juce::Desktop::getInstance().getMainMouseSource(),p,
        juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|modifiers),1,0,0,0,0,c,c,
        juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};
}

static void checkPlaylistSelection(ProjectView& host)
{
    using Selection=orbit::playlist::Selection;
   #if JUCE_MAC
    for(int code:{'Q','H'})playlistCheck(!host.keyPressed(juce::KeyPress(code,juce::ModifierKeys(juce::ModifierKeys::commandModifier|juce::ModifierKeys::ctrlModifier),0)),"host stole a macOS Control-Command chord");
   #endif
    std::cout<<"CHECK: arrangement selection transactions\n"<<std::flush;
    host.createProjectDiscardingCurrent();host.setSize(1440,900);
    for(int i=0;i<3;++i)host.addInstrumentTrack();host.showPlaylist();
    auto* grid=host.playlistWindow->grid;grid->setDrawMode(false);grid->setHorizontalZoom(1.0f);
    const auto a=host.project.getTrackId(0),b=host.project.getTrackId(1),c=host.project.getTrackId(2);
    host.project.appendPatternNoteTicks(a,0,60,0,64,80);host.project.appendPatternNoteTicks(a,0,60,0,32,110);
    host.project.setPatternSwitch(a,0,false,0,64,true);host.project.setPatternSwitch(a,0,true,64,64,true);
    host.project.setPatternAutomationPoint(a,0,"MIDI","cc74","Brightness",0,0.2f);
    host.project.setPatternAutomationPoint(a,0,"MIDI","cc74","Brightness",8,0.8f);
    host.project.setPatternName(a,0,"Acid phrase");host.project.setPatternCreated(a,0,true);
    host.project.setPatternCreated(b,0,true);host.project.setPatternLengthSteps(b,0,32);
    const auto ap=host.project.getPatternId(a,0),bp=host.project.getPatternId(b,0);
    for(int start:{0,2,8})host.project.setPlaylistClip(a,ap,start,0,true);
    host.project.setPlaylistClip(b,bp,1,0,true);host.refreshEnginePlaylist();host.playlistWindow->refresh();
    grid->setViewportArea({0,0,1600,600});
    auto point=[&](float bar,float row){return juce::Point<float>{grid->labelWidth()+bar*grid->barWidth(),grid->headerHeight()+row*grid->rowHeight()};};
    playlistCheck(grid->keyPressed(juce::KeyPress('A',juce::ModifierKeys(juce::ModifierKeys::commandModifier),0)) && grid->selectedClips.size()==4,"Mac Cmd-A without text did not select all Playlist clips");
    grid->clearClipSelection();
    const auto beforeSelection=host.undoHistory.size();const auto p=point(5,2.8f),q=point(0.1f,0.1f);
    auto press=selectionMouse(grid,p.x,p.y);grid->mouseDown(press);
    auto travel=selectionMouse(grid,q.x,q.y,0,p,true);grid->mouseDrag(travel);grid->mouseUp(travel);
    playlistCheck(grid->selectedClips==Selection{{0,0,ap},{0,2,ap},{1,1,bp}},"rectangle did not select whole intersecting clips across tracks");
    playlistCheck(host.undoHistory.size()==beforeSelection,"selection edited the project");
    auto dragSelected=[&](int rows,int bars,int modifiers)
    {
        auto& button=*grid->cellButtons.at(0);const auto start=juce::Point<float>(button.getWidth()/2.0f,button.getHeight()/2.0f);
        auto down=selectionMouse(&button,start.x,start.y,modifiers);button.mouseDown(down);
        auto move=selectionMouse(&button,start.x+bars*grid->barWidth(),start.y+rows*grid->rowHeight(),modifiers,start,true);
        button.mouseDrag(move);button.mouseUp(move);
    };
    const auto beforeMove=host.undoHistory.size();dragSelected(0,4,0);
    playlistCheck(host.project.getPlaylistPatternId(a,0).isEmpty() && host.project.getPlaylistClipStartBar(a,4)==4
        && host.project.getPlaylistClipStartBar(a,6)==6 && host.project.getPlaylistClipStartBar(b,5)==5,"group move changed spacing or left originals");
    playlistCheck(host.project.getPlaylistClipLengthBars(b,5)==2 && host.undoHistory.size()==beforeMove+1,"group move changed length or has multiple Undo entries");
    playlistCheck(grid->selectedClips==Selection{{0,4,ap},{0,6,ap},{1,5,bp}},"moved selection did not follow clips");
    host.undoProjectAction();playlistCheck(host.project.getPlaylistClipStartBar(a,0)==0 && host.project.getPlaylistClipStartBar(b,1)==1,"group Undo incomplete");
    host.redoProjectAction();grid->setDrawMode(false);grid->setViewportArea({0,0,1600,600});
    const Selection moved{{0,4,ap},{0,6,ap},{1,5,bp}};grid->selectedClips=moved;
    const auto collision=host.undoHistory.size();
    playlistCheck(!host.transformPlaylistSelection(moved,0,2,false).has_value() && host.undoHistory.size()==collision
        && host.project.getPlaylistClipStartBar(a,4)==4 && host.project.getPlaylistClipStartBar(a,6)==6,"blocked group edit was partial");
    for(const auto delta:{-100,std::numeric_limits<int>::max()})
        playlistCheck(!host.transformPlaylistSelection(moved,0,delta,false).has_value() && host.undoHistory.size()==collision,"out-of-grid group mutated project");
    playlistCheck(!host.transformPlaylistSelection(moved,std::numeric_limits<int>::max(),0,false).has_value(),"overflowing track delta accepted");
    const auto copy=host.transformPlaylistSelection(moved,0,10,true);
    playlistCheck(copy.has_value() && copy->size()==3 && host.project.getPlaylistClipStartBar(a,14)==14
        && host.project.getPlaylistClipStartBar(b,15)==15 && host.project.getPlaylistClipStartBar(a,4)==4,"group copy did not keep originals/spacing");
    host.undoProjectAction();playlistCheck(host.project.getPlaylistPatternId(a,14).isEmpty() && host.project.getPlaylistClipStartBar(a,4)==4,"copy Undo removed originals");
    const auto cross=host.transformPlaylistSelection(Selection{{0,4,ap},{0,6,ap}},2,20,false);
    playlistCheck(cross.has_value() && cross->size()==2,"cross-generator group transfer failed");
    const auto copiedId=host.project.getPlaylistPatternId(c,24);const auto slot=host.project.getPatternIndexForId(c,copiedId);
    playlistCheck(slot>=0 && host.project.getPlaylistPatternId(c,26)==copiedId && host.project.getPatternNotes(c,slot).size()==2,"repeated source did not share one copied destination pattern or lost coincident notes");
    playlistCheck(host.project.getPatternSwitches(c,slot).size()==2 && host.project.getPatternAutomationLaneCount(c,slot)==1
        && host.project.getPatternAutomationLane(c,slot,0).points.size()==2 && host.project.getPatternName(c,slot)=="Acid phrase","cross-generator copy lost switches/automation/name");
    host.undoProjectAction();
    for(int i=0;i<64;++i)host.project.setPatternCreated(c,i,true);
    const auto full=host.undoHistory.size();playlistCheck(!host.transformPlaylistSelection(Selection{{0,4,ap}},2,20,true).has_value()
        && host.undoHistory.size()==full && host.project.getPlaylistClipStartBar(a,4)==4,"full destination slots caused a partial edit");
    for(int i=0;i<64;++i)host.project.setPatternCreated(c,i,false);
    // Ctrl and Cmd pass through actual per-cell selection gestures, with vertical
    // movement reserved for track transfers while Select mode is active.
    grid->setDrawMode(false);host.playlistWindow->refresh();grid->setViewportArea({0,0,1600,600});
    auto& cell=*grid->cellButtons.at(4);grid->selectedClips=Selection{{0,4,ap}};
    const auto shiftUndo=host.undoHistory.size();auto shift=selectionMouse(&cell,10,20,juce::ModifierKeys::shiftModifier);cell.mouseDown(shift);cell.mouseUp(shift);
    playlistCheck(grid->selectedClips.empty() && host.undoHistory.size()==shiftUndo && host.project.getPlaylistClipStartBar(a,4)==4,"Shift selection triggered a clip edit");
    grid->selectedClips=Selection{{0,4,ap}};
    auto down=selectionMouse(&cell,10,20,juce::ModifierKeys::ctrlModifier);cell.mouseDown(down);
    auto drag=selectionMouse(&cell,10+10*grid->barWidth(),20,juce::ModifierKeys::ctrlModifier,down.position,true);cell.mouseDrag(drag);cell.mouseUp(drag);
    playlistCheck(host.project.getPlaylistClipStartBar(a,14)==14 && host.project.getPlaylistClipStartBar(a,4)==4,"Ctrl-drag did not copy a selection through cell handlers");
    playlistCheck(grid->keyPressed(juce::KeyPress(juce::KeyPress::deleteKey)) && host.project.getPlaylistPatternId(a,14).isEmpty()
        && host.project.isPatternCreated(a,0),"Delete removed source patterns or failed to delete selected placements");
    host.undoProjectAction();playlistCheck(host.project.getPlaylistClipStartBar(a,14)==14,"selection-delete Undo failed");
    // Cmd-drag must be a copy on macOS too, without popup deletion or pattern travel.
    grid->setDrawMode(false);host.playlistWindow->refresh();grid->setViewportArea({0,0,1600,600});grid->selectedClips=Selection{{0,6,ap}};
    auto& cmdCell=*grid->cellButtons.at(6);auto cmdDown=selectionMouse(&cmdCell,10,20,juce::ModifierKeys::commandModifier);cmdCell.mouseDown(cmdDown);
    auto cmdDrag=selectionMouse(&cmdCell,10+12*grid->barWidth(),20,juce::ModifierKeys::commandModifier,cmdDown.position,true);cmdCell.mouseDrag(cmdDrag);cmdCell.mouseUp(cmdDrag);
    playlistCheck(host.project.getPlaylistClipStartBar(a,18)==18 && host.project.getPlaylistClipStartBar(a,6)==6,"Cmd-drag did not copy through cell handlers");
    // Audio uses native engine clip IDs and keeps fractional start/offset/state.
    auto addAudio=[&](const char* name)
    {auto t=host.edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*host.edit),nullptr);host.project.addTrack(ProjectDocument::TrackKind::audio,name);return t.get();};
    auto audio=addAudio("Audio 1");auto audio2=addAudio("Audio 2");
    Selection audioKey;
    {auto wave=audio->insertWaveClip("Missing sample",juce::File{},
        {{tracktion::TimePosition::fromSeconds(2.25),tracktion::TimePosition::fromSeconds(3.75)},tracktion::TimeDuration::fromSeconds(0.5)},false);
    playlistCheck(wave!=nullptr,"audio fixture creation failed");wave->setIsReversed(true);
    audioKey.insert({3,1,wave->itemID.toString()});}
    const auto movedAudio=host.transformPlaylistSelection(audioKey,1,2,false);playlistCheck(movedAudio.has_value(),"audio group transfer failed");
    auto* movedWave=dynamic_cast<te::WaveAudioClip*>(host.getGeneratorTracks()[4]->getClips()[0]);
    playlistCheck(movedWave!=nullptr && std::abs(movedWave->getPosition().getStart().inSeconds()-6.25)<1e-8
        && std::abs(movedWave->getPosition().getOffset().inSeconds()-0.5)<1e-8 && movedWave->getIsReversed(),"audio transfer lost fractional position, offset or reversal");
    const auto beforeType=host.undoHistory.size();playlistCheck(!host.transformPlaylistSelection(*movedAudio,-2,1,false).has_value() && host.undoHistory.size()==beforeType,"audio-to-instrument transfer was not atomic");
    const auto audioCopy=host.transformPlaylistSelection(*movedAudio,0,3,true);playlistCheck(audioCopy.has_value() && host.getGeneratorTracks()[4]->getClips().size()==2,"audio copy failed");
    host.undoProjectAction();playlistCheck(host.getGeneratorTracks()[4]->getClips().size()==1,"audio copy Undo failed");
    host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/playlist-selection.orbit"));
    playlistCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/playlist-selection.orbit")),"selection edits failed to reopen");
    playlistCheck(host.project.getPlaylistClipStartBar(a,14)==14 && host.getGeneratorTracks()[4]->getClips().size()==1,"native reopen lost group edits");
    // Hover-N pins the clip identity while its rename dialog is open.
    grid->clearClipSelection();grid->setDrawMode(false);
    const auto renamePoint=point(4.5f,0.5f);grid->mouseMove(selectionMouse(grid,renamePoint.x,renamePoint.y));
    const auto renameUndo=host.undoHistory.size();
    playlistCheck(grid->keyPressed(juce::KeyPress('n',0,'n')) && checkedOptions!=nullptr,"hover-N did not open pattern rename");
    host.selectTrackByIndex(1);checkedOptions->texts.at("name")->setText("Renamed phrase");
    playlistCheck(checkedApply(*checkedOptions) && host.project.getPatternName(a,0)=="Renamed phrase"
        && host.project.getPatternName(b,0)!="Renamed phrase" && host.undoHistory.size()==renameUndo+1,"hover-N changed the wrong pattern or lacks one Undo");
    checkedApply={};checkedOptions.reset();host.undoProjectAction();
    playlistCheck(host.project.getPatternName(a,0)=="Acid phrase","pattern rename Undo failed");
    grid->mouseMove(selectionMouse(grid,float(grid->labelWidth()+3.5f*grid->barWidth()),float(grid->headerHeight()+4.5f*grid->rowHeight())));
    playlistCheck(grid->keyPressed(juce::KeyPress('n',0,'n')) && checkedOptions!=nullptr,"hover-N did not open audio rename");
    checkedOptions->texts.at("name")->setText("Relink later");const auto audioRenameUndo=host.undoHistory.size();
    playlistCheck(checkedApply(*checkedOptions) && host.getGeneratorTracks()[4]->getClips()[0]->getName()=="Relink later"
        && host.undoHistory.size()==audioRenameUndo+1,"audio rename failed");
    checkedApply={};checkedOptions.reset();host.undoProjectAction();
    playlistCheck(host.getGeneratorTracks()[4]->getClips()[0]->getName()=="Missing sample","audio rename Undo failed");
    grid->mouseMove(selectionMouse(grid,float(grid->labelWidth()+3.5f*grid->barWidth()),float(grid->headerHeight()+4.5f*grid->rowHeight())));
    playlistCheck(grid->keyPressed(juce::KeyPress('n',0,'n')),"stale audio rename fixture failed");
    const auto clipId=host.getGeneratorTracks()[4]->getClips()[0]->itemID.toString();
    playlistCheck(host.deletePlaylistSelection(Selection{{4,3,clipId}}),"stale rename delete failed");
    const auto deletedUndo=host.undoHistory.size();
    playlistCheck(!checkedApply(*checkedOptions) && host.undoHistory.size()==deletedUndo,"stale audio rename edited another clip");
    checkedApply={};checkedOptions.reset();host.undoProjectAction();
    auto prefs=host.playlistWindow->playlistViewPreferences();prefs.setProperty("follow",true,nullptr);prefs.setProperty("navigation",false,nullptr);prefs.setProperty("notes",false,nullptr);
    host.playlistWindow->setPlaylistViewPreferences(prefs);
    auto& viewport=host.playlistWindow->surface->viewport;viewport.setViewPosition(0,0);host.playlistWindow->setPlayheadPosition(200,120,true);
    playlistCheck(viewport.getViewPositionX()>0,"Follow Play Cursor did not scroll to playback position");
    playlistCheck(!viewport.isHorizontalScrollBarShown(),"Navigation Bar preference did not hide scrollbar");
    playlistCheck(grid->keyPressed(juce::KeyPress('s',juce::ModifierKeys::shiftModifier,'S')) && !grid->followsPlayCursor(),"Shift-S failed to toggle Follow");
    const auto fixed=viewport.getViewPositionX();host.playlistWindow->setPlayheadPosition(400,120,true);
    playlistCheck(viewport.getViewPositionX()==fixed,"disabled Follow still scrolls");
    host.project.appendPatternNoteTicks(a,0,72,64,64,1);host.project.appendPatternNoteTicks(a,0,76,128,64,127);host.invalidatePlaylistPatternPreview(a,0);
    const auto preview=host.getPlaylistPreviewNotes(0,4);playlistCheck(std::any_of(preview.begin(),preview.end(),[](const auto& n){return n.velocity==1;})
        && std::any_of(preview.begin(),preview.end(),[](const auto& n){return n.velocity==127;}),"Playlist preview lost authored velocities");
    viewport.setViewPosition(0,0);host.playlistWindow->setPlayheadPosition(0,120,false);grid->showPatternGrid=false;
    const juce::Rectangle<int> area(grid->labelWidth()+4*grid->barWidth(),grid->headerHeight(),grid->barWidth(),grid->rowHeight());
    prefs=grid->viewPreferences();prefs.setProperty("velocities",false,nullptr);grid->setViewPreferences(prefs);auto plain=grid->createComponentSnapshot(area);
    prefs.setProperty("velocities",true,nullptr);prefs.setProperty("follow",true,nullptr);grid->setViewPreferences(prefs);auto velocities=grid->createComponentSnapshot(area);
    playlistCheck(plain.getPixelAt(15,15)!=velocities.getPixelAt(15,15) || plain.getPixelAt(9,20)!=velocities.getPixelAt(9,20),"velocity preview preference does not affect rendering");
    for(int skin:{5,6})
    {
        host.setSkin(skin);grid->selectedClips=Selection{{0,4,ap},{1,5,bp}};auto image=grid->createComponentSnapshot({0,0,1300,300});
        juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/playlist-selection-skin")+juce::String(skin)+".png"));juce::PNGImageFormat png;playlistCheck(png.writeImageToStream(image,out),"selection render failed");
    }
    std::cout<<"PASS: rectangle/group selection, Ctrl-drag, atomic move/copy/delete and Undo/Redo, cross-generator pattern metadata, coincident notes, automation and Accent/Slide, slot/collision/bounds refusal, native audio state/offsets, missing media and native reopen.\n";
}
