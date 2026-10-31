static void sideCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void sideFixture(const juce::File& file,float amplitude,double frequency,bool pulses)
{
    juce::WavAudioFormat wav;auto stream=std::make_unique<juce::FileOutputStream>(file);
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));sideCheck(writer!=nullptr,"sidechain fixture writer failed");
    juce::AudioBuffer<float> audio(2,48000);
    for(int i=0;i<48000;++i)
    {
        const double t=i/48000.0;const bool on=!pulses || std::fmod(t,0.5)<0.25;
        const float value=on?amplitude*float(std::sin(t*frequency*juce::MathConstants<double>::twoPi)):0;
        audio.setSample(0,i,value);audio.setSample(1,i,value);
    }
    writer->writeFromAudioSampleBuffer(audio,0,48000);
}
static juce::AudioBuffer<float> sideRender(ProjectView& host,const juce::String& name)
{
    juce::WavAudioFormat wav;te::Renderer::Parameters p(*host.edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&wav;
    p.sampleRateForAudio=48000;p.bitDepth=24;p.usePlugins=true;p.useMasterPlugins=true;p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
    te::Renderer::RenderTask task("Sidechain validation",p,nullptr,nullptr);int attempts=0;
    while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++attempts<100000){}
    sideCheck(attempts<100000 && task.errorMessage.isEmpty(),"sidechain render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(p.destFile),true));sideCheck(reader!=nullptr,"sidechain render unreadable");
    juce::AudioBuffer<float> audio(2,48000);reader->read(&audio,0,48000,0,true,true);return audio;
}
static double sideToneAmplitude(const juce::AudioBuffer<float>& audio,double start,double end,double frequency)
{
    const int first=int(start*48000),last=int(end*48000);double real=0,imaginary=0;
    for(int i=first;i<last;++i){const auto angle=i*frequency*juce::MathConstants<double>::twoPi/48000;real+=audio.getSample(0,i)*std::cos(angle);imaginary+=audio.getSample(0,i)*std::sin(angle);}
    return 2*std::sqrt(real*real+imaginary*imaginary)/(last-first);
}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1220,850);
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/sidechain.orbit");
    if(argc>1)
    {
        sideCheck(host.loadProjectFile(file),"fresh sidechain project load failed");
        auto* p=dynamic_cast<te::CompressorPlugin*>(host.getInsertPlugins(0)[0]);sideCheck(p!=nullptr && p->useSidechainTrigger.get() && p->getSidechainSourceID()==host.getGeneratorTracks()[1]->itemID,"sidechain IDs/trigger lost in fresh process");
        sideCheck(host.isMasterMuted() && host.masterInsertEnd()<host.edit->getMasterPluginList().size(),"Master mute gate lost in fresh process");
        sideCheck(sideRender(host,"fresh-muted").getMagnitude(0,48000)<0.000001,"muted project leaked audio in a fresh process");
        std::cout<<"PASS: sidechain routing and automated Master mute restored in a fresh process.\n";return 0;
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
    const std::array<juce::File,2> inputs{juce::File("OPTIONS_CHECK_FOLDER/side-bass.wav"),juce::File("OPTIONS_CHECK_FOLDER/side-trigger.wav")};
    sideFixture(inputs[0],0.01f,1000,false);sideFixture(inputs[1],0.6f,100,true);
    for(int n=0;n<2;++n)
    {
        auto* track=host.getGeneratorTracks()[n];for(auto* p:track->pluginList)if(p->isSynth())p->setEnabled(false);
        track->insertWaveClip(n==0?"Bass":"Trigger",inputs[size_t(n)],{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);
        host.setTrackGainDb(n,0);
    }
    host.setPlaybackMode(ProjectView::PlaybackMode::song);host.addInsert(0,1);
    auto* compressor=dynamic_cast<te::CompressorPlugin*>(host.getInsertPlugins(0)[0]);sideCheck(compressor!=nullptr && compressor->canSidechain(),"compressor has no sidechain capability");
    const auto pluginId=compressor->itemID.toString(),sourceId=host.getGeneratorTracks()[1]->itemID.toString();
    compressor->setThreshold(0.02f);compressor->setRatio(0.05f);compressor->attackMs.setParameter(0.1f,juce::sendNotification);compressor->releaseMs.setParameter(10,juce::sendNotification);
    const auto dry=sideRender(host,"side-off");const auto original=sideToneAmplitude(dry,.6,.7,1000);sideCheck(original>.005,"bass fixture was silent");
    const auto undo=host.undoHistory.size();sideCheck(host.setEffectSidechain(pluginId,sourceId),"sidechain source refused");
    sideCheck(host.undoHistory.size()==undo+1,"sidechain assignment split Undo");const auto ducked=sideRender(host,"side-on");
    const auto active=sideToneAmplitude(ducked,.6,.7,1000),rest=sideToneAmplitude(ducked,.35,.45,1000);
    std::cout<<"Sidechain bass amplitude: off="<<original<<" active="<<active<<" rest="<<rest<<"\n";
    sideCheck(active<original*.3 && rest>original*.8,"sidechain did not duck/recover the destination's tone");
    host.undoProjectAction();sideCheck(!host.sidechainPluginById(pluginId)->getSidechainSourceID().isValid(),"sidechain Undo failed");host.redoProjectAction();
    sideCheck(host.sidechainPluginById(pluginId)->getSidechainSourceID().toString()==sourceId,"sidechain Redo failed");
    host.openInsertEditor(0,0);auto* window=host.mixerWindow->effectWindow.get();
    sideCheck(window!=nullptr && window->sidechainButton.isVisible() && bool(window->onChooseSidechain),"effect sidechain selector missing");
    sideCheck(window->sidechainButton.getButtonText().contains(host.getGeneratorTracks()[1]->getName()),"sidechain label does not identify source");
    const auto feedbackUndo=host.undoHistory.size();sideCheck(!host.setEffectSidechain(pluginId,host.getGeneratorTracks()[0]->itemID.toString()) && host.undoHistory.size()==feedbackUndo,"self sidechain accepted or changed Undo");
    host.addInsert(1,1);const auto other=host.getInsertPlugins(1)[0]->itemID.toString();sideCheck(!host.setEffectSidechain(other,host.getGeneratorTracks()[0]->itemID.toString()),"mutual sidechain feedback accepted");
    host.setTrackOutputRoutes(0,2);auto* bus=host.getOutputBusTrack(0);sideCheck(!host.setEffectSidechain(pluginId,bus->itemID.toString()),"sidechain bus feedback accepted");host.setTrackOutputRoutes(0,1);
    host.setTrackGainDb(1,-60);sideCheck(sideToneAmplitude(sideRender(host,"side-quiet-trigger"),.6,.7,1000)>original*.8,"sidechain ignored source level");host.setTrackGainDb(1,0);
    sideCheck(host.setEffectSidechain(pluginId,{}),"sidechain disconnect refused");
    sideCheck(sideToneAmplitude(sideRender(host,"side-disconnected"),.6,.7,1000)>original*.8,"disconnect did not return to self detection");host.setEffectSidechain(pluginId,sourceId);
    // Playback updates displayed host sliders without writing a second parameter edit.
    {auto parameter=host.sidechainPluginById(pluginId)->getAutomatableParameter(0);const auto originalParameter=parameter->getCurrentValue();int changes=0;
    PluginParameterWindow::ParameterRow row(parameter,[&]{++changes;});parameter->setNormalisedParameter(.25f,juce::sendNotification);row.refreshFromParameter();
    sideCheck(std::abs(row.value.getValue()-parameter->getCurrentValue())<.001 && changes==0,"parameter display refresh wrote an edit or failed to follow native value");
    row.value.onDragStart();const auto held=row.value.getValue();parameter->setNormalisedParameter(.75f,juce::sendNotification);row.refreshFromParameter();
    sideCheck(row.value.getValue()==held,"automation display fought an active drag");row.value.onDragEnd();sideCheck(std::abs(row.value.getValue()-parameter->getCurrentValue())<.001,"display did not resume after drag");
    parameter->setParameter(originalParameter,juce::sendNotification);}
    host.showMixer(false);host.setTrackGainDb(0,-7);host.mixerWindow->refreshControlValues();sideCheck(std::abs(host.mixerWindow->channelStrips[0]->gain.getValue()+7)<.01,"mixer fader did not follow native control value");
    host.setTrackGainDb(0,0);
    // Master mute must stay silent even when level automation is actively read.
    const auto target=host.targetForParameter(*host.masterVolumePlugin()->volParam);orbit::automation::Points curve;curve.add({0,0.9f});curve.add({8,0.6f});
    host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,curve,target.pluginId);host.updateEngineAutomation();
    const auto muteUndo=host.undoHistory.size();host.setMasterMuted(true);sideCheck(host.undoHistory.size()==muteUndo+1,"Master mute did not create one Undo");
    sideCheck(sideRender(host,"master-muted-automation").getMagnitude(0,48000)<.000001,"Master automation overrode mute");
    host.addInsert(host.getMixerTracks().size(),3);host.ensureMasterMuteGate();sideCheck(host.masterInsertEnd()==host.edit->getMasterPluginList().size()-1,"new Master insert was placed after the mute gate");
    host.setMasterMuted(false);sideCheck(sideRender(host,"master-unmuted").getMagnitude(0,48000)>.001,"Master unmute left a gate or lost signal");
    host.setMasterMuted(true);host.saveToFile(file);sideCheck(host.loadProjectFile(file),"sidechain/mute native reopen failed");
    sideCheck(host.isMasterMuted() && host.sidechainPluginById(pluginId)->getSidechainSourceID().toString()==sourceId,"sidechain or mute state lost on reopen");
    // Upgrade the old mute representation without discarding its saved level.
    host.edit->state.setProperty(host.masterMuteProperty(),false,nullptr);host.ensureMasterMuteGate();
    host.masterVolumePlugin()->setVolumeDb(-100);host.edit->state.setProperty(host.masterPreMuteGainProperty(),-7.0,nullptr);
    host.edit->state.setProperty(host.masterMuteProperty(),true,nullptr);host.ensureMasterMuteGate();
    sideCheck(std::abs(host.getMasterGainDb()+7)<.01 && sideRender(host,"legacy-master-mute").getMagnitude(0,48000)<.000001,"legacy Master mute failed to restore its level behind a silent gate");
    host.setMasterMuted(false);sideCheck(sideRender(host,"legacy-master-unmute").getMagnitude(0,48000)>.001,"legacy Master unmute stayed silent");
    // Do not keep test parameter handles past an Edit replacement.
    std::cout<<"PASS: native sidechain duck/recovery renders, source level/disconnect, self/mutual/bus feedback refusal, selector binding, one Undo/Redo and save/reopen; steady automation displays and drag protection; automated Master mute remains silent.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
