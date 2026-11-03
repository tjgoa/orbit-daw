static void blockCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::Array<ProjectDocument::AutomationPoint> blockPoints(std::initializer_list<ProjectDocument::AutomationPoint> values)
{juce::Array<ProjectDocument::AutomationPoint> result;for(auto point:values)result.add(point);return result;}
static float blockValue(const ProjectDocument& doc,const juce::String& owner,const juce::String& plugin,const juce::String& param,double time)
{
    for(int i=0;i<doc.getSongAutomationLaneCount(owner);++i)
    {const auto lane=doc.getSongAutomationLane(owner,i);if(lane.pluginId==plugin && lane.parameterId==param)return orbit::automation::valueAt(lane,time);}
    throw std::runtime_error("expected lane missing");
}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,900);
    const auto saved=juce::File("OPTIONS_CHECK_FOLDER/song-event-blocks.orbit");
    if(argc>1)
    {
        blockCheck(host.loadProjectFile(saved),"fresh reopen failed");const auto id=host.project.getTrackId(0);
        auto* synth=host.generatorControlPlugin(id);const auto param=synth->getAutomatableParameters()[0]->paramID;
        blockCheck(std::abs(blockValue(host.project,id,synth->itemID.toString(),param,68.5)-.37)<.001,"fresh curve changed");
        const auto prefs=host.playlistWindow->playlistViewPreferences();blockCheck(bool(prefs["generatorEvents"]) && bool(prefs["mixerEvents"]),"event options not persisted");
        std::cout<<"PASS: fresh-process events, instance target and options.\n";return 0;
    }
    // Compare the resulting continuous curve to the intended source/destination
    // behavior at off-grid sample times, including overlap and preserved gaps.
    {
        ProjectDocument::AutomationLane source;source.points=blockPoints({{0,.1f},{19,.7f},{35.5,.3f},{60,.9f},{100,.2f}});
        auto changed=source;changed.points=orbit::playlist::editSongEventBlocks(source,.4f,{{20,60}},{{{40,80},20,source}});
        for(int i=0;i<400;++i)
        {
            const double t=i*.249+.017;const float expected=t>=40 && t<80?orbit::automation::valueAt(source,t-20):t>=20 && t<60?orbit::automation::valueAt(source,19.9999):orbit::automation::valueAt(source,t);
            blockCheck(std::isfinite(orbit::automation::valueAt(changed,t)) && std::abs(orbit::automation::valueAt(changed,t)-expected)<.00002f,"continuous block splice changed unrelated values or source curve");
        }
        blockCheck(std::abs(orbit::automation::valueAt(source,35.5)-.3f)<.0001f,"exact interior-point interpolation invalid");
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);host.addInstrumentTrack(true);host.addInstrumentTrack();host.showPlaylist();
    using Selection=orbit::playlist::Selection;using Lane=ProjectDocument::AutomationLane;
    const auto a=host.project.getTrackId(0),b=host.project.getTrackId(1),c=host.project.getTrackId(2);
    auto* synth=host.generatorControlPlugin(a);auto* targetSynth=host.generatorControlPlugin(b);
    auto* volume=host.getGeneratorTracks()[0]->getVolumePlugin();auto* targetVolume=host.getGeneratorTracks()[1]->getVolumePlugin();
    const auto synthId=synth->itemID.toString(),targetSynthId=targetSynth->itemID.toString(),volumeId=volume->itemID.toString(),targetVolumeId=targetVolume->itemID.toString();
    const auto parameter=synth->getAutomatableParameters()[0]->paramID,volumeParameter=volume->getAutomatableParameters()[0]->paramID;
    auto setLane=[&](const juce::String& owner,te::Plugin* plugin,const juce::String& param,std::initializer_list<ProjectDocument::AutomationPoint> points)
    {host.project.replaceSongAutomationPoints(owner,plugin->getName(),param,param,blockPoints(points),plugin->itemID.toString());};
    const auto ap=host.project.getPatternId(a,0);host.project.setPatternCreated(a,0,true);host.project.appendPatternNoteTicks(a,0,36,0,64,100);
    host.project.setPatternAutomationPoint(a,0,"MIDI","Pitch","Pitch",0,.25f);
    host.project.setPatternSwitch(a,0,false,0,64,true);
    for(int bar:{1,2,8})host.project.setPlaylistClip(a,ap,bar,0,true);
    setLane(a,synth,parameter,{{0,.1f},{16,.2f},{20.5,.37f},{32,.8f},{48,.3f},{160,.6f}});
    setLane(a,volume,volumeParameter,{{0,.9f},{16,.8f},{32,.4f},{48,.7f},{160,.6f}});
    setLane(b,targetSynth,parameter,{{0,.7f},{160,.7f}});setLane(b,targetVolume,volumeParameter,{{0,.6f},{160,.6f}});
    host.project.replaceSongAutomationPoints(a,"MIDI","Pitch","Pitch",blockPoints({{0,.5f},{16,.2f},{32,.8f},{48,.5f}}));
    host.showSongEventEditor("mixer:master");auto* master=host.masterVolumePlugin();const auto masterParameter=master->getAutomatableParameters()[0]->paramID;
    setLane("mixer:master",master,masterParameter,{{0,.1f},{16,.9f},{32,.2f}});
    const auto masterBefore=host.project.getSongAutomationLane("mixer:master",0).points;
    host.setPlaybackMode(ProjectView::PlaybackMode::song);host.refreshEnginePlaylist();
    auto options=[&](bool generators,bool mixer)
    {auto prefs=host.playlistWindow->playlistViewPreferences();prefs.setProperty("generatorEvents",generators,nullptr);prefs.setProperty("mixerEvents",mixer,nullptr);host.playlistWindow->setPlaylistViewPreferences(prefs);};
    auto value=[&](const juce::String& owner,const juce::String& plugin,const juce::String& param,double time){return blockValue(host.project,owner,plugin,param,time);};
    const auto original=host.project;
    options(false,false);blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value(),"clips-only copy failed");
    blockCheck(orbit::automation::samePoints(host.project.getSongAutomationLane(a,0).points,original.getSongAutomationLane(a,0).points),"disabled options changed events");host.undoProjectAction();
    options(true,false);const auto undo=host.undoHistory.size();
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value(),"generator event copy failed");
    blockCheck(host.undoHistory.size()==undo+1 && std::abs(value(a,synthId,parameter,68.5)-.37f)<.0001f,"fractional event or one Undo lost");
    blockCheck(orbit::automation::samePoints(host.project.getSongAutomationLane(a,1).points,original.getSongAutomationLane(a,1).points),"generator-only copy altered mixer lane");
    blockCheck(std::abs(value(a,synthId,parameter,10)-blockValue(original,a,synthId,parameter,10))<.0001f && std::abs(value(a,synthId,parameter,90)-blockValue(original,a,synthId,parameter,90))<.0001f,"copy bent outside linear curves");
    blockCheck(std::abs(value(a,synthId,parameter,64)-.2f)<.0001f,"copy omitted start boundary");
    host.undoProjectAction();blockCheck(orbit::automation::samePoints(host.project.getSongAutomationLane(a,0).points,original.getSongAutomationLane(a,0).points),"Undo failed to restore exact source curve");
    host.redoProjectAction();blockCheck(std::abs(value(a,synthId,parameter,68.5)-.37f)<.0001f,"Redo lost curve");host.undoProjectAction();
    options(false,true);blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value(),"mixer-only copy failed");
    blockCheck(orbit::automation::samePoints(host.project.getSongAutomationLane(a,0).points,original.getSongAutomationLane(a,0).points),"mixer-only copy changed generator");
    blockCheck(std::abs(value(a,volumeId,volumeParameter,64)-.8f)<.0001f,"mixer copy omitted events");host.undoProjectAction();
    options(true,true);const auto beforeMove=host.project;
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap},{0,2,ap}},0,3,false).has_value(),"adjacent multi-clip move failed");
    blockCheck(host.project.getPlaylistPatternId(a,1).isEmpty() && host.project.getPlaylistClipStartBar(a,4)==4 && host.project.getPlaylistClipStartBar(a,5)==5,"clip group did not follow events");
    blockCheck(std::abs(value(a,synthId,parameter,68.5)-.37f)<.0001f && std::abs(value(a,synthId,parameter,80)-.8f)<.0001f,"adjacent union lost points");
    blockCheck(std::abs(value(a,synthId,parameter,20)-blockValue(beforeMove,a,synthId,parameter,15.9999))<.0001f,"cut region did not hold preceding value");
    blockCheck(orbit::automation::samePoints(host.project.getSongAutomationLane("mixer:master",0).points,masterBefore),"shared Master events moved with a track");
    blockCheck(host.project.getPatternNotes(a,0).size()==1 && host.project.getPatternSwitches(a,0).size()==1 && host.project.getPatternAutomationLaneCount(a,0)==1,"source Pattern data altered");
    host.undoProjectAction();
    // Move a group into its own former range. All sources must be snapshotted.
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap},{0,2,ap}},0,1,false).has_value(),"self-overlap move failed");
    blockCheck(std::abs(value(a,synthId,parameter,36.5)-.37f)<.0001f && std::abs(value(a,synthId,parameter,48)-.8f)<.0001f,"self-overlap used mutated source");host.undoProjectAction();
    // Explicit source/destination plugin IDs, different synth cancels entire edit.
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},1,3,true).has_value(),"matching cross-track copy failed");
    blockCheck(std::abs(value(b,targetSynthId,parameter,68.5)-.37f)<.0001f && std::abs(value(b,targetVolumeId,volumeParameter,64)-.8f)<.0001f,"cross-track parameter mapping failed");
    blockCheck(std::abs(value(b,targetSynthId,parameter,60)-.7f)<.0001f && std::abs(value(b,targetSynthId,parameter,85)-.7f)<.0001f,"cross-track destination guards lost original curve");
    host.undoProjectAction();
    const auto rejectedUndo=host.undoHistory.size();
    blockCheck(!host.transformPlaylistSelection(Selection{{0,1,ap}},2,3,true).has_value() && host.undoHistory.size()==rejectedUndo && host.project.getPlaylistClips(c).empty(),"incompatible generator caused partial copy");
    // A copied Song lane must not take over existing destination Pattern automation.
    host.project.replaceSongAutomationPoints(b,host.generatorControlPlugin(b)->getName(),parameter,parameter,{},targetSynthId);
    host.project.setPatternAutomationPoint(b,0,host.generatorControlPlugin(b)->getName(),parameter,parameter,0,.6f);
    const auto patternUndo=host.undoHistory.size();
    blockCheck(!host.transformPlaylistSelection(Selection{{0,1,ap}},1,3,true).has_value() && host.undoHistory.size()==patternUndo && host.project.getPlaylistClips(b).empty(),"new Song lane disabled destination Pattern automation");
    host.project.replacePatternAutomationPoints(b,0,host.generatorControlPlugin(b)->getName(),parameter,parameter,{});
    // An empty destination gets its actual parameter baseline outside the copy.
    options(true,false);const auto destBaseline=float(host.generatorControlPlugin(b)->state.getChildWithName("ORBITAUTOMATIONBASES").getChildWithProperty("parameterId",parameter).getProperty("value"));
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},1,3,true).has_value(),"copy to empty Song target failed");
    blockCheck(std::abs(value(b,targetSynthId,parameter,60)-destBaseline)<.0001f,"new lane used an arbitrary baseline");host.undoProjectAction();
    // Matching duplicate inserts on the source are intentionally ambiguous across rows.
    auto addVolume=[&](int row){auto plugin=host.edit->getPluginCache().createNewPlugin(te::VolumeAndPanPlugin::xmlTypeName,{});host.getGeneratorTracks()[row]->pluginList.insertPlugin(plugin,-1,nullptr);};
    addVolume(0);addVolume(1);options(false,true);
    blockCheck(!host.transformPlaylistSelection(Selection{{0,1,ap}},1,3,true).has_value() && host.undoHistory.size()==rejectedUndo && host.project.getPlaylistClips(b).empty(),"duplicate plugin mapping guessed an instance");
    // Same-track exact-ID events still work even with duplicate names.
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value(),"exact same-track lane was rejected for duplicate names");host.undoProjectAction();
    // Legacy unique name-only targets are promoted to IDs, and Undo restores
    // the original lane representation rather than leaving duplicate edits.
    options(true,false);const auto legacyParam=host.generatorControlPlugin(a)->getAutomatableParameters()[1]->paramID;
    host.project.setSongAutomationPoint(a,host.generatorControlPlugin(a)->getName(),legacyParam,legacyParam,16,.42f);
    host.project.setSongAutomationPoint(a,host.generatorControlPlugin(a)->getName(),legacyParam,legacyParam,32,.84f);
    const int legacyLanes=host.project.getSongAutomationLaneCount(a);
    blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value(),"unique legacy target failed");
    blockCheck(std::abs(value(a,synthId,legacyParam,64)-.42f)<.0001f,"legacy target was not resolved to instance");
    host.undoProjectAction();blockCheck(host.project.getSongAutomationLaneCount(a)==legacyLanes,"legacy Undo left a promoted lane");
    options(false,true);
    // Audio selection uses the precise media range, not its rounded visual bars.
    auto audio=host.edit->insertNewAudioTrack(te::TrackInsertPoint::getEndOfTracks(*host.edit),nullptr);host.project.addTrack(ProjectDocument::TrackKind::audio,"Missing audio");
    const auto audioId=host.project.getTrackId(3);auto wave=audio->insertWaveClip("Missing",juce::File{},{{tracktion::TimePosition::fromSeconds(2.25),tracktion::TimePosition::fromSeconds(3.75)},tracktion::TimeDuration::fromSeconds(.5)},false);
    blockCheck(wave!=nullptr,"audio fixture failed");const auto waveId=wave->itemID.toString();wave=nullptr;audio=nullptr;
    auto* audioVolume=host.getGeneratorTracks()[3]->getVolumePlugin();const auto audioVolumeId=audioVolume->itemID.toString();const auto audioParam=audioVolume->getAutomatableParameters()[0]->paramID;
    setLane(audioId,audioVolume,audioParam,{{0,.1f},{18,.3f},{20.5,.7f},{30,.4f},{60,.9f}});
    const auto beforeAudio=host.project;
    blockCheck(host.transformPlaylistSelection(Selection{{3,2.25/host.playlistSecondsPerBar(),waveId}},0,2,true).has_value(),"audio+event copy failed");
    blockCheck(std::abs(value(audioId,audioVolumeId,audioParam,52.5)-.7f)<.0001f && std::abs(value(audioId,audioVolumeId,audioParam,49)-blockValue(beforeAudio,audioId,audioVolumeId,audioParam,49))<.0001f,"audio fractional range failed");host.undoProjectAction();
    // Until the wider pattern placement model supports meter-dependent bars,
    // reject mixed pattern/event moves; audio-only ranges already use seconds.
    host.project.setInitialTimeSignature(3,4);options(true,true);const auto meterUndo=host.undoHistory.size();
    blockCheck(!host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value() && host.undoHistory.size()==meterUndo,"unsupported pattern meter changed clips or events");
    blockCheck(host.transformPlaylistSelection(Selection{{3,2.25/host.playlistSecondsPerBar(),waveId}},0,2,true).has_value(),"3/4 audio+event copy failed");
    blockCheck(std::abs(value(audioId,audioVolumeId,audioParam,44.5)-.7f)<.0001f,"3/4 audio delta used 16 steps");
    bool audioAtExpected=false;for(auto* clip:host.getGeneratorTracks()[3]->getClips())audioAtExpected|=std::abs(clip->getPosition().getStart().inSeconds()-5.25)<1e-8;
    blockCheck(audioAtExpected,"3/4 audio events and native clip moved different durations");
    host.undoProjectAction();host.project.setInitialTimeSignature(4,4);
    options(true,true);blockCheck(host.transformPlaylistSelection(Selection{{0,1,ap}},0,3,true).has_value(),"final copy failed");
    host.setPlaybackMode(ProjectView::PlaybackMode::song);host.updateEngineAutomation();
    auto* native=host.songEventParameter({a,synthId,host.generatorControlPlugin(a)->getName(),parameter,parameter,{}});
    blockCheck(native!=nullptr,"native parameter lost");auto& curve=native->getCurve();bool matched=false;
    for(int i=0;i<curve.getNumPoints();++i)if(std::abs(curve.getPointTime(i).inSeconds()-8.5625)<1e-6 && std::abs(curve.getPointValue(i)-native->valueRange.convertFrom0to1(.37f))<.001)matched=true;
    blockCheck(matched,"native curve time or parameter range projection wrong");
    // No edit can consume an active capture or mutate clips on a bad target.
    host.automationTakeActive=true;const auto takeUndo=host.undoHistory.size();
    blockCheck(!host.transformPlaylistSelection(Selection{{0,1,ap}},0,5,true).has_value() && host.undoHistory.size()==takeUndo,"active take was edited");host.automationTakeActive=false;
    host.saveToFile(saved);blockCheck(host.loadProjectFile(saved),"reopen failed");
    blockCheck(std::abs(value(a,synthId,parameter,68.5)-.37f)<.0001f,"saved event lost");
    blockCheck(host.songEventWindow!=nullptr && host.songEventWindow->isVisible(),"event editor closed during edit");
    std::cout<<"PASS: disabled/independent policies, fractional curves and boundaries, group/cross-track/self-overlap moves, shared Master preservation, atomic mapping rejection, audio offsets, 3/4 audio and pattern-meter refusal, native projection, one Undo/Redo and reopen.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
