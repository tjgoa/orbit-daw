static void limitCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::AudioBuffer<float> limitRender(ProjectView& host,const juce::String& name)
{
 juce::WavAudioFormat wav;te::Renderer::Parameters p(*host.edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&wav;p.sampleRateForAudio=48000;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;
 p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
 te::Renderer::RenderTask task("Limiter validation",p,nullptr,nullptr);int attempts=0;
 while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++attempts<100000){}
 limitCheck(attempts<100000 && task.errorMessage.isEmpty(),"limiter native render failed");
 std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(p.destFile),true));limitCheck(reader!=nullptr,"limiter render unreadable");
 juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));reader->read(&audio,0,audio.getNumSamples(),0,true,true);return audio;
}
int main(int argc,char**)
{
 try{
 juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1220,850);
 const auto saved=juce::File("OPTIONS_CHECK_FOLDER/limiter.orbit");
 if(argc>1){limitCheck(host.loadProjectFile(saved),"fresh limiter project failed");auto* limiter=dynamic_cast<orbit::mixing::LookaheadLimiterPlugin*>(host.getInsertPlugins(0)[0]);limitCheck(limiter!=nullptr && std::abs(limiter->parameters[0]->getCurrentValue()-36)<.001,"fresh limiter state failed");const auto audio=limitRender(host,"fresh-limiter");limitCheck(audio.getMagnitude(0,audio.getNumSamples())<=1.00001f,"fresh limiter ceiling failed");std::cout<<"PASS: fresh native limiter state/render\n";return 0;}
 host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
 const auto fixture=juce::File("OPTIONS_CHECK_FOLDER/limiter-pulse.wav");juce::WavAudioFormat wav;
 {auto stream=std::make_unique<juce::FileOutputStream>(fixture);std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));juce::AudioBuffer<float> audio(2,48000);audio.clear();audio.setSample(0,0,.125f);audio.setSample(1,0,.0625f);audio.setSample(0,9600,.25f);audio.setSample(1,9600,.125f);audio.setSample(0,47980,.125f);audio.setSample(1,47980,.0625f);writer->writeFromAudioSampleBuffer(audio,0,48000);}
 for(int n=0;n<2;++n){auto* track=host.getGeneratorTracks()[n];for(auto* plugin:track->pluginList)if(plugin->isSynth())plugin->setEnabled(false);host.setTrackGainDb(n,0);host.setTrackEqEnabled(n,false);track->insertWaveClip("Pulse",fixture,{{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)},{}},false);}
 host.setPlaybackMode(ProjectView::PlaybackMode::song);host.setMasterGainDb(0);host.setTrackEqEnabled(host.getMixerTracks().size(),false);
 const auto dry=limitRender(host,"limiter-dry");const auto magnitude=dry.getMagnitude(0,dry.getNumSamples());limitCheck(magnitude>.1,"limiter pulse fixture silent");
 const auto undo=host.undoHistory.size();host.addInsert(0,7);auto* limiter=dynamic_cast<orbit::mixing::LookaheadLimiterPlugin*>(host.getInsertPlugins(0)[0]);limitCheck(limiter!=nullptr && host.undoHistory.size()==undo+1,"limiter insertion missing/Undo split");
 const auto aligned=limitRender(host,"limiter-aligned");std::cout<<"Limiter native parallel pulse: dry="<<magnitude<<" wet="<<aligned.getMagnitude(0,aligned.getNumSamples())<<" latency="<<limiter->getLatencySeconds()<<"\n";
 limitCheck(std::abs(aligned.getMagnitude(0,aligned.getNumSamples())-magnitude)<.0001,"native limiter latency compensation misaligned parallel tracks");
 limitCheck(aligned.getNumSamples()==dry.getNumSamples(),"limiter changed export length");
 for(int i=0;i<dry.getNumSamples();++i)limitCheck(std::abs(dry.getSample(0,i)-aligned.getSample(0,i))<.00001,"limiter altered transparent timing or lost end-of-export samples");
 host.undoProjectAction();limitCheck(host.getInsertPlugins(0).isEmpty(),"limiter insert Undo failed");host.redoProjectAction();limiter=dynamic_cast<orbit::mixing::LookaheadLimiterPlugin*>(host.getInsertPlugins(0)[0]);
 host.setTrackGainDb(1,-100);limiter->parameters[0]->setParameter(36,juce::sendNotification);limiter->parameters[1]->setParameter(75,juce::sendNotification);
 const auto loud=limitRender(host,"limiter-loud");const auto max=loud.getMagnitude(0,loud.getNumSamples());limitCheck(max>.8 && max<=1.00001,"native limiter did not catch boosted pulse");
 for(int i=0;i<loud.getNumSamples();++i)limitCheck(std::isfinite(loud.getSample(0,i)),"limiter output nonfinite");
 limiter->setEnabled(false);const auto bypass=limitRender(host,"limiter-bypass");limitCheck(bypass.getMagnitude(0,bypass.getNumSamples())>.2 && bypass.getMagnitude(0,bypass.getNumSamples())<.3,"limiter bypass retained gain or delayed silence");limiter->setEnabled(true);
 host.openInsertEditor(0,0);host.mixerWindow->effectWindow->refreshSidechain();limitCheck(host.mixerWindow->effectWindow->limiterMeters.isVisible(),"limiter input/reduction meters absent");
 // A normal loop wrap must retain the look-ahead history; an actual seek flushes it.
 limiter->parameters[0]->setParameter(0,juce::sendNotification);limiter->initialise({tracktion::TimePosition::fromSeconds(0),48000,480});
 host.edit->getTransport().setLoopRange({tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(.01)});host.edit->getTransport().looping=true;
 juce::AudioBuffer<float> block(2,480);auto fill=[&]{for(int c=0;c<2;++c)for(int n=0;n<480;++n)block.setSample(c,n,.2f);};
 auto process=[&](double start){te::PluginRenderContext context(&block,juce::AudioChannelSet::stereo(),0,480,nullptr,0,{tracktion::TimePosition::fromSeconds(start),tracktion::TimePosition::fromSeconds(start+.01)},true,false,false,false);limiter->applyToBuffer(context);};
 fill();process(0);fill();process(0);limitCheck(block.getSample(0,0)>.19f,"limiter loop wrap cleared its delay history");fill();process(2);limitCheck(block.getSample(0,0)==0 && block.getSample(0,300)>.19f,"limiter seek retained old audio or failed to refill");
 host.edit->getTransport().looping=false;limiter->parameters[0]->setParameter(36,juce::sendNotification);
 host.saveToFile(saved);limitCheck(host.loadProjectFile(saved),"limiter native reopen failed");limiter=dynamic_cast<orbit::mixing::LookaheadLimiterPlugin*>(host.getInsertPlugins(0)[0]);limitCheck(limiter!=nullptr && limiter->parameters[0]->getCurrentValue()==36 && limiter->parameters[1]->getCurrentValue()==75,"limiter settings lost on reopen");
 host.copyInsertEffects(0);limitCheck(host.pasteInsertEffects(host.getMixerTracks().size(),false),"limiter chain copy failed");limitCheck(dynamic_cast<orbit::mixing::LookaheadLimiterPlugin*>(host.getInsertPlugins(host.getMixerTracks().size())[0])!=nullptr,"limiter copy lost type");
 std::cout<<"PASS: native limiter insertion/Undo, parallel-track latency compensation, boosted peak ceiling, finite render, meters, parameter reopen and chain copy. Listening remains manual.\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
