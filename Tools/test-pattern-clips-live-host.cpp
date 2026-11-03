// Real hosted audio callbacks plus a pattern switch while the playhead moves.
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();
    auto& dm=host.engine.getDeviceManager();auto& device=dm.getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;params.sampleRate=48000;params.inputChannels=0;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);dm.dispatchPendingUpdates();
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();
    for(int slot:{0,1}){host.project.setPatternCreated(id,slot,true);host.project.appendPatternNoteTicks(id,slot,36+slot*24,0,512,100);}
    auto* synth=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(id));synth->parameters[8]->setParameter(1,juce::sendNotificationSync);synth->parameters[15]->setParameter(1,juce::sendNotificationSync);
    host.setPlaybackMode(ProjectView::PlaybackMode::song);host.saveOption("recordContent",2);host.saveOption("recordCountIn",0);host.setTrackRecordArm(0,true);host.playButton.onClick();
    if(!host.patternClipTake.active || !host.edit->getTransport().isPlaying())throw std::runtime_error("live performance did not start");
    dm.setCpuLimitBeforeMuting(1000);juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;
    double peak=0,lowReal=0,lowImag=0,highReal=0,highImag=0;int measured=0;
    for(int block=0;block<500;++block)
    {
        if(block==100)
        {
            host.selectGeneratorControlPattern(id,1);
            // The app dispatches Edit changes on its GUI loop. Dispatch this
            // edit update synchronously, without reallocating the test device.
            host.edit->getTransport().editHasChanged();
        }
        audio.clear();midi.clear();device.processBlock(audio,midi);peak=juce::jmax(peak,double(audio.getMagnitude(0,512)));
        for(int n=0;n<512;++n)
        {
            const double time=(block*512+n)/48000.;if(time<2.4 || time>=2.8)continue;
            const double sample=audio.getSample(0,n);const double low=juce::MathConstants<double>::twoPi*65.4063913251*time,high=juce::MathConstants<double>::twoPi*261.625565301*time;
            lowReal+=sample*std::cos(low);lowImag+=sample*std::sin(low);highReal+=sample*std::cos(high);highImag+=sample*std::sin(high);++measured;
        }
    }
    const double low=std::hypot(lowReal,lowImag),high=std::hypot(highReal,highImag);
    std::cout<<"Live switch: peak="<<peak<<" cursor="<<host.patternClipPosition()<<" C2="<<low<<" C4="<<high<<" samples="<<measured<<'\n';
    if(peak<.001 || host.patternClipPosition()<5 || high<low*2 || high<1)throw std::runtime_error("queued C4 pattern did not replace C2 in live native audio");
    if(!host.project.getPlaylistClips(id).empty())throw std::runtime_error("live launch changed authored data before Stop");
    host.stopButton.onClick();const auto clips=host.project.getPlaylistClips(id);
    if(clips.size()!=2 || clips[0].startBar!=0 || clips[0].lengthBars!=1 || clips[1].startBar!=1 || clips[1].patternId!=host.project.getPatternId(id,1))throw std::runtime_error("live performance committed different pattern boundaries");
    std::cout<<"PASS: real hosted audio advances, queued live switch changes pitch at the bar, pending document stays unchanged, Stop commits matching boundaries.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
