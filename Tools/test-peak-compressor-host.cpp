static void peakCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static double peakRenderRate=48000;
static juce::AudioBuffer<float> peakRender(ProjectView& host,const juce::String& name)
{
    juce::WavAudioFormat wav;te::Renderer::Parameters p(*host.edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&wav;
    p.sampleRateForAudio=peakRenderRate;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;
    p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
    te::Renderer::RenderTask task("Peak compressor validation",p,nullptr,nullptr);int attempts=0;
    while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++attempts<100000){}
    peakCheck(attempts<100000 && task.errorMessage.isEmpty(),"peak compressor native render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(p.destFile),true));peakCheck(reader!=nullptr,"peak render unreadable");
    juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));
    reader->read(&audio,0,audio.getNumSamples(),0,true,true);return audio;
}
static double peakTone(const juce::AudioBuffer<float>& audio,double start,double end,double frequency)
{
    const int first=int(start*peakRenderRate),last=int(end*peakRenderRate);double real=0,imaginary=0;
    for(int i=first;i<last;++i){const auto angle=i*frequency*juce::MathConstants<double>::twoPi/peakRenderRate;real+=audio.getSample(0,i)*std::cos(angle);imaginary+=audio.getSample(0,i)*std::sin(angle);}
    return 2*std::sqrt(real*real+imaginary*imaginary)/(last-first);
}
static orbit::mixing::PeakCompressorPlugin* peakPlugin(ProjectView& host)
{return dynamic_cast<orbit::mixing::PeakCompressorPlugin*>(host.getInsertPlugins(0)[0]);}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1220,850);
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/peak-compressor.orbit");
    if(argc>1)
    {
        peakCheck(host.loadProjectFile(file),"fresh peak compressor load failed");auto* plugin=peakPlugin(host);
        peakCheck(plugin!=nullptr && plugin->getSidechainSourceID()==host.getGeneratorTracks()[1]->itemID,"fresh compressor/source identity lost");
        peakCheck(std::abs(plugin->parameters[1]->getCurrentValue()+30)<.001 && std::abs(plugin->parameters[6]->getCurrentValue()-30)<.001,"fresh compressor controls lost");
        const auto audio=peakRender(host,"fresh-peak");peakCheck(peakTone(audio,.6,.7,1000)<peakTone(audio,.35,.45,1000)*.3,"fresh peak sidechain did not duck/recover");
        std::cout<<"PASS: fresh-process original peak compressor parameters, routing and native duck/recovery render.\n";return 0;
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
    for(int n=0;n<2;++n)
    {
        const auto fixture=juce::File("OPTIONS_CHECK_FOLDER/peak-fixture-"+juce::String(n)+".wav");juce::WavAudioFormat wav;
        {auto stream=std::make_unique<juce::FileOutputStream>(fixture);std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));juce::AudioBuffer<float> audio(2,48000);
        for(int i=0;i<48000;++i){const double time=i/48000.;const float tone=float(.05*std::sin(time*1000*juce::MathConstants<double>::twoPi));const float trigger=std::fmod(time,.5)<.25?float(.6*std::sin(time*100*juce::MathConstants<double>::twoPi)):0;
        audio.setSample(0,i,n==0?tone:0);audio.setSample(1,i,n==0?-tone:trigger);}writer->writeFromAudioSampleBuffer(audio,0,48000);}
        auto* track=host.getGeneratorTracks()[n];for(auto* plugin:track->pluginList)if(plugin->isSynth())plugin->setEnabled(false);
        host.setTrackGainDb(n,0);host.setTrackEqEnabled(n,false);track->insertWaveClip("Peak fixture",fixture,{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);
    }
    host.setPlaybackMode(ProjectView::PlaybackMode::song);host.setMasterGainDb(0);host.setTrackEqEnabled(host.getMixerTracks().size(),false);
    const auto dry=peakRender(host,"peak-dry");peakCheck(peakTone(dry,.6,.7,1000)>.01,"peak fixture is silent");
    const auto undo=host.undoHistory.size();host.addInsert(0,8);auto* plugin=peakPlugin(host);peakCheck(plugin!=nullptr && host.undoHistory.size()==undo+1,"peak insertion/Undo missing");
    plugin->parameters[2]->setParameter(1,juce::sendNotification);
    for(double rate:{44100.,48000.,96000.})
    {
        peakRenderRate=rate;plugin->setEnabled(false);const auto reference=peakRender(host,"peak-reference-"+juce::String(rate));plugin->setEnabled(true);
        for(float blend:{0.f,50.f,100.f})
        {
            plugin->parameters[7]->setParameter(blend,juce::sendNotification);const auto audio=peakRender(host,"peak-transparent-"+juce::String(rate)+"-"+juce::String(blend));
            peakCheck(audio.getNumSamples()==reference.getNumSamples(),"compressor changed export length");double error=0;
            for(int i=0;i<audio.getNumSamples();++i)error=juce::jmax(error,double(std::abs(audio.getSample(0,i)-reference.getSample(0,i))));
            std::cout<<"Peak native PDC rate="<<rate<<" mix="<<blend<<" error="<<error<<"\n";peakCheck(error<.00001,"peak compressor parallel mix/PDC altered transparent audio");
        }
    }
    peakRenderRate=48000;
    host.undoProjectAction();peakCheck(host.getInsertPlugins(0).isEmpty(),"peak insert Undo failed");host.redoProjectAction();plugin=peakPlugin(host);
    plugin->parameters[1]->setParameter(-30,juce::sendNotification);plugin->parameters[2]->setParameter(4,juce::sendNotification);plugin->parameters[5]->setParameter(.1f,juce::sendNotification);plugin->parameters[6]->setParameter(30,juce::sendNotification);
    const auto self=peakRender(host,"peak-self");const auto original=peakTone(self,.6,.7,1000);const auto pluginId=plugin->itemID.toString(),source=host.getGeneratorTracks()[1]->itemID.toString();
    const auto sideUndo=host.undoHistory.size();peakCheck(host.setEffectSidechain(pluginId,source) && host.undoHistory.size()==sideUndo+1,"peak sidechain/Undo selection failed");
    const auto external=peakRender(host,"peak-side");const auto active=peakTone(external,.6,.7,1000),rest=peakTone(external,.35,.45,1000);
    std::cout<<"Peak sidechain tone: self="<<original<<" external="<<active<<" recovered="<<rest<<"\n";
    peakCheck(active<original*.3 && rest>original*1.2,"right-only external trigger failed to duck/recover anti-phase stereo");
    peakCheck(peakTone(external,.6,.7,100)<.00001,"sidechain trigger leaked into destination left audio");
    host.undoProjectAction();peakCheck(!peakPlugin(host)->getSidechainSourceID().isValid(),"peak sidechain Undo failed");host.redoProjectAction();plugin=peakPlugin(host);
    peakCheck(!host.setEffectSidechain(pluginId,host.getGeneratorTracks()[0]->itemID.toString()),"peak self-feedback accepted");
    host.openInsertEditor(0,0);auto* window=host.mixerWindow->effectWindow.get();peakCheck(window!=nullptr && window->dynamicsMeter.isVisible() && window->sidechainButton.isVisible(),"peak meters/sidechain UI absent");
    window->refreshLimiterMeters();peakCheck(std::isfinite(window->dynamicsMeter.outputDb),"peak meter value is not finite");
    peakCheck(window->parameters.rows.size()==8 && window->parameters.rows[1]->value.getSliderStyle()==juce::Slider::RotaryVerticalDrag,"dedicated compressor knob panel absent");
    window->parameters.rows[2]->value.setValue(100,juce::sendNotificationSync);
    peakCheck(window->parameters.rows[2]->value.getTextFromValue(100).contains("INF"),"rotary panel lost infinite-ratio readout");
    window->parameters.rows[2]->value.setValue(4,juce::sendNotificationSync);
    plugin->parameters[2]->setParameter(100,juce::sendNotification);peakCheck(plugin->parameters[2]->getCurrentValueAsStringWithLabel().contains("INF"),"infinite ratio is not labelled");plugin->parameters[2]->setParameter(4,juce::sendNotification);
    {if(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()==nullptr)window->removeFromDesktop();window->setSize(720,480);auto image=window->createComponentSnapshot(window->getLocalBounds());juce::FileOutputStream output(juce::File("OPTIONS_CHECK_FOLDER/peak-compressor.png"));juce::PNGImageFormat().writeImageToStream(image,output);}
    {PluginParameterWindow::ParameterRow ratioRow(plugin->parameters[2],[]{});peakCheck(std::abs(ratioRow.value.proportionOfLengthToValue(.5)-4)<.001,"host ratio slider ignored its musical range mapping");}
    const auto gestureUndo=host.undoHistory.size();
    auto* thresholdRow=window->parameters.rows[1].get();thresholdRow->value.onDragStart();thresholdRow->value.setValue(-24,juce::sendNotificationSync);thresholdRow->value.setValue(-20,juce::sendNotificationSync);thresholdRow->value.onDragEnd();
    peakCheck(host.undoHistory.size()==gestureUndo+1,"effect slider gesture did not group one Undo");
    host.undoProjectAction();peakCheck(std::abs(peakPlugin(host)->parameters[1]->getCurrentValue()+30)<.001,"effect parameter Undo lost its initial value");
    host.redoProjectAction();plugin=peakPlugin(host);peakCheck(std::abs(plugin->parameters[1]->getCurrentValue()+20)<.001,"effect parameter Redo lost final value");plugin->parameters[1]->setParameter(-30,juce::sendNotification);
    const auto target=host.targetForParameter(*plugin->parameters[3]);orbit::automation::Points curve;curve.add({0,plugin->parameters[3]->valueRange.convertTo0to1(-12)});curve.add({16,plugin->parameters[3]->valueRange.convertTo0to1(0)});
    host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,curve,target.pluginId);host.updateEngineAutomation();
    // Publish the SDK iterator explicitly: this harness renders synchronously
    // on the message thread instead of running the application event loop.
    peakCheck(plugin->parameters[3]->hasAutomationPoints(),"native compressor automation curve absent");
    plugin->parameters[3]->updateStream();
    peakCheck(plugin->parameters[3]->hasAutomationPoints() && plugin->isAutomationNeeded(),"native compressor automation stream was not prepared");
    const auto automated=peakRender(host,"peak-automated");
    std::cout<<"Peak automation early="<<peakTone(automated,.35,.45,1000)<<" late="<<peakTone(automated,.85,.95,1000)<<"\n";peakCheck(peakTone(automated,.85,.95,1000)>peakTone(automated,.35,.45,1000)*1.2,"peak output Song automation did not reach the native processor");
    host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,{},target.pluginId);host.updateEngineAutomation();juce::Thread::sleep(30);juce::Timer::callPendingTimersSynchronously();plugin=peakPlugin(host);plugin->parameters[3]->setParameter(0,juce::sendNotification);
    host.saveToFile(file);peakCheck(host.loadProjectFile(file),"peak native reopen failed");plugin=peakPlugin(host);peakCheck(plugin!=nullptr && plugin->getSidechainSourceID().toString()==source,"peak native state/source failed after reopen");
    host.copyInsertEffects(0);peakCheck(host.pasteInsertEffects(1,false),"peak chain copy failed");auto* copied=dynamic_cast<orbit::mixing::PeakCompressorPlugin*>(host.getInsertPlugins(1)[0]);
    peakCheck(copied!=nullptr && copied->itemID.toString()!=pluginId && !copied->getSidechainSourceID().isValid() && copied->parameters[1]->getCurrentValue()==-30,"peak copied state/IDs/self-sidechain refusal failed");
    std::cout<<"PASS: original peak compressor native multi-rate PDC/blend, external trigger isolation, duck/recovery, meters, automation, Undo/Redo, copy and native reopen. Hardware/listening disabled.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
