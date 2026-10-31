static void distortionCheck(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static double distortionRate=48000;
static juce::AudioBuffer<float> distortionRender(ProjectView& host,const juce::String& name)
{
    juce::WavAudioFormat format;te::Renderer::Parameters p(*host.edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&format;p.sampleRateForAudio=distortionRate;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;
    p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
    te::Renderer::RenderTask task("Distortion validation",p,nullptr,nullptr);int n=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++n<100000){}
    distortionCheck(n<100000 && task.errorMessage.isEmpty(),"native distortion render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(p.destFile),true));distortionCheck(reader!=nullptr,"distortion render unreadable");
    juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));reader->read(&audio,0,audio.getNumSamples(),0,true,true);return audio;
}
static double distortionTone(const juce::AudioBuffer<float>& audio,double hz,double start=.2,double end=.8)
{double real=0,imaginary=0;int count=0;for(int i=int(distortionRate*start);i<int(distortionRate*end);++i){const double angle=i*hz*juce::MathConstants<double>::twoPi/distortionRate;real+=audio.getSample(0,i)*std::cos(angle);imaginary+=audio.getSample(0,i)*std::sin(angle);++count;}return 2*std::hypot(real,imaginary)/count;}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1280,850);const juce::File file("OPTIONS_CHECK_FOLDER/distortion.orbit");
    if(argc>1)
    {
        distortionCheck(host.loadProjectFile(file),"fresh distortion project failed");auto plugins=host.getInsertPlugins(0);
        distortionCheck(plugins.size()==2 && dynamic_cast<orbit::mixing::TubeDistortionPlugin*>(plugins[0]) && dynamic_cast<orbit::mixing::TwoBandDistortionPlugin*>(plugins[1]),"fresh distortion native types absent");
        auto* tube=dynamic_cast<orbit::mixing::DistortionPluginBase*>(plugins[0]);distortionCheck(tube->parameters[0]->getCurrentValue()==24,"fresh tube drive lost");const auto audio=distortionRender(host,"fresh-distortion");distortionCheck(distortionTone(audio,9000)>.001,"fresh effects silent");std::cout<<"PASS: fresh distortion native types, controls and DSP render.\n";return 0;
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
    for(int row=0;row<2;++row){auto* track=host.getGeneratorTracks()[row];for(auto* p:track->pluginList)if(p->isSynth())p->setEnabled(false);host.setTrackGainDb(row,0);host.setTrackEqEnabled(row,false);}
    host.setMasterGainDb(0);host.setTrackEqEnabled(host.getMixerTracks().size(),false);host.setPlaybackMode(ProjectView::PlaybackMode::song);
    juce::File fixture("OPTIONS_CHECK_FOLDER/distortion-fixture.wav");juce::WavAudioFormat format;
    {auto stream=std::make_unique<juce::FileOutputStream>(fixture);std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(),48000,2,24,{},0));juce::AudioBuffer<float> samples(2,48000);for(int i=0;i<48000;++i){const float value=i==0?.23f:float(.15*std::sin(i*9000*juce::MathConstants<double>::twoPi/48000));samples.setSample(0,i,value);samples.setSample(1,i,-value);}writer->writeFromAudioSampleBuffer(samples,0,48000);}
    host.getGeneratorTracks()[0]->insertWaveClip("Distortion fixture",fixture,{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);
    for(int kind:{9,10})
    {
        const auto before=host.undoHistory.size();host.addInsert(0,kind);auto* plugin=dynamic_cast<orbit::mixing::DistortionPluginBase*>(host.getInsertPlugins(0)[0]);distortionCheck(plugin!=nullptr && host.undoHistory.size()==before+1,"distortion insertion/Undo failed");
        plugin->parameters[4]->setParameter(0,juce::sendNotification);plugin->parameters[5]->setParameter(0,juce::sendNotification);
        for(double rate:{44100.,48000.,96000.})
        {
            distortionRate=rate;plugin->setEnabled(false);auto reference=distortionRender(host,"dry-"+juce::String(kind)+"-"+juce::String(rate));plugin->setEnabled(true);auto dry=distortionRender(host,"processed-dry-"+juce::String(kind)+"-"+juce::String(rate));
            double error=0;distortionCheck(dry.getNumSamples()==reference.getNumSamples(),"effect changed render duration");for(int i=0;i<dry.getNumSamples();++i)for(int ch=0;ch<2;++ch)error=std::max(error,double(std::abs(dry.getSample(ch,i)-reference.getSample(ch,i))));std::cout<<"Distortion dry PDC kind="<<kind<<" rate="<<rate<<" latency="<<plugin->getLatencySeconds()*rate<<" error="<<error<<"\n";distortionCheck(error<.00001,"oversampling dry path misaligned with native PDC");
        }
        distortionRate=48000;plugin->parameters[kind==9?0:2]->setParameter(24,juce::sendNotification);
        plugin->parameters[5]->setParameter(0,juce::sendNotification);auto dry=distortionRender(host,"blend-dry-"+juce::String(kind));plugin->parameters[5]->setParameter(100,juce::sendNotification);auto wet=distortionRender(host,"blend-wet-"+juce::String(kind));plugin->parameters[5]->setParameter(50,juce::sendNotification);auto half=distortionRender(host,"blend-half-"+juce::String(kind));
        double error=0;for(int i=0;i<half.getNumSamples();++i)for(int ch=0;ch<2;++ch){const auto v=wet.getSample(ch,i);distortionCheck(std::isfinite(v),"wet render nonfinite");error=std::max(error,double(std::abs(half.getSample(ch,i)-(v+dry.getSample(ch,i))*.5f)));}distortionCheck(error<.00001,"parallel blend failed aligned dry/wet sum");
        // The odd-symmetric modes must preserve opposite stereo polarities.
        for(int i=1000;i<wet.getNumSamples()-1000;++i)distortionCheck(std::abs(wet.getSample(0,i)+wet.getSample(1,i))<.00001,"stereo channel state leaked or image changed");
        if(kind==9)
        {
            orbit::mixing::DistortionCore direct(orbit::mixing::DistortionCore::Mode::tube);direct.prepare(48000);direct.setControls({24,0,800,30,12000,0});juce::AudioBuffer<float> unfiltered(2,48000);for(int i=0;i<48000;++i){auto v=direct.process(float(.15*std::sin(i*9000*juce::MathConstants<double>::twoPi/48000)),0);unfiltered.setSample(0,i,v[0]);unfiltered.setSample(1,i,0);}
            const auto alias=distortionTone(wet,21000),raw=distortionTone(unfiltered,21000);std::cout<<"Tube 21k alias: oversampled="<<alias<<" native="<<raw<<"\n";distortionCheck(alias<raw*.1,"oversampling did not substantially suppress folded third harmonic");
        }
        host.openInsertEditor(0,0);distortionCheck(host.mixerWindow->effectWindow->isEditingPlugin(plugin->itemID.toString()),"distortion parameter editor binding failed");
        auto target=host.targetForParameter(*plugin->parameters[4]);orbit::automation::Points points;points.add({0,0});points.add({16,1});host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,points,target.pluginId);host.updateEngineAutomation();distortionCheck(plugin->parameters[4]->hasAutomationPoints(),"distortion native automation not projected");plugin->parameters[4]->updateStream();auto automated=distortionRender(host,"automated-level-"+juce::String(kind));distortionCheck(distortionTone(automated,9000,.7,.8)>distortionTone(automated,9000,.2,.3)*2,"distortion output automation did not affect real rendered audio");
        host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,{},target.pluginId);host.updateEngineAutomation();
        host.removeInsert(0,0);host.undoProjectAction();distortionCheck(dynamic_cast<orbit::mixing::DistortionPluginBase*>(host.getInsertPlugins(0)[0])!=nullptr,"distortion removal Undo failed");host.removeInsert(0,0);
    }
    host.addInsert(0,9);host.addInsert(0,10);auto* tube=dynamic_cast<orbit::mixing::DistortionPluginBase*>(host.getInsertPlugins(0)[0]);tube->parameters[0]->setParameter(24,juce::sendNotification);host.copyInsertEffects(0);distortionCheck(host.pasteInsertEffects(1,false),"distortion chain copy failed");distortionCheck(host.getInsertPlugins(0)[0]->itemID!=host.getInsertPlugins(1)[0]->itemID,"copied distortion shared native ID");
    host.saveToFile(file);distortionCheck(host.loadProjectFile(file),"distortion same-process reopen failed");distortionCheck(host.getInsertPlugins(0).size()==2,"saved distortion chain lost");
    std::cout<<"PASS: native Tube/Two-Band rendering at three rates, first-sample dry PDC, aligned parallel blend, stereo isolation, alias rejection, parameter GUI/automation binding, Undo, chain copy and reopen. Listening remains manual.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
