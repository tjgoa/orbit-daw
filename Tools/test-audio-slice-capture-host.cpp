static void sliceCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool sliceNear(double a,double b){return std::abs(a-b)<1.e-7;}
static juce::MouseEvent sliceMouse(juce::Component* c,juce::Point<float> p,juce::Point<float> down={},bool drag=false,int mods=0,int clicks=1)
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|mods),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),clicks,drag};}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1440,1000);h.createProjectDiscardingCurrent();h.addAudioTrack();h.showPlaylist();const auto id=h.selectedProjectTrackId();
    juce::File file("OPTIONS_CHECK_FOLDER/slice-source.wav");juce::WavAudioFormat wav;
    {auto out=std::make_unique<juce::FileOutputStream>(file);std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(out.release(),48000,2,24,{},0));juce::AudioBuffer<float> block(2,48000);for(int second=0;second<26;++second){for(int n=0;n<48000;++n){block.setSample(0,n,float(second+.5)*.01f);block.setSample(1,n,-float(second+.5)*.005f);}writer->writeFromAudioSampleBuffer(block,0,48000);}}
    auto added=h.engineTrackForProjectId(id)->insertWaveClip("Tail",file,{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(26)},{}},false);const auto clipID=added->itemID.toString();added=nullptr;
    auto* grid=h.playlistWindow->grid;grid->setDrawMode(false);grid->setHorizontalZoom(.5f);grid->setSnapIntervalSteps(16);grid->setSnapToGrid(true);
    auto point=[&](double bar,int row=0){return juce::Point<float>{float(grid->labelWidth()+bar*grid->barWidth()),float(grid->headerHeight()+row*grid->rowHeight()+grid->rowHeight()/2)};};
    auto drag=[&](double from,double to,int row=0,int destRow=0){auto down=sliceMouse(grid,point(from,row));grid->mouseDown(down);auto move=sliceMouse(grid,point(to,destRow),down.position,true);grid->mouseDrag(move);grid->mouseUp(move);};
    auto count=[&]{return h.selectablePlaylistItems().size();};
    drag(13.-.025,8.);auto* clip=h.audioClipForID(id,clipID);sliceCheck(clip && sliceNear(clip->getEditTimeRange().getEnd().inSeconds(),16),"right edge did not trim the tail in Select mode");
    drag(8.-.025,13.);sliceCheck(sliceNear(h.audioClipForID(id,clipID)->getEditTimeRange().getEnd().inSeconds(),26),"right edge could not restore source tail");
    drag(.025,1.);clip=h.audioClipForID(id,clipID);sliceCheck(sliceNear(clip->getEditTimeRange().getStart().inSeconds(),2) && sliceNear(clip->getPosition().getOffset().inSeconds(),2),"left trim slipped the source audio");
    h.undoProjectAction();grid=h.playlistWindow->grid;sliceCheck(sliceNear(h.audioClipForID(id,clipID)->getPosition().getOffset().inSeconds(),0),"edge trim Undo failed");
    sliceCheck(h.trimPlaylistAudioClip({0,0,clipID},0,8),"eight-bar fixture trim failed");grid->clearClipSelection();drag(6.,7.95);
    auto selected=h.playlistWindow->arrangementSelection();sliceCheck(selected.size()==1 && sliceNear(selected.begin()->rangeStart,6) && sliceNear(selected.begin()->rangeEnd,8) && count()==1,"Select did not mark only the last two bars without cutting");
    const auto before=h.undoHistory.size();drag(7.,11.);sliceCheck(count()==2 && h.undoHistory.size()==before+1,"range drag did not slice and move as one Undo");
    clip=h.audioClipForID(id,clipID);sliceCheck(clip && sliceNear(clip->getEditTimeRange().getEnd().inSeconds(),12),"slice drag changed unselected audio");
    auto moved=h.findAudioClipAtPosition(0,10);sliceCheck(moved && sliceNear(moved->getEditTimeRange().getStart().inSeconds(),20) && sliceNear(moved->getPosition().getOffset().inSeconds(),12) && sliceNear(moved->getEditTimeRange().getLength().inSeconds(),4),"moved audio slice lost source excerpt");
    h.undoProjectAction();sliceCheck(count()==1 && sliceNear(h.audioClipForID(id,clipID)->getEditTimeRange().getEnd().inSeconds(),16),"slice Undo did not rejoin the original");h.redoProjectAction();sliceCheck(count()==2,"slice Redo failed");h.undoProjectAction();
    auto middle=h.transformPlaylistSelection({{0,0,clipID,2,4}},0,8,false);sliceCheck(bool(middle) && count()==3,"middle-slice move did not retain both unselected sides");
    sliceCheck(h.findAudioClipAtPosition(0,0) && h.findAudioClipAtPosition(0,4) && !h.findAudioClipAtPosition(0,3),"middle slice did not leave the correct gap");h.undoProjectAction();
    const auto occupied=h.undoHistory.size();sliceCheck(!h.transformPlaylistSelection({{0,0,clipID,6,8}},0,-4,false) && count()==1 && h.undoHistory.size()==occupied,"slice overlapping unselected source partially committed");
    h.playlistWindow->setArrangementSelection({{0,0,clipID,6,8}});h.copyPlaylistClips(false);grid=h.playlistWindow->grid;grid->clipboardPasteRow=0;grid->clipboardPasteBar=10;h.pastePlaylistClips();
    moved=h.findAudioClipAtPosition(0,10);sliceCheck(moved && sliceNear(moved->getPosition().getOffset().inSeconds(),12) && sliceNear(moved->getEditTimeRange().getLength().inSeconds(),4) && sliceNear(grid->clipboardPasteBar,12),"partial Copy/Paste or insertion advance failed");
    h.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/slices.orbit"));sliceCheck(h.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/slices.orbit")),"sliced audio reopen failed");sliceCheck(h.findAudioClipAtPosition(0,10) && sliceNear(h.findAudioClipAtPosition(0,10)->getPosition().getOffset().inSeconds(),12),"saved source excerpt changed");
    h.createProjectDiscardingCurrent();std::vector<juce::String> ids;
    for(int bars:{3,4,8}){h.addInstrumentTrack(true);const auto trackID=h.selectedProjectTrackId();ids.push_back(trackID);h.project.setPatternNote(trackID,0,48,0,true);h.project.setPatternLengthSteps(trackID,0,bars*16);h.engineTrackForProjectId(trackID)->setSolo(true);}
    h.addInstrumentTrack(true);const auto blank=h.selectedProjectTrackId();ids.push_back(blank);h.project.setPatternLengthSteps(blank,3,32);h.project.setTrackSelectedPatternIndex(blank,3);h.engineTrackForProjectId(blank)->setSolo(true);
    h.setPlaybackMode(ProjectView::PlaybackMode::pattern);h.showPlaylist();h.playlistEditActive=true;grid=h.playlistWindow->grid;grid->clipboardPasteBar=0;
    sliceCheck(h.patternSnapshotRows(true).size()==4 && sliceNear(h.patternSnapshotLength(h.patternSnapshotRows(true)),24),"3/4/8 common cycle or blank solo eligibility incorrect");
    for(int n=0;n<8;++n)h.addPatternsToPlaylist(true);
    sliceCheck(sliceNear(grid->clipboardPasteBar,192) && h.project.getPlaylistClips(ids[0]).size()==64 && h.project.getPlaylistClips(ids[1]).size()==48 && h.project.getPlaylistClips(ids[2]).size()==24 && h.project.getPlaylistClips(blank).size()==96,"repeated F9 did not advance/fill complete cycles or blank patterns");
    h.undoProjectAction();sliceCheck(h.project.getPlaylistClips(ids[0]).size()==56,"F9 capture was not one Undo transaction");h.redoProjectAction();
    h.project.setPatternLengthSteps(ids[0],0,7*16);const auto cycle=h.patternSnapshotLength(h.patternSnapshotRows(true));sliceCheck(sliceNear(cycle,8),"unreconciled 7/8 did not fall back to longest pattern");
    grid=h.playlistWindow->grid;grid->clipboardPasteBar=200;h.addPatternsToPlaylist(true);const auto clips=h.project.getPlaylistClips(ids[0]);sliceCheck(sliceNear(clips.back().startBar,200) && sliceNear(clips.back().lengthBars,7) && h.project.getPlaylistPatternId(ids[0],207.5).isEmpty() && sliceNear(grid->clipboardPasteBar,208),"7-bar fallback did not leave one gap at the 8-bar boundary");
    h.project.setPatternLengthSteps(ids[2],0,512*16);sliceCheck(h.patternSnapshotLength(h.patternSnapshotRows(true))<=24,"snapshot overflowed 24-bar cap");
    h.openGeneratorControls(blank);auto* window=h.generatorControlWindows.at(blank).window.get();sliceCheck(!window->isMaximized(),"fixture unexpectedly maximized");
    for(auto* c:std::array<juce::Component*,3>{&window->up,&window->down,&window->steps}){auto event=sliceMouse(c,{3,3},{},false,0,2);window->mouseDoubleClick(event);sliceCheck(!window->isMaximized(),"toolbar descendant double-click maximized generator");}
    window->mouseDoubleClick(sliceMouse(window,{180,10},{},false,0,2));sliceCheck(window->isMaximized(),"actual title double-click no longer maximizes");
    std::cout<<"PASS: audio left/right tail trim/restore, Select range drag with remainder/source alignment, middle slices, atomic overlap rejection, one Undo/Redo, partial Copy/Paste and insertion advance, project reopen, repeated all/solo F9 with blank selectors, bounded common cycles and seven/eight gaps, safe toolbar double-click and title maximize.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
