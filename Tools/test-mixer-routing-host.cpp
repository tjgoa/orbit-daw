// Real native graph renders: verify signal routing, not just menu state.
static float mixerRenderPeak(ProjectView& host,const juce::String& name)
{
    juce::WavAudioFormat wav;te::Renderer::Parameters p(*host.edit);
    p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&wav;
    p.sampleRateForAudio=48000;p.bitDepth=24;p.usePlugins=true;p.useMasterPlugins=true;
    p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
    te::Renderer::RenderTask task("Mixer routing validation",p,nullptr,nullptr);
    int attempts=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++attempts<100000){}
    mixCheck(attempts<100000 && task.errorMessage.isEmpty() && p.destFile.existsAsFile(),"native routing render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(p.destFile),true));
    mixCheck(reader!=nullptr,"routing render unreadable");juce::AudioBuffer<float> audio(2,24000);
    reader->read(&audio,0,24000,24000,true,true);return audio.getMagnitude(0,24000);
}
static void checkMixerRouting(ProjectView& host)
{
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();
    for(int bus=0;bus<4;++bus)mixCheck(host.getFxReturnTrack(bus)!=nullptr,"default Master FX return missing");
    const auto source=juce::File("OPTIONS_CHECK_FOLDER/routing-input.wav");juce::WavAudioFormat wav;
    {auto output=std::make_unique<juce::FileOutputStream>(source);std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(output.release(),48000,2,24,{},0));
     mixCheck(writer!=nullptr,"routing fixture writer failed");juce::AudioBuffer<float> input(2,48000);
     for(int n=0;n<48000;++n){const auto value=0.05f*std::sin(float(n)*0.03f);input.setSample(0,n,value);input.setSample(1,n,value);}writer->writeFromAudioSampleBuffer(input,0,48000);}
    auto* track=host.getGeneratorTracks()[0];for(auto* p:track->pluginList)if(p->isSynth())p->setEnabled(false);
    track->insertWaveClip("Routing fixture",source,{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);
    host.setTrackGainDb(0,0);host.setMasterGainDb(0);
    mixCheck(host.isMasterVolumeBeforeInserts(),"new project did not default to pre-insert Master volume");
    const auto baseline=mixerRenderPeak(host,"route-master");mixCheck(baseline>0.01,"routing fixture silent");
    mixCheck(host.setTrackOutputRoutes(0,3),"Master plus Bus 1 route refused");
    const auto parallel=mixerRenderPeak(host,"route-master-bus");mixCheck(std::abs(parallel/baseline-2)<0.04,"parallel Master/bus output did not sum exactly once per route");
    const auto bus0=[&]{return host.getMixerTracks().indexOf(host.getOutputBusTrack(0));};
    host.setTrackGainDb(bus0(),-6);mixCheck(host.setTrackOutputRoutes(0,2),"bus-only route refused");
    const auto onlyBus=mixerRenderPeak(host,"route-bus-only");mixCheck(std::abs(onlyBus/baseline-std::pow(10.0f,-6.0f/20))<0.025,"bus-only output retained a dry Master leak or lost bus level");
    host.showMixer(true);mixCheck(host.mixerWindow->getVisibleTrackCount()==5,"Master Section did not show four returns and the used bus");
    host.ensureOutputBusTrack(7);host.mixerWindow->refresh(0);mixCheck(host.mixerWindow->getVisibleTrackCount()==5,"unused bus remained visible in Master Section");
    const auto routeUndo=host.undoHistory.size();host.setTrackOutputRoutes(0,6);
    mixCheck(host.undoHistory.size()==routeUndo+1,"multi-route change was not one Undo");
    const auto twoBuses=mixerRenderPeak(host,"route-two-buses");mixCheck(std::abs(twoBuses/baseline-(1+std::pow(10.0f,-6.0f/20)))<0.04,"two simultaneous buses rendered incorrectly");
    host.undoProjectAction();mixCheck(host.getTrackOutputRoutes(0)==2,"route Undo failed");host.redoProjectAction();mixCheck(host.getTrackOutputRoutes(0)==6,"route Redo failed");
    const auto bus1=[&]{return host.getMixerTracks().indexOf(host.getOutputBusTrack(1));};
    host.setTrackOutputRoutes(bus0(),4);const auto cycleUndo=host.undoHistory.size();
    mixCheck(!host.setTrackOutputRoutes(bus1(),2) && host.undoHistory.size()==cycleUndo,"bus feedback cycle was accepted or changed Undo");
    mixCheck(!host.setTrackOutputRoutes(bus0(),2),"self bus route accepted");host.setTrackOutputRoutes(bus0(),1);
    // Internal route nodes must not exhaust the engine's default plugin budget.
    for(int bus=0;bus<8;++bus){host.ensureOutputBusTrack(bus);host.setTrackGainDb(host.getMixerTracks().indexOf(host.getOutputBusTrack(bus)),0);}
    mixCheck(host.setTrackOutputRoutes(0,511),"all eight buses plus Master route refused");
    mixCheck(std::abs(mixerRenderPeak(host,"route-all-nine")/baseline-9)<.08,"all-bus graph lost a route to the engine plugin budget");
    // A disconnected dry output must still feed an independent FX send.
    host.setTrackSendGainDb(0,0,0);auto* fx=host.getFxReturnTrack(0);for(auto* p:fx->pluginList)if(dynamic_cast<te::ReverbPlugin*>(p)!=nullptr)p->setEnabled(false);
    const auto fxIndex=host.getMixerTracks().indexOf(fx);host.setTrackGainDb(fxIndex,0);
    host.setTrackOutputRoutes(0,0);host.setTrackSendLinked(0,0,false);host.setTrackGainDb(0,0);
    const auto pre=mixerRenderPeak(host,"send-pre");host.setTrackGainDb(0,-20);
    const auto preQuiet=mixerRenderPeak(host,"send-pre-quiet");mixCheck(pre>0.01 && std::abs(preQuiet/pre-1)<0.025,"unlinked send changed with the channel fader or disconnected output lost sends");
    host.setTrackSendLinked(0,0,true);const auto postQuiet=mixerRenderPeak(host,"send-post-quiet");
    mixCheck(std::abs(postQuiet/pre-0.1)<0.015,"linked send did not follow channel gain");
    host.setTrackSendGainDb(0,0,-100);host.setTrackOutputRoutes(0,1);host.setTrackGainDb(0,0);
    // A compressor makes pre/post Master volume order audibly measurable.
    const auto master=host.getMixerTracks().size();host.addInsert(master,1);
    auto* compressor=dynamic_cast<te::CompressorPlugin*>(host.getInsertPlugins(master)[0]);mixCheck(compressor!=nullptr,"Master compressor absent");
    compressor->setThreshold(0.01f);compressor->setRatio(0.05f);compressor->attackMs.setParameter(0.1f,juce::sendNotification);compressor->releaseMs.setParameter(20,juce::sendNotification);
    host.setMasterGainDb(-20);const auto before=mixerRenderPeak(host,"master-pre-compressor");const auto orderUndo=host.undoHistory.size();
    host.setMasterVolumeBeforeInserts(false);const auto after=mixerRenderPeak(host,"master-post-compressor");
    mixCheck(before>after*2 && host.undoHistory.size()==orderUndo+1,"Master volume did not move across the nonlinear insert with one Undo");
    host.undoProjectAction();mixCheck(host.isMasterVolumeBeforeInserts() && std::abs(host.getMasterGainDb()+20)<0.01,"Master order Undo lost gain or order");host.redoProjectAction();
    host.setPlaybackMode(ProjectView::PlaybackMode::song);
    host.showSongEventEditor("mixer:master");auto oldTarget=host.targetForParameter(*host.masterVolumePlugin()->volParam);
    orbit::automation::Points points;points.add({0.125,0.3f});points.add({4.5,0.8f});host.project.replaceSongAutomationPoints(oldTarget.trackId,oldTarget.pluginName,oldTarget.parameterId,oldTarget.parameterName,points,oldTarget.pluginId);
    const auto oldPan=host.targetForParameter(*host.masterVolumePlugin()->panParam);
    host.project.replaceSongAutomationPoints(oldPan.trackId,oldPan.pluginName,oldPan.parameterId,oldPan.parameterName,points,{});
    host.updateEngineAutomation();host.setMasterVolumeBeforeInserts(true);const auto newTarget=host.targetForParameter(*host.masterVolumePlugin()->volParam);
    mixCheck(newTarget.pluginId!=oldTarget.pluginId && host.songEventPoints(newTarget).size()==2,"Master order change lost exact-instance Song automation");
    mixCheck(host.songEventPoints(host.targetForParameter(*host.masterVolumePlugin()->panParam)).size()==2,"legacy Master pan did not migrate to the pre-insert parameter ID");
    mixCheck(host.masterVolumePlugin()->volParam->getCurve().getNumPoints()==2 && std::abs(host.masterVolumePlugin()->volParam->getCurve().getPointTime(0).inSeconds()-0.015625)<0.000001,"migrated Master Song curve lost native engine timing");
    mixCheck(host.edit->getMasterVolumePlugin()->volParam->getCurve().getNumPoints()==0 && std::abs(host.edit->getMasterVolumePlugin()->getVolumeDb())<0.01,"neutral final Master retained duplicate gain/automation");
    host.setTrackOutputRoutes(0,7);const auto file=juce::File("OPTIONS_CHECK_FOLDER/mixer-routes.orbit");host.saveToFile(file);mixCheck(host.loadProjectFile(file),"route/order native reopen failed");
    mixCheck(host.getTrackOutputRoutes(0)==7 && host.isMasterVolumeBeforeInserts() && host.songEventPoints(host.targetForParameter(*host.masterVolumePlugin()->volParam)).size()==2,"route/order/automation lost on reopen");
    std::cout<<"PASS: native graph renders verify parallel Master/bus sums, bus-only isolation, two buses, unlinked/linked send gain, disconnected dry output with sends, nonlinear Master pre/post insert order; feedback refusal, Undo and exact automation ID migration/reopen.\n";
}
