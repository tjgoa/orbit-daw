static void audioCheck(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static std::vector<te::WaveAudioClip*> audioWaves(ProjectView& host,int row)
{std::vector<te::WaveAudioClip*> result;for(auto* clip:host.getGeneratorTracks()[row]->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip))result.push_back(wave);return result;}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();
    auto& dm=host.engine.getDeviceManager();auto& device=dm.getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;
    params.sampleRate=48000;params.inputChannels=2;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);dm.dispatchPendingUpdates();dm.setCpuLimitBeforeMuting(1000);
    host.createProjectDiscardingCurrent();host.addAudioTrack();host.setPlaybackMode(ProjectView::PlaybackMode::song);
    host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/audio.orbit"));
    host.saveOption("audioInput",true);host.saveOption("audioMonitor",false);host.saveOption("audioMono",false);host.saveOption("audioDepth",1);host.applyAudioInputPolicy();dm.dispatchPendingUpdates();
    auto ids=host.availableAudioInputIDs();std::cout<<"Inputs: "<<ids.joinIntoString(",")<<"\n";audioCheck(ids.size()>=1,"hosted inputs not available");
    auto id=host.selectedProjectTrackId();host.engineTrackForProjectId(id)->state.setProperty("orbitAudioInputId",ids[0],nullptr);host.syncAudioRecordingInputs();
    host.setTrackRecordArm(0,true);host.saveOption("recordCountIn",0);host.saveOption("recordContent",3);
    auto run=[&](int blocks,bool signal=true)
    {juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;for(int b=0;b<blocks;++b){for(int n=0;n<512;++n){audio.setSample(0,n,signal?.25f:0);audio.setSample(1,n,signal?-.125f:0);}device.processBlock(audio,midi);}};
    const auto undo=host.undoHistory.size();host.recordButton.onClick();audioCheck(host.recordArmed,"Record did not arm audio");host.playButton.onClick();
    audioCheck(host.audioRecordingActive && host.edit->getTransport().isRecording(),"audio take did not start");
    const auto takeFolder=host.audioTakeFolder;run(200);audioCheck(host.pendingAudioRecordingItems().size()==1,"pending recording preview absent");audioCheck(audioWaves(host,0).empty(),"audio clips committed before Stop");
    host.setTrackGainDb(0,-6);audioCheck(host.audioRecordingActive,"mixer adjustment stopped audio capture");
    host.stopButton.onClick();audioCheck(!host.audioRecordingActive,"Stop left audio take active");auto clips=audioWaves(host,0);
    audioCheck(clips.size()==1 && host.undoHistory.size()==undo+1,"audio Stop did not commit one clip and one Undo");
    auto file=clips[0]->getOriginalFile();audioCheck(file.existsAsFile(),"recording media missing");juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    audioCheck(reader && reader->numChannels==2 && reader->sampleRate==48000 && reader->bitsPerSample==24,"stereo WAV format wrong");
    juce::AudioBuffer<float> decoded(2,4096);reader->read(&decoded,0,4096,4096,true,true);audioCheck(std::abs(decoded.getSample(0,100)-.25f)<1e-5 && std::abs(decoded.getSample(1,100)+.125f)<1e-5,"recorded signal or channel identity wrong");
    std::cout<<"WAV: "<<file.getFullPathName()<<" frames="<<reader->lengthInSamples<<" clip="<<clips[0]->getPosition().getLength().inSeconds()<<"\n";
    audioCheck(takeFolder.getChildFile("recorded-take.orbit").existsAsFile(),"committed recovery missing");
    host.undoProjectAction();audioCheck(std::abs(host.getGeneratorTracks()[0]->getVolumePlugin()->getVolumeDb()+6)<.01,"audio Undo reverted live mixer adjustment");audioCheck(audioWaves(host,0).empty() && file.existsAsFile(),"Undo deleted media or left clip");host.redoProjectAction();audioCheck(audioWaves(host,0).size()==1,"Redo did not restore audio clip");
    host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/audio.orbit"));audioCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/audio.orbit")),"audio project reopen failed");
    audioCheck(audioWaves(host,0).size()==1 && audioWaves(host,0)[0]->getOriginalFile().existsAsFile(),"reopen lost recorded audio");
    audioCheck(!host.prepareAudioRecording(0),"recording overwrote occupied region");host.loopButton.setToggleState(true,juce::dontSendNotification);audioCheck(!host.prepareAudioRecording(4),"loop recording silently accepted");host.loopButton.setToggleState(false,juce::dontSendNotification);
    // Two tracks from one stereo source, one take Undo; native count-in excludes
    // preroll signal. Record again punches out and leaves playback running.
    host.addAudioTrack();auto second=host.selectedProjectTrackId();host.engineTrackForProjectId(second)->state.setProperty("orbitAudioInputId",ids[0],nullptr);host.setTrackRecordArm(1,true);host.syncAudioRecordingInputs();
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(4));host.saveOption("recordCountIn",3);host.recordButton.onClick();host.playButton.onClick();audioCheck(host.audioRecordingActive,"multitrack count-in start failed");
    run(300);host.updateAudioRecording();audioCheck(host.status.getText().startsWith("Recording audio inputs"),"count-in status did not change to recording");host.recordButton.onClick();audioCheck(!host.audioRecordingActive && host.edit->getTransport().isPlaying(),"Record punch-out stopped playback");
    audioCheck(audioWaves(host,0).size()==2 && audioWaves(host,1).size()==1,"multitrack recording lost a destination");
    for(auto* clip:audioWaves(host,1))audioCheck(std::abs(clip->getPosition().getStart().inSeconds()-4)<1e-6 && clip->getPosition().getLength().inSeconds()<1.3,"count-in leaked into recorded clip");host.stopButton.onClick();
    const auto beforeCancel=host.undoHistory.size();host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(8));host.recordButton.onClick();host.playButton.onClick();run(10);host.stopButton.onClick();audioCheck(host.undoHistory.size()==beforeCancel && std::abs(host.edit->getTransport().getPosition().inSeconds()-8)<1e-6,"cancelled count-in added Undo or lost cursor");
    host.saveOption("audioMono",true);host.applyAudioInputPolicy();dm.dispatchPendingUpdates();ids=host.availableAudioInputIDs();audioCheck(ids.size()>=2,"mono inputs unavailable");
    host.engineTrackForProjectId(id)->state.setProperty("orbitAudioInputId",ids[0],nullptr);host.engineTrackForProjectId(second)->state.setProperty("orbitAudioInputId",ids[1],nullptr);host.syncAudioRecordingInputs();host.saveOption("recordCountIn",0);host.saveOption("audioDepth",0);host.applyAudioInputPolicy();
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(8));host.recordButton.onClick();host.playButton.onClick();audioCheck(host.audioRecordingActive,"mono capture failed");run(100);host.stopButton.onClick();
    auto mono=audioWaves(host,1).back()->getOriginalFile();std::unique_ptr<juce::AudioFormatReader> monoReader(formats.createReaderFor(mono));audioCheck(monoReader && monoReader->numChannels==1 && monoReader->bitsPerSample==16,"mono WAV format wrong");juce::AudioBuffer<float> one(1,128);monoReader->read(&one,0,128,2048,true,false);audioCheck(std::abs(one.getSample(0,20)+.125f)<.001,"mono right input recorded left channel");
    // Existing later clips bound all capture destinations. Overshoot the GUI
    // timer deliberately: the native commit range still ends at the boundary.
    host.syncAudioRecordingInputs();auto* track=host.engineTrackForProjectId(id);auto later=track->insertWaveClip("Later",file,{{tracktion::TimePosition::fromSeconds(12),tracktion::TimePosition::fromSeconds(13)},{}},false);
    audioCheck(later!=nullptr,"later-clip fixture failed");const auto laterID=later->itemID.toString();later=nullptr;
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(11));host.recordButton.onClick();host.playButton.onClick();audioCheck(host.audioRecordingActive && host.audioRecordingLimit==12,"future clip did not bound take");run(150);host.updateAudioRecording();audioCheck(!host.audioRecordingActive && !host.edit->getTransport().isPlaying(),"take did not stop before later content");
    bool foundLater=false;for(auto* clip:audioWaves(host,0)){if(clip->itemID.toString()==laterID)foundLater=clip->getPosition().getStart().inSeconds()==12;else if(clip->getPosition().getStart().inSeconds()>10)audioCheck(clip->getPosition().getEnd().inSeconds()<=12+1e-6,"take overlapped later content");}audioCheck(foundLater,"recording replaced existing later audio");
    host.showAudioTrackRecordingOptions(id);audioCheck(checkedOptions && checkedOptions->choices.at("source")->getSelectedItemIndex()>0,"audio input selector not restored");checkedApply={};checkedOptions.reset();
    // Monitor off stays silent, Monitor on routes the selected mono input.
    host.saveOption("audioMonitor",true);host.applyAudioInputPolicy();host.syncAudioRecordingInputs();host.edit->getTransport().ensureContextAllocated();host.edit->getTransport().editHasChanged();
    juce::AudioBuffer<float> monitored(2,512);juce::MidiBuffer m;float peak=0;for(int b=0;b<20;++b){for(int n=0;n<512;++n){monitored.setSample(0,n,.2f);monitored.setSample(1,n,.025f);}device.processBlock(monitored,m);peak=juce::jmax(peak,monitored.getMagnitude(0,512));}audioCheck(peak>.05f,"assigned input monitoring silent");
    host.saveOption("audioMonitor",false);host.applyAudioInputPolicy();host.edit->getTransport().editHasChanged();peak=0;for(int b=0;b<20;++b){for(int n=0;n<512;++n){monitored.setSample(0,n,.2f);monitored.setSample(1,n,.025f);}device.processBlock(monitored,m);if(b>10)peak=juce::jmax(peak,monitored.getMagnitude(0,512));}audioCheck(peak<1e-6f,"Monitor off still audible");
    // Device-loss policy is exercised by invalidating a pinned input ID while
    // native capture is live; remaining WAV data must still be committed.
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(14));host.recordButton.onClick();host.playButton.onClick();audioCheck(host.audioRecordingActive,"disconnect fixture failed");run(40);const auto lostFolder=host.audioTakeFolder;
    host.engineTrackForProjectId(id)->state.setProperty("orbitAudioInputId","unavailable-input",nullptr);host.updateAudioRecording();audioCheck(!host.audioRecordingActive && !host.edit->getTransport().isPlaying(),"missing input did not stop recording");
    audioCheck(juce::JSON::parse(lostFolder.getChildFile("take.json")).getProperty("state",{}).toString()=="needs-review" && lostFolder.getChildFile("recorded-take.orbit").existsAsFile(),"missing input discarded recovery evidence");
    // A native engine stop can commit before the host timer sees it. It must
    // still belong to exactly one take, including Undo and recovery snapshots.
    host.engineTrackForProjectId(id)->state.setProperty("orbitAudioInputId",ids[0],nullptr);host.syncAudioRecordingInputs();host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(16));const auto nativeStopUndo=host.undoHistory.size();const auto oldCount=audioWaves(host,0).size();
    host.recordButton.onClick();host.playButton.onClick();audioCheck(host.audioRecordingActive,"engine-stop fixture failed");run(50);host.setTrackGainDb(0,-3);const auto nativeStopFolder=host.audioTakeFolder;host.edit->getTransport().stop(false,false);host.updateAudioRecording();
    audioCheck(!host.audioRecordingActive && host.undoHistory.size()==nativeStopUndo+1 && audioWaves(host,0).size()==oldCount+1,"engine-initiated stop lost take transaction");
    audioCheck(nativeStopFolder.getChildFile("recorded-take.orbit").existsAsFile(),"engine stop lost recovery snapshot");host.undoProjectAction();audioCheck(audioWaves(host,0).size()==oldCount && std::abs(host.getGeneratorTracks()[0]->getVolumePlugin()->getVolumeDb()+3)<.01,"engine-stop Undo left recording or reverted mixer adjustment");host.redoProjectAction();audioCheck(audioWaves(host,0).size()==oldCount+1,"engine-stop Redo lost recording");
    audioCheck(host.loadProjectFile(nativeStopFolder.getChildFile("recorded-take.orbit")),"take recovery snapshot could not open");audioCheck(audioWaves(host,0).size()==oldCount+1 && audioWaves(host,0).back()->getOriginalFile().existsAsFile(),"recovery snapshot lost WAV references");
    std::cout<<"PASS: native hosted input capture, stereo/mono and channel separation, WAV depth/rate, stopped Record/Play, count-in and cancellation, multi-track, punch-out, occupied-region/loop rejection, single Undo/Redo, durable recovery and project reopen. Hardware disabled.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
