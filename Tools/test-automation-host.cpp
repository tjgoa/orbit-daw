static void eventCheck(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
static juce::MouseEvent eventMouse(juce::Component* c, float x, float y, bool drag=false, juce::Point<float> start={})
{
    juce::Point<float> p{x,y};
    return {juce::Desktop::getInstance().getMainMouseSource(), p, juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1,0,0,0,0,c,c,juce::Time::getCurrentTime(), drag?start:p,juce::Time::getCurrentTime(),1,drag};
}
static void eventTool(orbit::automation::SongEventPanel& p, const juce::String& text)
{
    for (auto* c : p.getChildren()) if (auto* b=dynamic_cast<juce::TextButton*>(c); b != nullptr && (b->getButtonText()==text || b->getTooltip()==text)) {b->onClick();return;}
    const juce::StringArray actions{"Copy","Paste","Clear","Smooth","Undo","Redo"};const int i=actions.indexOf(text);if(i>=0){p.action(i);return;}
    throw std::runtime_error("tool button absent");
}
#include "test-event-workspace-host.cpp"
int main(int argc, char**)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui; ProjectView host; host.stopTimer(); host.setSize(1220,760);
        const auto file=juce::File("OPTIONS_CHECK_FOLDER/song-events.orbit");
        if (argc>1)
        {
            eventCheck(host.loadProjectFile(file),"fresh Song project load failed");
            host.showSongEventEditor();
            auto& p=host.songEventWindow->panel;
            eventCheck(!p.targets.empty(),"fresh targets absent");
            bool found=false;
            for (const auto& t:p.targets) if (host.songEventPoints(t).size()==3) found=true;
            eventCheck(found,"instance Song curve lost on fresh reopen");
            eventCheck(p.isFollowing() && p.isGradientFillEnabled(),"Song view preferences did not persist");
            host.showSongEventEditor("mixer:master");eventCheck(host.songEventPoints(p.targets[0]).size()==3,"Master lane lost on fresh reopen");
            std::cout<<"PASS: Song instance lane restored in fresh process.\n";return 0;
        }
        host.createProjectDiscardingCurrent(); host.addInstrumentTrack();
        const auto id=host.selectedProjectTrackId();
        const bool rollVisible=host.pianoRollWindow->isVisible();
        host.handleMenuResult(2,304);
        eventCheck(host.songEventWindow!=nullptr && host.songEventWindow->isVisible(),"View Event Editor did not open Song editor");
        eventCheck(host.pianoRollWindow->isVisible()==rollVisible,"Event Editor changed Piano Roll visibility");
        auto& panel=host.songEventWindow->panel;
        eventCheck(!panel.targets.empty(),"no automatable targets");
        testEventWorkspace(host);
        auto target=panel.targets[0]; auto originalId=target.pluginId;
        eventTool(panel,"Line");
        const auto undoCount=host.undoHistory.size();
        const float top=panel.graph.plot().getY(), bottom=panel.graph.plot().getBottom();
        panel.graph.mouseDown(eventMouse(&panel.graph,4,bottom));
        panel.graph.mouseDrag(eventMouse(&panel.graph,132,top,true,{4,bottom}));
        eventCheck(host.songEventPoints(target).isEmpty(),"pending drag mutated project");
        panel.graph.mouseUp(eventMouse(&panel.graph,132,top));
        auto points=host.songEventPoints(target);
        eventCheck(points.size()==17 && points[0].value==0 && points.getLast().value==1,"Line missed full normalized range or song time");
        eventCheck(host.undoHistory.size()==undoCount+1,"drag did not create one Undo transaction");
        eventCheck(host.project.getPatternAutomationLaneCount(id,0)==0,"Song editing changed Pattern lanes");
        host.setPlaybackMode(ProjectView::PlaybackMode::song);
        auto* parameter=host.songEventParameter(target);
        eventCheck(parameter!=nullptr && parameter->getCurve().getNumPoints()==17,"Song curve not projected to engine");
        eventCheck(std::abs(parameter->getCurve().getPointTime(16).inSeconds()-2.0)<0.0001,"Song step/BPM projection wrong");
        eventCheck(std::abs(parameter->getCurve().getPointValue(16)-parameter->valueRange.convertFrom0to1(1))<0.001,"engine parameter range conversion wrong");
        eventCheck(panel.formatValue(1).isNotEmpty(),"units formatter missing");
        host.undoProjectAction(); eventCheck(host.songEventPoints(target).isEmpty(),"Undo left Song points");
        host.redoProjectAction(); eventCheck(host.songEventPoints(target).size()==17,"Redo lost Song points");
        eventCheck(host.songEventParameter(target)!=nullptr && target.pluginId==originalId,"Undo broke pinned instance ID");
        // Escape cancels a pending curve without adding history.
        eventTool(panel,"Draw"); const auto undoAfterRedo=host.undoHistory.size();
        panel.graph.mouseDown(eventMouse(&panel.graph,4,top));
        eventCheck(panel.graph.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey)),"Escape not handled");
        panel.graph.mouseUp(eventMouse(&panel.graph,4,top));
        eventCheck(host.undoHistory.size()==undoAfterRedo && host.songEventPoints(target)[0].value==0,"cancel committed gesture");
        // Every editing command remains on the pinned Song lane, including zero-text Mac shortcuts.
        eventCheck(panel.handleKey(juce::KeyPress('C',juce::ModifierKeys::commandModifier,0)),"Command-C not handled");
        panel.action(2); eventCheck(host.songEventPoints(target).isEmpty(),"Clear failed");
        eventCheck(panel.handleKey(juce::KeyPress('V',juce::ModifierKeys::commandModifier,0)),"Command-V not handled");
        eventCheck(host.songEventPoints(target).size()==17,"Paste failed");
        eventTool(panel,"Move Graph");
        panel.graph.mouseDown(eventMouse(&panel.graph,4,bottom)); panel.graph.mouseDrag(eventMouse(&panel.graph,132,bottom,true,{4,bottom}));panel.graph.mouseUp(eventMouse(&panel.graph,132,bottom));
        eventCheck(host.songEventPoints(target)[0].position==16 && host.songEventPoints(target).getLast().position==32,"Move Graph did not translate whole lane");
        eventTool(panel,"Erase");panel.graph.mouseDown(eventMouse(&panel.graph,132,bottom));panel.graph.mouseDrag(eventMouse(&panel.graph,140,bottom,true,{132,bottom}));panel.graph.mouseUp(eventMouse(&panel.graph,140,bottom));
        eventCheck(host.songEventPoints(target)[0].position==18,"Erase range failed");
        // Conflict detection refuses stale gestures and resolves targets again after session reconstruction.
        auto before=host.songEventPoints(target); auto changed=before; changed.getReference(0).value=0.33f;
        host.project.replaceSongAutomationPoints(id,target.pluginName,target.parameterId,target.parameterName,changed,target.pluginId);
        const auto conflictUndo=host.undoHistory.size();host.commitSongEvents(host.project.getProjectId(),target,before,{});
        eventCheck(host.undoHistory.size()==conflictUndo && host.songEventPoints(target)[0].value==0.33f,"stale gesture overwrote newer events");
        // Duplicate same-name plugin instances must remain independent.
        auto* track=host.songEventTrack(id);
        auto volume=host.edit->getPluginCache().createNewPlugin(te::VolumeAndPanPlugin::xmlTypeName,{});
        track->pluginList.insertPlugin(volume,-1,nullptr);host.refreshSongEventEditor();
        // The common volume/pan parameter provides a deterministic duplicate instance pair.
        std::vector<orbit::automation::SongTarget> volumes;
        for (const auto& t:panel.targets)if(t.pluginName==volume->getName() && t.parameterId==volume->getAutomatableParameters()[0]->paramID)volumes.push_back(t);
        volume=nullptr; // Do not keep a plugin alive beyond replacement of its owning Edit.
        eventCheck(volumes.size()>=2,"duplicate volume targets not enumerated");
        host.project.setSongAutomationPoint(id,volumes[0].pluginName,volumes[0].parameterId,volumes[0].parameterName,0,0.25f);
        for (const auto& t:volumes) host.songEventParameter(t)->getCurve().addPoint(tracktion::TimePosition::fromSeconds(0),0.5f,0);
        host.updateEngineAutomation();
        for (const auto& t:volumes) eventCheck(host.songEventParameter(t)->getCurve().getNumPoints()==0,"ambiguous legacy data left stale engine curves active");
        orbit::automation::Points a,b; a.add({0,0.2f});a.add({16,0.8f});a.add({32,0.4f});b.add({0,0.9f});b.add({16,0.1f});
        host.commitSongEvents(host.project.getProjectId(),volumes[0],host.songEventPoints(volumes[0]),a);
        host.commitSongEvents(host.project.getProjectId(),volumes[1],{},b);
        eventCheck(host.songEventPoints(volumes[0]).size()==3 && host.songEventPoints(volumes[1]).size()==2,"same-name instances shared a lane");
        eventCheck(host.songEventParameter(volumes[0])->getCurve().getNumPoints()==3 && host.songEventParameter(volumes[1])->getCurve().getNumPoints()==2,"same-name engine curves collided");
        host.setPlaybackMode(ProjectView::PlaybackMode::pattern);
        eventCheck(host.songEventParameter(volumes[0])->getCurve().getNumPoints()==0,"Song events leaked into Pattern mode");
        host.setPlaybackMode(ProjectView::PlaybackMode::song);
        const auto staleProjectId=host.project.getProjectId();
        host.saveToFile(file); eventCheck(host.loadProjectFile(file),"Song project reopen failed");
        eventCheck(host.songEventPoints(volumes[0]).size()==3 && host.songEventPoints(volumes[1]).size()==2,"Song IDs/data lost on save/reopen");
        // Models sanitize malformed points and remap IDs for imported/cloned tracks.
        auto doc=ProjectDocument::createNew();auto tid=doc.addTrack(ProjectDocument::TrackKind::instrument);
        orbit::automation::Points malformed;malformed.add({-2,-1});malformed.add({8,2});malformed.add({8,0.7f});malformed.add({70000,0.5f});malformed.add({9,std::numeric_limits<float>::quiet_NaN()});
        doc.replaceSongAutomationPoints(tid,"Effect","p","Param",malformed,"old");doc.remapSongAutomationPluginIds(tid,{{"old","new"}});
        auto lane=doc.getSongAutomationLane(tid,0);
        eventCheck(lane.pluginId=="new" && lane.points.size()==3 && lane.points[0].position==0 && lane.points[1].value==0.7f && lane.points[2].position>65535 && lane.points[2].position<65536,"model normalization/ID remap failed");
        host.showSongEventEditor();
        auto image=panel.createComponentSnapshot(panel.getLocalBounds());
        if (auto output=juce::File("OPTIONS_CHECK_FOLDER/song-event-editor.png").createOutputStream()) juce::PNGImageFormat().writeImageToStream(image,*output);
        // Song transport, visible loop and gradient fill are view-only, including empty lanes.
        host.setPlaybackMode(ProjectView::PlaybackMode::song);
        host.loopStartSlider.setValue(1,juce::dontSendNotification);host.loopLengthSlider.setValue(8,juce::dontSendNotification);
        host.loopButton.setToggleState(true,juce::dontSendNotification);host.updateLoopRange();
        host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(7.25));host.syncSongEventTransport();
        eventCheck(std::abs(panel.playheadPixel()-464)<0.01 && std::abs(panel.loopPixelRange().getEnd()-1024)<0.01,"Song cursor or bars 1-8 loop not synchronized");
        const auto viewUndo=host.undoHistory.size();const auto authored=host.songEventPoints(target);
        panel.setViewOptions(true,true);panel.onViewOptionsChanged();
        panel.setTransportState(300,true,true,0,128,true);
        eventCheck(panel.firstVisiblePixel()>0 && panel.playheadPixel()<panel.firstVisiblePixel()+panel.visibleGraphWidth(),"Follow did not reveal advancing playhead");
        panel.setTransportState(0,true,true,0,128,true);eventCheck(panel.firstVisiblePixel()==0,"loop return did not follow back to bar 1");
        const auto lineImage=[&]{panel.setViewOptions(false,false);return panel.graph.createComponentSnapshot(panel.graph.getLocalBounds().withWidth(400));}();
        panel.setViewOptions(false,true);auto fillImage=panel.graph.createComponentSnapshot(panel.graph.getLocalBounds().withWidth(400));
        int changedPixels=0;for(int y=0;y<lineImage.getHeight();++y)for(int x=0;x<lineImage.getWidth();++x)if(lineImage.getPixelAt(x,y)!=fillImage.getPixelAt(x,y))++changedPixels;eventCheck(changedPixels>100,"gradient option did not change curve fill");
        panel.setViewOptions(true,true);panel.onViewOptionsChanged();
        eventCheck(host.undoHistory.size()==viewUndo && orbit::automation::samePoints(host.songEventPoints(target),authored),"transport/fill changed authored events or Undo");
        auto filled=panel.createComponentSnapshot(panel.getLocalBounds());if(auto out=juce::File("OPTIONS_CHECK_FOLDER/song-event-filled.png").createOutputStream())juce::PNGImageFormat().writeImageToStream(filled,*out);
        // Master, return and output-bus curves persist independently of generator lanes.
        host.ensureFxReturnTrack(0);host.ensureOutputBusTrack(0);
        for(const auto& owner:juce::StringArray{"mixer:master","mixer:fx:0","mixer:bus:0"})
        {
            host.showSongEventEditor(owner);std::cout<<"CHECK module "<<owner<<" selected="<<panel.trackId<<" targets="<<panel.targets.size()<<"\n"<<std::flush;eventCheck(panel.trackId==owner && !panel.targets.empty(),"Mixer module not selectable");
            const auto mixerTarget=panel.targets[0];host.commitSongEvents(host.project.getProjectId(),mixerTarget,host.songEventPoints(mixerTarget),a);
            eventCheck(host.songEventParameter(mixerTarget)->getCurve().getNumPoints()==3,"Mixer Song curve not projected");
        }
        host.saveToFile(file);eventCheck(host.loadProjectFile(file),"Mixer automation reopen failed");host.showSongEventEditor("mixer:master");
        eventCheck(host.songEventPoints(panel.targets[0]).size()==3,"Master automation lost on reopen");
        // Pattern curves follow clips and repeat within extended placements.
        auto projection=ProjectDocument::createNew();auto projectionId=projection.addTrack(ProjectDocument::TrackKind::instrument);
        orbit::automation::Points ramp;ramp.add({0,0.1f});ramp.add({15,0.9f});
        projection.replacePatternAutomationPoints(projectionId,0,"Filter","cutoff","Cutoff",ramp);
        projection.setPlaylistClip(projectionId,projection.getPatternId(projectionId,0),2,2,true);
        const auto arranged=orbit::automation::arrangedPatternCurve(projection,projectionId,"Filter","cutoff",0.5f);
        eventCheck(arranged.at(32)==0.1f && arranged.at(48)==0.1f && arranged.at(64)==0.5f && arranged.at(47)>0.8f,"Pattern repetition/reset or gap baseline incorrect");
        // Automation takes preserve source precision and commit multiple lanes with one Undo.
        host.setPlaybackMode(ProjectView::PlaybackMode::song);host.showSongEventEditor(id);
        const auto liveTarget=panel.targets[0];host.showSongEventEditor("mixer:master");const auto masterTarget=panel.targets[0];
        const auto takeUndo=host.undoHistory.size();const auto oldLive=host.songEventPoints(liveTarget);
        eventCheck(host.beginAutomationRecording() && host.edit->getAutomationRecordManager().isWritingAutomation(),"automation recorder did not arm");
        host.rememberUndo("Adjust Acid Bass");eventCheck(host.undoHistory.size()==takeUndo,"control movement split automation take Undo");
        orbit::automation::Points precise;precise.add({0.125,0.2f});precise.add({0.375,0.7f});precise.add({7.25,0.4f});
        host.applyAutomationTake({{liveTarget,-1,precise},{masterTarget,-1,precise}});
        eventCheck(!host.automationTakeActive && !host.edit->getAutomationRecordManager().isWritingAutomation() && host.undoHistory.size()==takeUndo+1,"multi-lane take was not one stopped transaction");
        eventCheck(host.songEventPoints(liveTarget)[0].position==0.125 && host.songEventPoints(masterTarget)[1].position==0.375,"recorded sub-step timing was quantized");
        eventCheck(std::abs(host.songEventParameter(liveTarget)->getCurve().getPointTime(0).inSeconds()-0.015625)<0.000001,"fractional automation engine time conversion failed");
        host.undoProjectAction();eventCheck(orbit::automation::samePoints(host.songEventPoints(liveTarget),oldLive),"automation take Undo failed");host.redoProjectAction();
        const auto emptyTakeUndo=host.undoHistory.size();eventCheck(host.beginAutomationRecording(),"empty recorder did not arm");host.finishAutomationRecording();
        eventCheck(host.undoHistory.size()==emptyTakeUndo,"empty automation take created Undo");
        // A native stop may punch out before our UI timer; touched identities must survive it.
        const auto stoppedTakeUndo=host.undoHistory.size();host.beginAutomationRecording();
        auto* stoppedParam=host.songEventParameter(liveTarget);stoppedParam->setNormalisedParameter(0.9f,juce::sendNotification);
        eventCheck(!host.automationTouched.empty(),"manual parameter change was not observed during automation capture");
        stoppedParam->getCurve().clear();stoppedParam->getCurve().addPoint(tracktion::TimePosition::fromSeconds(0.046875),stoppedParam->valueRange.convertFrom0to1(0.9f),0);
        host.edit->getAutomationRecordManager().setWritingAutomation(false);host.finishAutomationRecording();
        eventCheck(host.undoHistory.size()==stoppedTakeUndo+1 && host.automationTouchListeners.empty()
            && std::abs(host.songEventPoints(liveTarget)[0].position-0.375)<0.00001,"native stop lost the take or retained parameter listeners");
        host.undoProjectAction();
        // Lazy mixer controls must attach capture listeners before their first change.
        host.beginAutomationRecording();host.setTrackSendGainDb(0,1,-6);
        eventCheck(!host.automationTouched.empty(),"first change to a newly created send escaped automation capture");
        host.cancelAutomationRecording();
        host.beginAutomationRecording();host.setTrackEqBand(0,0,900.0f,4.0f);
        eventCheck(!host.automationTouched.empty(),"first change to a newly created EQ escaped automation capture");
        host.cancelAutomationRecording();
        // Pattern-record destination is pinned and uses the same precise document clock.
        host.setPlaybackMode(ProjectView::PlaybackMode::pattern);host.beginAutomationRecording();
        host.applyAutomationTake({{liveTarget,0,precise}});
        bool precisePattern=false;for(int laneIndex=0;laneIndex<host.project.getPatternAutomationLaneCount(id,0);++laneIndex)
            if(host.project.getPatternAutomationLane(id,0,laneIndex).points[0].position==0.125)precisePattern=true;
        eventCheck(precisePattern,"Pattern automation take lost sub-step positions");
        host.setPlaybackMode(ProjectView::PlaybackMode::song);host.showSongEventEditor(id);
        const auto repeatedTarget=panel.targets[1];
        host.project.replacePatternAutomationPoints(id,0,repeatedTarget.pluginName,repeatedTarget.parameterId,repeatedTarget.parameterName,ramp);
        host.project.setPlaylistClip(id,host.project.getPatternId(id,0),2,2,true);host.refreshEnginePlaylist();
        auto* repeated=host.songEventParameter(repeatedTarget);
        eventCheck(repeated->getCurve().getNumPoints()>=5,"arranged Pattern curve did not reach engine");
        const auto normalizedAt=[&](double seconds){auto* p=host.songEventParameter(repeatedTarget);return p->valueRange.convertTo0to1(p->getCurve().getValueAt(tracktion::TimePosition::fromSeconds(seconds)));};
        eventCheck(std::abs(normalizedAt(4)-0.1f)<0.001 && std::abs(normalizedAt(6)-0.1f)<0.001,"Pattern automation failed to restart on extended placement");
        orbit::automation::Points overrideCurve;overrideCurve.add({0,0.6f});
        host.commitSongEvents(host.project.getProjectId(),repeatedTarget,{},overrideCurve);
        eventCheck(std::abs(normalizedAt(4)-0.6f)<0.001,"Song events did not override Pattern projection");
        host.commitSongEvents(host.project.getProjectId(),repeatedTarget,overrideCurve,{});
        eventCheck(std::abs(normalizedAt(4)-0.1f)<0.001,"Clear did not restore arranged Pattern automation");
        host.openParameterEvents(repeatedTarget,false);
        eventCheck(orbit::automation::sameTarget(panel.targets[size_t(panel.selectedTarget)],repeatedTarget),"control entry opened wrong Song parameter");
        host.openParameterEvents(repeatedTarget,true);
        eventCheck(host.pianoRollWindow->isVisible() && host.automationParameterId==repeatedTarget.parameterId,"control entry opened wrong Pattern parameter");
        const auto patternLfoUndo=host.undoHistory.size();host.showAutomationLfo(true);
        eventCheck(!host.pianoRollWindow->grid->automationPreview.empty(),"Pattern LFO preview absent");
        host.lfoWindow->panel.values[3].setValue(.25,juce::sendNotificationSync);host.lfoWindow->panel.onCancel();
        eventCheck(host.undoHistory.size()==patternLfoUndo && host.pianoRollWindow->grid->automationPreview.empty(),"Pattern LFO cancel mutated data or kept preview");
        host.saveToFile(file);eventCheck(host.loadProjectFile(file),"precise automation take reopen failed");
        eventCheck(host.songEventPoints(liveTarget)[0].position==0.125 && host.songEventPoints(masterTarget)[1].position==0.375,"fractional Song/Master points lost on native reopen");
        host.createProjectDiscardingCurrent();const auto staleUndo=host.undoHistory.size();
        host.commitSongEvents(staleProjectId,volumes[0],a,{});
        eventCheck(host.undoHistory.size()==staleUndo && panel.trackId=="mixer:master","old project callback edited replacement project");
        std::cout<<"PASS: distinct Song Event Editor routing, full-range line/draw/erase/move, one-gesture Undo/Redo, cancel/conflict protection, Mac clipboard, instance identity and engine curves, Song/Pattern isolation, native save/reopen, import ID remap, malformed points, transport/loop/fill/follow preferences, Master/bus/return targets, fractional multi-lane takes with one Undo and parameter-bound entry. Hardware disabled; native performance timing remains manual.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
