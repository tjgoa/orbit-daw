// Production handlers and native graph; isolated prefs and hosted audio only.
static void clipCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::AudioBuffer<float> clipRender(ProjectView& host,const juce::String& name,double end=6)
{
    juce::WavAudioFormat format;te::Renderer::Parameters p(*host.edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&format;p.sampleRateForAudio=48000;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(end)};
    te::Renderer::RenderTask task("Pattern Clips check",p,nullptr,nullptr);int n=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++n<100000){}clipCheck(n<100000 && task.errorMessage.isEmpty(),"native performance render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(p.destFile),true));clipCheck(reader!=nullptr,"performance render unreadable");juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));reader->read(&audio,0,audio.getNumSamples(),0,true,true);return audio;
}
static juce::String clipSignature(ProjectView& host,double end)
{
    juce::StringArray result;
    for(int row=0;row<host.project.getTrackCount();++row)for(auto* c:host.getGeneratorTracks()[row]->getClips())if(auto* clip=dynamic_cast<te::MidiClip*>(c))
    {
        const auto range=clip->getEditTimeRange();if(range.getStart().inSeconds()>=end)continue;
        juce::String key=juce::String(row)+":"+juce::String(range.getStart().inSeconds(),6)+":"+juce::String(juce::jmin(end,range.getEnd().inSeconds()),6)+":"+juce::String(clip->getLoopLengthBeats().inBeats(),6);
        for(auto* n:clip->getSequence().getNotes())key+=":"+juce::String(n->getNoteNumber())+","+juce::String(n->getStartBeat().inBeats(),6)+","+juce::String(n->getLengthBeats().inBeats(),6)+","+juce::String(n->getVelocity());
        result.add(key);
    }
    result.sort(true);return result.joinIntoString(";");
}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;
    struct Cleanup{~Cleanup(){checkedApply={};checkedOptions.reset();}}cleanup;
    ProjectView host;host.stopTimer();host.setSize(1440,900);
    const juce::File file("OPTIONS_CHECK_FOLDER/pattern-clips.orbit");
    if(argc>1)
    {
        clipCheck(host.loadProjectFile(file),"fresh project reopen failed");const auto id=host.project.getTrackId(0);
        auto clips=host.project.getPlaylistClips(id);clipCheck(clips.size()==4 && clips[0].exactLength && host.project.getPatternLengthSteps(id,0)==64,"performed placements/source lengths lost on reopen");
        clipCheck(!host.patternClipSnapshot && host.patternClipRuntime.empty() && host.recordContentMode()==2,"transient take serialized or content preference lost");
        auto audio=clipRender(host,"fresh");clipCheck(audio.getMagnitude(0,audio.getNumSamples())>.001,"fresh performance silent");std::cout<<"PASS: fresh-process performed arrangement, original pattern lengths, CLIPS preference and native audio.\n";return 0;
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto a=host.selectedProjectTrackId();
    for(int slot=0;slot<4;++slot){host.project.setPatternCreated(a,slot,true);host.project.appendPatternNoteTicks(a,slot,36+slot*12,0,512,100);host.project.setPatternLengthSteps(a,slot,slot==0?64:16);}
    host.addInstrumentTrack(true);const auto b=host.selectedProjectTrackId();
    for(int slot:{0,1,8,9}){host.project.setPatternCreated(b,slot,true);host.project.appendPatternNoteTicks(b,slot,48+slot,0,256,90);}
    host.project.setTrackSelectedPatternIndex(b,8);host.addInstrumentTrack(true);const auto c=host.selectedProjectTrackId();host.project.setPatternCreated(c,0,true);host.project.appendPatternNoteTicks(c,0,84,0,64,80);
    host.setTrackRecordArm(0,true);host.setTrackRecordArm(1,true);host.setPlaybackMode(ProjectView::PlaybackMode::song);host.saveOption("recordContent",2);host.recordModeButton.setButtonText(host.recordContentLabel());
    host.project.setPlaylistClipExactBars(a,host.project.getPatternId(a,0),8,1);host.project.setPlaylistClipExactBars(b,host.project.getPatternId(b,0),6,1);host.refreshEnginePlaylist();
    host.openGeneratorControls(a,false);host.openGeneratorControls(b,false);host.showPlaylist();host.showSongEventEditor();host.showMixer();clipCheck(host.recordContentMode()==2 && host.recordModeButton.getButtonText()=="CLIPS","opening Event Editor changed the explicit CLIPS recording mode");
    const auto bounds=host.generatorControlWindows.at(a).window->getBounds();const auto bBounds=host.generatorControlWindows.at(b).window->getBounds();const auto undo=host.undoHistory.size();auto originalNotes=host.capturePatternData(a,0).notes;
    clipCheck(host.preparePatternClipRecording(0),"armed performance setup failed");clipCheck(host.patternClipTake.targets.size()==2 && host.patternClipTake.endBar==6 && host.recordButton.getToggleState(),"arming/bounded region/record lamp wrong");
    clipCheck(host.project.getPlaylistPatternId(a,0).isEmpty() && host.undoHistory.size()==undo,"pending take edited authored arrangement or Undo");
    clipCheck(host.patternClipRuntime.size()==2 && host.patternClipRuntime[0].clip->getEditTimeRange().getEnd().inSeconds()==12,"engine was not prepared beyond the GUI timer");
    const auto originalRuntime=host.patternClipRuntime[0].clip->itemID;
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(.25));host.selectGeneratorControlPattern(a,1);host.selectGeneratorControlPattern(a,2);host.selectGeneratorControlPattern(a,0);
    clipCheck(host.patternClipTake.launches[a.toStdString()].size()==1,"same-bar queued cancellation failed in generator controls");
    host.selectGeneratorControlTrack(a);host.selectPatternSlot(3);host.rememberSongPatternsAt(0);clipCheck(host.project.getTrackSelectedPatternIndex(a)==3 && host.patternClipTake.active,"main selector lost queued choice or stopped take");
    host.selectGeneratorControlTrack(b);host.selectGeneratorControlPattern(b,9);host.selectGeneratorControlTrack(c);host.selectGeneratorControlPattern(c,1);
    clipCheck(host.patternClipTake.active && host.patternClipTake.targets.size()==2 && !host.patternClipTake.contains(c.toStdString()),"changing selected/unarmed generator stopped or rerouted take");
    host.advancePatternClipRecording(2.2,true);clipCheck(host.visiblePlaylistClipStart(a,1)==1 && host.visiblePlaylistClipLength(a,1)==1 && host.getPlaylistClipLabel(0,1)==host.project.getPatternName(a,3),"growing preview missing or wrong pattern");
    clipCheck(host.project.getPlaylistPatternId(a,1).isEmpty() && host.patternClipRuntime[0].clip->itemID==originalRuntime,"timer mutated document or rebuilt an unchanged MIDI clip");
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(2.3));host.selectGeneratorControlPattern(a,2);host.advancePatternClipRecording(5.1,true);
    clipCheck(host.generatorControlWindows.at(a).window->isVisible() && host.generatorControlWindows.at(b).window->isVisible() && host.generatorControlWindows.at(a).window->getBounds()==bounds && host.generatorControlWindows.at(b).window->getBounds()==bBounds && host.songEventWindow->isVisible(),"recording changed workspace visibility or geometry");
    // Inspect actual engine ranges and notes before rendering. The long source
    // pattern must be cut at a launch without changing its source length.
    bool found=false;for(const auto& r:host.patternClipRuntime)if(r.trackId==a && r.startBar==0){found=true;clipCheck(r.lengthBars==1 && r.clip->getEditTimeRange().getEnd().inSeconds()==2 && r.clip->getSequence().getNotes()[0]->getNoteNumber()==36,"runtime pattern selection/range wrong");}clipCheck(found,"initial runtime clip vanished");
    const auto midiBefore=clipSignature(host,6);auto pending=clipRender(host,"pending");clipCheck(pending.getMagnitude(0,48000)>.001 && pending.getMagnitude(2*48000,48000)>.001 && pending.getMagnitude(4*48000,48000)>.001,"prepared performance has silent bars");
    clipCheck(host.finishPatternClipRecording(5.1),"atomic commit failed");clipCheck(host.undoHistory.size()==undo+1 && !host.patternClipSnapshot && !host.recordButton.getToggleState() && !host.getTrackRecordArm(0) && !host.getTrackRecordArm(1),"take grouping/disarm/record lamp failed");
    auto clips=host.project.getPlaylistClips(a);clipCheck(clips.size()==4 && host.project.getPlaylistClipLengthBars(a,0)==1 && host.project.getPlaylistClipStartBar(a,1)==1 && host.project.getPlaylistClipStartBar(a,2)==2 && host.project.getPlaylistPatternId(a,8)==host.project.getPatternId(a,0),"performed ranges incorrect or existing clip overwritten");
    clipCheck(host.project.getPatternLengthSteps(a,0)==64 && host.capturePatternData(a,0).notes==originalNotes && host.project.getPlaylistClips(c).empty(),"recording modified source notes/length or unarmed arrangement");
    host.undoProjectAction();clipCheck(host.project.getPlaylistPatternId(a,0).isEmpty() && host.project.getPlaylistPatternId(a,8).isNotEmpty(),"take Undo lost pre-existing data");host.redoProjectAction();clipCheck(host.project.getPlaylistClipLengthBars(a,0)==1,"take Redo lost exact shorter placement");
    host.saveToFile(file);clipCheck(host.loadProjectFile(file),"same-process reopen failed");auto committed=clipRender(host,"committed");clipCheck(clipSignature(host,6)==midiBefore,"committed MIDI ranges/loop lengths/notes differ from prepared performance");clipCheck(committed.getMagnitude(0,48000)>.001 && committed.getMagnitude(2*48000,48000)>.001 && committed.getMagnitude(4*48000,48000)>.001,"committed performance has silent bars");
    // All following fixtures are separate projects; keep the persisted example.
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);auto id=host.selectedProjectTrackId();host.project.setPatternCreated(id,0,true);host.project.appendPatternNoteTicks(id,0,36,0,512,100);host.setPlaybackMode(ProjectView::PlaybackMode::song);
    const auto canceled=host.undoHistory.size();clipCheck(host.preparePatternClipRecording(4.2),"fractional-cursor setup failed");clipCheck(host.patternClipTake.startBar==3,"fractional cursor was not quantized forward");host.edit->setCountInMode(te::Edit::CountIn::oneBar);host.recordingUsesEnginePreroll=true;host.recordingPunchSeconds=4.2;host.recordingStartMs=juce::Time::getMillisecondCounterHiRes()+2000;host.updateRecordingCountInStatus(juce::Time::getMillisecondCounterHiRes());clipCheck(host.status.getText().contains("Count-in"),"clip count-in status absent");
    clipCheck(host.finishPatternClipRecording(5.5) && host.undoHistory.size()==canceled && host.project.getPlaylistClips(id).empty() && host.patternClipRuntime.empty() && !host.recordingUsesEnginePreroll,"cancel during wait/count-in left authored clips or Undo");
    clipCheck(host.preparePatternClipRecording(0),"conflict fixture setup failed");host.project.setPlaylistClipExactBars(id,host.project.getPatternId(id,0),1,1);const auto conflictUndo=host.undoHistory.size();clipCheck(!host.finishPatternClipRecording(3) && host.patternClipSnapshot && !host.patternClipTake.active && host.undoHistory.size()==conflictUndo && host.project.getPlaylistPatternId(id,0).isEmpty(),"conflict partially committed/discarded take");host.project.setPlaylistClip(id,{},1,1,false);clipCheck(host.finishPatternClipRecording(100) && host.project.getPlaylistClipLengthBars(id,0)==2,"retry changed stopped take extent");
    host.loopButton.setToggleState(true,juce::dontSendNotification);clipCheck(!host.preparePatternClipRecording(6),"loop recording silently accepted");host.loopButton.setToggleState(false,juce::dontSendNotification);host.project.setInitialTimeSignature(3,4);clipCheck(!host.preparePatternClipRecording(6),"unsupported meter silently accepted");host.project.setInitialTimeSignature(4,4);
    clipCheck(!host.preparePatternClipRecording(0) && !host.preparePatternClipRecording(1024),"occupied/end positions accepted");
    // Production Play and Record paths with the engine's hosted device. No
    // physical audio/MIDI device is opened.
    auto& device=host.engine.getDeviceManager().getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;params.sampleRate=48000;params.inputChannels=0;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);host.engine.getDeviceManager().dispatchPendingUpdates();
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);id=host.selectedProjectTrackId();host.project.setPatternCreated(id,0,true);host.project.appendPatternNoteTicks(id,0,36,0,512,100);host.setPlaybackMode(ProjectView::PlaybackMode::song);host.saveOption("recordContent",2);host.saveOption("recordCountIn",0);host.setTrackRecordArm(0,true);
    host.playButton.onClick();clipCheck(host.patternClipTake.active && host.edit->getTransport().isPlaying(),"R-arm plus Play failed to start pattern performance");
    // Device updates were dispatched before Play. Pump audio immediately;
    // dispatching device rebuilds here would stop the just-started test graph.
    host.engine.getDeviceManager().setCpuLimitBeforeMuting(1000);
    std::cout<<"Live prepared MIDI: "<<clipSignature(host,6)<<" at "<<host.patternClipPosition()<<std::endl;
    juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;double peak=0;
    for(int n=0;n<600;++n){audio.clear();midi.clear();device.processBlock(audio,midi);peak=juce::jmax(peak,double(audio.getMagnitude(0,512)));}
    std::cout<<"Live peak "<<peak<<" at "<<host.patternClipPosition()<<" stream "<<host.engine.getDeviceManager().getCurrentStreamTime()<<std::endl;
    clipCheck(peak>.001,"live hosted performance silent");host.stopButton.onClick();clipCheck(!host.patternClipSnapshot && !host.project.getPlaylistClips(id).empty() && !host.getTrackRecordArm(0),"transport Stop did not commit/disarm");
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(12));host.recordButton.onClick();clipCheck(host.recordArmed && !host.patternClipTake.active && !host.edit->getTransport().isPlaying(),"stopped Record did not only arm CLIPS");host.playButton.onClick();clipCheck(host.patternClipTake.active && host.edit->getTransport().isPlaying(),"armed Play fallback to selected generator failed");host.finishPatternClipRecording(12.5);host.edit->getTransport().stop(false,false);
    host.showRecordingOptions();clipCheck(checkedOptions->choices.at("content")->getNumItems()==3,"Recording Settings lacks Pattern Clips");
    {auto image=host.createComponentSnapshot(host.getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/pattern-clips-ui.png"));juce::PNGImageFormat().writeImageToStream(image,out);}
    clipCheck(host.preparePatternClipRecording(10) && !host.patternClipRuntime.empty(),"unfinished-take teardown fixture failed");
    std::cout<<"PASS: armed/selected routing, forward-bar queue/cancel, multi-window selection, growing preview without timer graph rebuild, native pending/committed sound, original patterns, atomic conflict retry, exact clip lengths, Stop/disarm, one Undo/Redo, save/reopen, count-in cancel, range/meter guards and hosted Play/Record.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
