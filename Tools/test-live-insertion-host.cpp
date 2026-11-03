static void insertionCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();
    auto& audio=host.engine.getDeviceManager().getHostedAudioDeviceInterface();
    te::HostedAudioDeviceInterface::Parameters setup;setup.inputChannels=0;setup.outputChannels=2;setup.sampleRate=44100;setup.blockSize=256;audio.initialise(setup);audio.prepareToPlay(44100,256);host.engine.getDeviceManager().dispatchPendingUpdates();
    auto* output=host.engine.getDeviceManager().getWaveOutDevice(0);insertionCheck(output!=nullptr,"hosted output missing");output->setEnabled(true);host.engine.getDeviceManager().setDefaultWaveOutDevice(output->getDeviceID());host.engine.getDeviceManager().dispatchPendingUpdates();
    if(argc>1 && juce::String(argv[1])=="--bootstrap-device")
    {host.engine.getDeviceManager().saveSettings();host.applicationProperties.saveIfNeeded();std::cout<<"Hosted audio fixture routing initialized.\n";return 0;}
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto acid=host.selectedProjectTrackId();
    host.project.setPatternCreated(acid,0,true);host.project.appendPatternNoteTicks(acid,0,48,0,960,110);host.project.setPatternLengthSteps(acid,0,16);host.project.setInitialTempoBpm(240);host.tempoSlider.setValue(240,juce::dontSendNotification);host.refreshEnginePlaylist();
    auto render=[&](double seconds)
    {
        host.engine.getDeviceManager().dispatchPendingUpdates();host.edit->dispatchPendingUpdatesSynchronously();juce::MessageManager::getInstance()->runDispatchLoopUntil(100);host.edit->getTransport().ensureContextAllocated();juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;double energy=0;
        std::atomic<bool> done{false};
        std::thread renderThread([&]
        {
            for(int block=0;block<int(seconds*44100/256);++block){buffer.clear();midi.clear();audio.processBlock(buffer,midi);for(int channel=0;channel<2;++channel)energy+=buffer.getRMSLevel(channel,0,256);juce::Thread::sleep(6);}
            done=true;
        });
        while(!done)juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
        renderThread.join();return energy;
    };
    const juce::KeyPress space(juce::KeyPress::spaceKey);insertionCheck(host.keyPressed(space),"Space was not handled");
    insertionCheck(host.edit->getTransport().isPlaying() && host.auditionPatternPlayback && bool(host.edit->getTransport().looping),"Space failed to start Pattern playback");
    // The test attaches its hosted device after ProjectView construction.
    // Prime that late-attached graph before the uninterrupted insertion checks.
    render(0.3);host.handleEditShortcut(space);host.handleEditShortcut(space);
    const auto firstEnergy=render(2.3);std::cout<<"Output devices="<<host.engine.getDeviceManager().getNumWaveOutDevices()<<" position="<<host.edit->getTransport().getPosition().inSeconds()<<" Initial audio energy "<<firstEnergy<<'\n';insertionCheck(firstEnergy>0.01,"hosted audio output has no audible synth signal");
    auto stillPlaying=[&](const char* label)
    {
        const double energy=render(2.3);std::cout<<label<<" energy="<<energy<<" playing="<<host.edit->getTransport().isPlaying()<<" loop="<<bool(host.edit->getTransport().looping)<<'\n';
        insertionCheck(host.edit->getTransport().isPlaying() && host.auditionPatternPlayback && bool(host.edit->getTransport().looping),"insertion stopped Pattern transport or its repeating loop");
        insertionCheck(energy>0.01,"insertion silenced audio across multiple pattern cycles");insertionCheck(host.project.getPatternNotes(acid,0).size()==1,"insertion changed original note data");
    };
    host.addInstrumentTrack();stillPlaying("4OSC insert");host.addDrumRackTrack();stillPlaying("Drum Rack insert");host.addInstrumentTrack(true);stillPlaying("Acid Bass insert");
    const juce::String path=GENERATOR_VST_PATH_LITERAL;if(path.isNotEmpty())
    {juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,path);insertionCheck(!found.isEmpty(),"VST fixture scan failed");host.addVst3Instrument(*found[0]);stillPlaying("VST3 insert");}
    host.loopStartSlider.setValue(7,juce::dontSendNotification);host.loopLengthSlider.setValue(3,juce::dontSendNotification);host.loopButton.setToggleState(false,juce::dontSendNotification);host.updateLoopRange();stillPlaying("Song loop edited while in Pattern mode");
    insertionCheck(host.handleEditShortcut(space) && !host.edit->getTransport().isPlaying(),"Space failed to stop playback");
    host.project.setPlaylistClip(acid,host.project.getPatternId(acid,0),0,0,true);host.refreshEnginePlaylist();host.setPlaybackMode(ProjectView::PlaybackMode::song);host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(0));host.handleEditShortcut(space);
    insertionCheck(host.edit->getTransport().isPlaying() && !bool(host.edit->getTransport().looping),"Song Space start or disabled Song loop was changed");
    host.addInstrumentTrack();insertionCheck(host.edit->getTransport().isPlaying() && !host.auditionPatternPlayback && !bool(host.edit->getTransport().looping),"Song insertion changed transport or loop mode");host.handleEditShortcut(space);insertionCheck(!host.edit->getTransport().isPlaying(),"Song Space stop failed");
    host.handleEditShortcut(space);insertionCheck(host.edit->getTransport().isPlaying(),"disconnect fixture failed to start");host.engine.getDeviceManager().deviceManager.closeAudioDevice();host.handleEditShortcut(space);insertionCheck(!host.edit->getTransport().isPlaying(),"Space could not stop after audio output disconnected");
    std::cout<<"PASS: actual hosted stereo audio across multiple cycles after 4OSC/Drum Rack/Acid/VST insertion; Pattern loop unaffected by Song controls; Song transport preserved; Space starts/stops Pattern and Song. No hardware used.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
