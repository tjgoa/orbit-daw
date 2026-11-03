static void audioFlowCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void audioFlowClick(juce::Button& button)
{
    audioFlowCheck(button.isEnabled(),"user-facing recording/transport button is disabled");
    // The isolated console harness does not run NSApplication's posted button
    // message loop. Exercise the same enabled/toggle state and production callback.
    if(button.getClickingTogglesState())button.setToggleState(!button.getToggleState(),juce::dontSendNotification);
    if(button.onClick)button.onClick();
}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);
    auto& dm=host.engine.getDeviceManager();auto& device=dm.getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters parameters;
    parameters.sampleRate=48000;parameters.inputChannels=2;parameters.outputChannels=2;device.initialise(parameters);device.prepareToPlay(48000,512);dm.dispatchPendingUpdates();dm.setCpuLimitBeforeMuting(1000);
    auto pump=[&](int count,bool signal=true)
    {
        juce::AudioBuffer<float> buffer(2,512);juce::MidiBuffer midi;float output=0;
        for(int b=0;b<count;++b)
        {
            for(int n=0;n<512;++n){buffer.setSample(0,n,signal?.19f:0);buffer.setSample(1,n,signal?-.07f:0);}
            device.processBlock(buffer,midi);output=juce::jmax(output,buffer.getMagnitude(0,512));
            if(b%8==0){host.timerCallback();dm.dispatchPendingUpdates();host.edit->dispatchPendingUpdatesSynchronously();}
        }
        return output;
    };
    host.createProjectDiscardingCurrent();host.setPlaybackMode(ProjectView::PlaybackMode::song);host.refreshProjectView("Empty Song fixture");host.syncOptionsDevices(true);dm.dispatchPendingUpdates();host.edit->dispatchPendingUpdatesSynchronously();
    audioFlowClick(host.playButton);audioFlowCheck(host.edit->getTransport().isPlaying(),"empty Song transport still waits for a generator/clip");pump(100,false);if(host.patternClipPosition()<=.5)std::cout<<"Empty Song position="<<host.patternClipPosition()<<" transport="<<host.edit->getTransport().getPosition().inSeconds()<<" playing="<<host.edit->getTransport().isPlaying()<<" status="<<host.status.getText()<<"\n";audioFlowCheck(host.patternClipPosition()>.5,"empty Song transport does not advance");audioFlowClick(host.stopButton);
    host.createProjectDiscardingCurrent();host.saveOption("recordContent",0);host.setPlaybackMode(ProjectView::PlaybackMode::pattern);host.addAudioTrack();host.showPlaylist();host.syncOptionsDevices(true);dm.dispatchPendingUpdates();host.edit->dispatchPendingUpdatesSynchronously();
    audioFlowCheck(host.project.getTrackCount()==1 && host.project.getTrackKind(0)==ProjectDocument::TrackKind::audio,"fixture contains an instrument");
    audioFlowCheck(host.playbackMode==ProjectView::PlaybackMode::song && host.playButton.isEnabled(),"first audio lane did not enable Song playback");
    audioFlowClick(host.playButton);pump(100,false);audioFlowCheck(host.edit->getTransport().isPlaying() && host.patternClipPosition()>.5,"audio-only empty Song does not play");audioFlowClick(host.stopButton);host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0));
    auto* grid=host.playlistWindow->grid;grid->setViewportArea({0,0,1200,500});host.playlistWindow->refresh();audioFlowCheck(grid->trackControls.size()==1,"audio row controls absent");
    auto& rowR=grid->trackControls[0]->record;audioFlowClick(rowR);
    audioFlowCheck(host.getTrackRecordArm(0) && host.recordContentMode()==3 && host.recordButton.isEnabled(),"audio row R did not select/enable AUDIO recording");
    host.saveOption("audioInput",false);host.applyAudioInputPolicy();dm.dispatchPendingUpdates();audioFlowClick(host.recordButton);audioFlowClick(host.playButton);
    audioFlowCheck(host.recordArmed && host.recordButton.getToggleState() && !host.audioRecordingActive && !host.edit->getTransport().isPlaying(),"input setup failure silently disarmed recording");
    audioFlowCheck(host.status.getText().contains("Enable audio input"),"missing input error not visible");audioFlowClick(host.recordButton);audioFlowCheck(!host.recordArmed && !host.recordButton.getToggleState(),"Record did not cancel failed arm");
    // Configure through the production input-settings panel, not track state.
    host.showAudioInputOptions();audioFlowCheck(checkedOptions && checkedApply,"audio input settings unavailable");
    checkedOptions->checks.at("enabled")->setToggleState(true,juce::dontSendNotification);checkedOptions->checks.at("monitor")->setToggleState(false,juce::dontSendNotification);checkedOptions->checks.at("mono")->setToggleState(false,juce::dontSendNotification);
    audioFlowCheck(checkedApply(*checkedOptions),"input settings Apply failed");checkedApply={};checkedOptions.reset();dm.dispatchPendingUpdates();host.syncOptionsDevices(true);dm.dispatchPendingUpdates();
    const auto ids=host.availableAudioInputIDs();audioFlowCheck(ids.size()==1,"expected one stereo source for automatic routing fixture");
    audioFlowClick(rowR);audioFlowCheck(!host.getTrackRecordArm(0),"audio R could not disarm");audioFlowClick(rowR);
    auto* track=host.engineTrackForProjectId(host.project.getTrackId(0));audioFlowCheck(track->state.getProperty("orbitAudioInputId").toString()==ids[0],"fresh audio track did not use its unique enabled input");
    host.setPlaybackMode(ProjectView::PlaybackMode::pattern);audioFlowClick(rowR);audioFlowClick(rowR);audioFlowCheck(host.playbackMode==ProjectView::PlaybackMode::song,"stopped audio R did not select Song mode");
    host.saveOption("recordCountIn",0);host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/audio-only.orbit"));host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0));pump(4,false); // idle hardware callbacks apply the queued seek before the clicks
    const auto beforeUndo=host.undoHistory.size();audioFlowClick(host.recordButton);audioFlowCheck(host.recordArmed && !host.audioRecordingActive,"Record started capture instead of arming");audioFlowClick(host.playButton);
    audioFlowCheck(host.audioRecordingActive && host.edit->getTransport().isRecording() && host.recordButton.getToggleState() && host.getTrackRecordArm(0),"AUDIO Record light or track arm dropped on Play");
    const auto folder=host.audioTakeFolder;pump(160);host.timerCallback();
    audioFlowCheck(host.audioRecordingActive && host.recordButton.getToggleState() && host.pendingAudioRecordingItems().size()==1,"normal GUI timer stopped/hid audio capture");
    audioFlowCheck(grid->playheadVisible && grid->playheadX>grid->labelWidth(),"audio-only Playlist playhead does not advance");
    te::RecordingThumbnailManager::Thumbnail::Ptr liveThumb;
    for(auto* input:host.edit->getAllInputDevices())if(input->isRecording() && input->getTargets().contains(track->itemID))liveThumb=host.engine.getRecordingThumbnailManager().getThumbnailFor(input->getRecordingFile(track->itemID));
    for(int retry=0;liveThumb && liveThumb->thumb->getTotalLength()<=0 && retry<100;++retry)juce::Thread::sleep(5);
    audioFlowCheck(liveThumb && liveThumb->thumb->getTotalLength()>0 && liveThumb->thumb->getApproximatePeak()>.18f,"live recording waveform has no captured sample data before Stop");
    {auto im=host.createComponentSnapshot(host.getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/audio-recording-live.png"));juce::PNGImageFormat().writeImageToStream(im,out);}
    audioFlowClick(host.stopButton);audioFlowCheck(!host.audioRecordingActive && !host.recordButton.getToggleState() && host.getTrackRecordArm(0),"Stop cleared row arm or left Record active");
    std::vector<te::WaveAudioClip*> clips;for(auto* clip:track->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip))clips.push_back(wave);
    audioFlowCheck(clips.size()==1 && host.undoHistory.size()==beforeUndo+1,"visible workflow did not create one audio take/Undo");
    const auto file=clips[0]->getOriginalFile();juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    audioFlowCheck(reader && reader->numChannels==2 && reader->lengthInSamples>4096,"visible recording workflow created no usable WAV");juce::AudioBuffer<float> decoded(2,512);reader->read(&decoded,0,512,2048,true,true);
    audioFlowCheck(std::abs(decoded.getSample(0,100)-.19f)<.0001f && std::abs(decoded.getSample(1,100)+.07f)<.0001f,"WAV does not contain the input signal/channels");
    audioFlowCheck(folder.getChildFile("recorded-take.orbit").existsAsFile(),"take recovery snapshot absent");
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0));pump(4,false);audioFlowClick(host.playButton);const auto peak=pump(100,false);audioFlowCheck(!host.audioRecordingActive && peak>.02f,"recorded WAV does not play in Song mode");audioFlowClick(host.stopButton);
    host.undoProjectAction();audioFlowCheck(file.existsAsFile(),"Undo destroyed recorded media");host.redoProjectAction();
    host.saveToFile(juce::File("OPTIONS_CHECK_FOLDER/audio-only.orbit"));audioFlowCheck(host.loadProjectFile(juce::File("OPTIONS_CHECK_FOLDER/audio-only.orbit")),"audio-only project reopen failed");
    audioFlowCheck(host.project.getTrackCount()==1 && host.playButton.isEnabled(),"reopened audio-only project disabled transport");
    // Missing explicit assignments must not be silently replaced by a different input.
    track=host.engineTrackForProjectId(host.project.getTrackId(0));track->state.setProperty("orbitAudioInputId","missing-explicit-input",nullptr);host.setTrackRecordArm(0,false);host.setTrackRecordArm(0,true);audioFlowCheck(track->state.getProperty("orbitAudioInputId").toString()=="missing-explicit-input","arming replaced an explicit unavailable input");
    std::cout<<"PASS: empty/audio-only Song transport, enabled row R/Record/Play callbacks, stopped audio auto-mode, recoverable setup errors, unique-input routing, persistent recording light and row arm, growing live recording thumbnail, GUI timer/playhead, exact captured WAV samples, Song playback, one Undo/Redo and project reopen. Hosted input; physical microphone not opened.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
