static void holdCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static ProjectDocument::AutomationLane holdLane(std::initializer_list<ProjectDocument::AutomationPoint> data)
{ProjectDocument::AutomationLane lane;for(auto point:data)lane.points.add(point);return lane;}
static juce::MouseEvent holdMouse(juce::Component* c,juce::Point<float> p,bool drag=false,juce::Point<float> down={})
{return {juce::Desktop::getInstance().getMainMouseSource(),p,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,c,c,juce::Time::getCurrentTime(),drag?down:p,juce::Time::getCurrentTime(),1,drag};}
int main(int argc,char** argv)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;
    const bool migration=argc>1 && juce::String(argv[1])=="--migration";
    if(migration)
    {
        juce::PropertiesFile::Options options;options.applicationName="OrbitOptionsCheck";options.filenameSuffix="settings";options.osxLibrarySubFolder="Application Support";options.folderName="OPTIONS_CHECK_FOLDER/prefs";
        options.getDefaultFile().getParentDirectory().createDirectory();juce::PropertiesFile old(options);
        old.setValue("options.playlistView.generatorEvents",false);old.setValue("options.playlistView.mixerEvents",false);old.removeValue("options.playlistEventLinkVersion");old.saveIfNeeded();
    }
    ProjectView host;host.stopTimer();host.setSize(1440,900);
    const auto saved=juce::File("OPTIONS_CHECK_FOLDER/automatic-links-hold.orbit");
    if(migration)
    {
        const auto prefs=host.playlistWindow->playlistViewPreferences();holdCheck(bool(prefs["generatorEvents"]) && bool(prefs["mixerEvents"]) && host.optionInt("playlistEventLinkVersion")==1,"old off defaults did not migrate to automatic linking");
        std::cout<<"PASS: old saved off defaults migrated to automatic linking.\n";return 0;
    }
    if(argc>1)
    {
        holdCheck(host.loadProjectFile(saved),"fresh load failed");const auto id=host.project.getTrackId(0);auto* synth=host.generatorControlPlugin(id);
        for(int param:{1,2})
        {
            bool found=false;for(int i=0;i<host.project.getSongAutomationLaneCount(id);++i)
            {auto lane=host.project.getSongAutomationLane(id,i);if(lane.pluginId==synth->itemID.toString() && lane.parameterId==synth->getAutomatableParameters()[param]->paramID){found=true;holdCheck(std::abs(orbit::automation::valueAt(lane,700)-(param==1?.8f:.65f))<.001,"fresh hold/target lost");}}
            holdCheck(found,"fresh recorded lane absent");
        }
        holdCheck(bool(host.playlistWindow->playlistViewPreferences()["generatorEvents"]) && bool(host.playlistWindow->playlistViewPreferences()["mixerEvents"]),"fresh auto links disabled");
        std::cout<<"PASS: fresh-process automatic links, native target identity and held final values.\n";return 0;
    }
    {
        const auto old=holdLane({{0,.2f},{512,.3f},{600,.7f}});const auto samples=holdLane({{1,.2f},{240,.8f}});
        auto next=old;next.points=orbit::automation::recordedSongCurve(old,samples.points,256);
        holdCheck(std::abs(orbit::automation::valueAt(next,400)-.8f)<.0001 && std::abs(orbit::automation::valueAt(next,511.999)-.8f)<.0001,"recorded value did not hold until next original event");
        holdCheck(std::abs(orbit::automation::valueAt(next,512)-.3f)<.0001 && std::abs(orbit::automation::valueAt(next,600)-.7f)<.0001,"later original event changed");
        auto empty=holdLane({{0,.2f}});empty.points=orbit::automation::recordedSongCurve(empty,samples.points,256);
        holdCheck(std::abs(orbit::automation::valueAt(empty,1000)-.8f)<.0001 && empty.points.getLast().position<256,"terminal hold returned or created an outside clip anchor");
        auto loopOld=holdLane({{0,.1f},{64,.4f},{96,.6f}});const auto loopSamples=holdLane({{1,.2f},{60,.8f},{66,.6f},{90,.9f}});
        auto looped=loopOld;looped.points=orbit::automation::recordedSongCurve(loopOld,loopSamples.points,96,orbit::automation::RecordLoop{0,64});
        holdCheck(std::abs(orbit::automation::valueAt(looped,26)-.9f)<.0001 && std::abs(orbit::automation::valueAt(looped,64)-.4f)<.0001,"loop last-pass recording or outside events lost");
        // Terminal cut/copy/move must not create a return to the removed tail.
        auto moved=empty;moved.points=orbit::playlist::editSongEventBlocks(empty,.2f,{{0,256}},{{{384,640},384,empty}});
        holdCheck(std::abs(orbit::automation::valueAt(moved,300)-.2f)<.0001 && std::abs(orbit::automation::valueAt(moved,700)-.8f)<.0001,"moving a terminal curve left an old tail or reset at its destination end");
    }
    auto& dm=host.engine.getDeviceManager();auto& device=dm.getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters config;config.sampleRate=48000;config.inputChannels=0;config.outputChannels=2;device.initialise(config);device.prepareToPlay(48000,512);dm.dispatchPendingUpdates();dm.setCpuLimitBeforeMuting(1000);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();const auto pattern=host.project.getPatternId(id,0);host.project.setPatternCreated(id,0,true);host.project.appendPatternNoteTicks(id,0,36,0,8,100);
    for(int bar=0;bar<16;++bar)host.project.setPlaylistClip(id,pattern,bar,0,true);
    host.project.setInitialTempoBpm(240);host.edit->tempoSequence.getTempo(0)->setBpm(240);host.tempoSlider.setValue(240,juce::dontSendNotification);
    host.setPlaybackMode(ProjectView::PlaybackMode::song);host.refreshEnginePlaylist();host.showPlaylist();
    auto prefs=host.playlistWindow->playlistViewPreferences();holdCheck(bool(prefs["generatorEvents"]) && bool(prefs["mixerEvents"]),"default/migrated links are off");
    host.saveOption("recordContent",1);host.saveOption("recordCountIn",0);
    auto* initial=host.generatorControlPlugin(id);auto* futureParameter=initial->getAutomatableParameters()[3];
    juce::Array<ProjectDocument::AutomationPoint> future;future.add({0,.2f});future.add({900,.4f});future.add({1000,.9f});
    host.project.replaceSongAutomationPoints(id,initial->getName(),futureParameter->paramID,futureParameter->getParameterName(),future,initial->itemID.toString());host.updateEngineAutomation();
    const auto undo=host.undoHistory.size();host.recordButton.onClick();host.playButton.onClick();holdCheck(host.automationTakeActive,"AUTO recording did not begin");
    juce::AudioBuffer<float> buffer(2,512);juce::MidiBuffer midi;
    auto pump=[&](int blocks){for(int n=0;n<blocks;++n){buffer.clear();midi.clear();device.processBlock(buffer,midi);}};
    pump(10);auto* synth=host.generatorControlPlugin(id);auto* cutoff=synth->getAutomatableParameters()[1];auto* reso=synth->getAutomatableParameters()[2];
    cutoff->setNormalisedParameter(.2f,juce::sendNotificationSync);reso->setNormalisedParameter(.3f,juce::sendNotificationSync);futureParameter->setNormalisedParameter(.25f,juce::sendNotificationSync);
    pump(1470);cutoff->setNormalisedParameter(.8f,juce::sendNotificationSync);reso->setNormalisedParameter(.65f,juce::sendNotificationSync);futureParameter->setNormalisedParameter(.7f,juce::sendNotificationSync);pump(20);
    std::cout<<"16-bar take end="<<host.patternClipPosition()<<" samples="<<host.automationSongSamples.size()<<std::endl;
    host.recordButton.onClick();holdCheck(!host.automationTakeActive && host.edit->getTransport().isPlaying() && host.undoHistory.size()==undo+1,"Record-off did not finish one take and continue playback");
    pump(200);holdCheck(std::abs(cutoff->getCurrentNormalisedValue()-.8f)<.001 && std::abs(reso->getCurrentNormalisedValue()-.65f)<.001 && std::abs(futureParameter->getCurrentNormalisedValue()-.7f)<.001,"parameters returned after recording while playback continued");host.stopButton.onClick();
    std::map<juce::String,ProjectDocument::AutomationLane> before;
    for(int n=0;n<host.project.getSongAutomationLaneCount(id);++n){auto lane=host.project.getSongAutomationLane(id,n);before[lane.parameterId]=lane;}
    holdCheck(before.size()==3,"multi-parameter capture lost a lane");
    const auto preserved=before.at(futureParameter->paramID);holdCheck(std::abs(orbit::automation::valueAt(preserved,800)-.7f)<.0001 && std::abs(orbit::automation::valueAt(preserved,900)-.4f)<.0001 && std::abs(orbit::automation::valueAt(preserved,1000)-.9f)<.0001,"native take lost later authored automation");
    auto* window=host.playlistWindow.get();auto* grid=window->grid;grid->setHorizontalZoom(.5f);window->surface->moveButton.onClick();grid->setViewportArea({0,0,1450,400});
    const juce::Point<float> start{float(grid->labelWidth()+17*grid->barWidth()),float(grid->headerHeight()+grid->rowHeight()+10)},end{float(grid->labelWidth()+1),float(grid->headerHeight()+1)};
    grid->mouseDown(holdMouse(grid,start));grid->mouseDrag(holdMouse(grid,end,true,start));grid->mouseUp(holdMouse(grid,end,true,start));holdCheck(grid->selectedClips.size()==16,"16-bar selection missed clips");
    auto* cell=grid->cellButtons.at(0).get();const juce::Point<float> down{10,20},to{10+24.f*grid->barWidth(),20};const auto moveUndo=host.undoHistory.size();
    cell->mouseDown(holdMouse(cell,down));cell->mouseDrag(holdMouse(cell,to,true,down));cell->mouseUp(holdMouse(cell,to,true,down));
    holdCheck(host.undoHistory.size()==moveUndo+1 && host.project.getPlaylistClipStartBar(id,24)==24 && host.project.getPlaylistClipStartBar(id,39)==39 && host.project.getPlaylistPatternId(id,0).isEmpty(),"16-bar group did not move to bar 25");
    for(int n=0;n<host.project.getSongAutomationLaneCount(id);++n)
    {
        auto lane=host.project.getSongAutomationLane(id,n);const auto original=before.at(lane.parameterId);
        for(double t:{4.,120.,250.})holdCheck(std::abs(orbit::automation::valueAt(lane,t+384)-orbit::automation::valueAt(original,t))<.0001,"Song curve did not follow real 16-bar drag");
        const float held=lane.parameterId==cutoff->paramID?.8f:lane.parameterId==reso->paramID?.65f:.7f;holdCheck(std::abs(orbit::automation::valueAt(lane,700)-held)<.001,"relocated curve did not hold beyond its clips");
    }
    host.undoProjectAction();holdCheck(host.project.getPlaylistClipStartBar(id,0)==0 && host.project.getPlaylistPatternId(id,24).isEmpty(),"linked move Undo did not restore clips");host.redoProjectAction();
    host.saveToFile(saved);holdCheck(host.loadProjectFile(saved),"native reopen failed");
    std::cout<<"PASS: real 16-bar three-parameter AUTO take holds after Record-off, default automatic selector/cell drag to bar 25 carries curves, no terminal reset, one Undo/Redo and native reopen; future-authored values and loop passes preserved.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
