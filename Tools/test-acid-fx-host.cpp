// Production Acid Bass FX, native persistence, UI and transport regression check.
#ifndef ORBIT_ACID_CHECK_FOLDER
#define ORBIT_ACID_CHECK_FOLDER "OPTIONS_CHECK_FOLDER"
#endif
static void acidFxCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::vector<float> fxSignal(double rate,orbit::synth::AcidBassFx::Controls c,int block)
{
    orbit::synth::AcidBassFx fx;fx.prepare(rate,block);fx.setControls(c);std::vector<float> samples(static_cast<size_t>(rate));
    for(size_t n=0;n<samples.size();++n)samples[n]=float(.25*std::sin(n*997*juce::MathConstants<double>::twoPi/rate));
    for(size_t n=0;n<samples.size();n+=size_t(block))fx.process(samples.data()+n,int(std::min(size_t(block),samples.size()-n)));
    return samples;
}
static juce::AudioBuffer<float> acidFxRender(ProjectView& host,const juce::String& name)
{
    juce::WavAudioFormat format;te::Renderer::Parameters p(*host.edit);p.destFile=juce::File(juce::String(ORBIT_ACID_CHECK_FOLDER)+"/"+name+".wav");p.audioFormat=&format;p.sampleRateForAudio=48000;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;
    p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
    te::Renderer::RenderTask task("Acid FX validation",p,nullptr,nullptr);int n=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++n<100000){}
    acidFxCheck(n<100000 && task.errorMessage.isEmpty(),"native FX render failed");
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(p.destFile),true));acidFxCheck(reader!=nullptr,"render unreadable");
    juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));reader->read(&audio,0,audio.getNumSamples(),0,true,true);return audio;
}
int main(int argc,char**)
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,900);const juce::File file(ORBIT_ACID_CHECK_FOLDER "/acid-fx.orbit");
    if(argc>1)
    {
        acidFxCheck(host.loadProjectFile(file),"fresh Acid FX project reopen failed");auto* p=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(host.project.getTrackId(0)));host.songModeButton.onClick();host.updateEngineAutomation();
        acidFxCheck(p!=nullptr && p->parameters.size()==17 && p->parameters[9]->getCurrentValue()==81 && p->parameters[10]->getCurrentValue()==67 && p->parameters[14]->getCurrentValue()==1 && p->parameters[16]->getCurrentValue()==1 && p->parameters[14]->hasAutomationPoints(),"fresh FX/triangle/legacy automation state lost");
        auto audio=acidFxRender(host,"fresh-fx");acidFxCheck(audio.getMagnitude(0,audio.getNumSamples())>.001,"fresh FX silent");std::cout<<"PASS: fresh-process native FX parameters and actual audio render.\n";return 0;
    }
    // Unchanged default dry samples, fixed latency, block segmentation and aligned blend.
    for(double rate:{44100.,48000.,96000.})
    {
        orbit::synth::AcidBassFx fx;fx.prepare(rate,257);const int latency=fx.latencySamples();acidFxCheck(latency>0 && latency<128,"invalid FX latency");
        auto dry=fxSignal(rate,{},257),wet=fxSignal(rate,{80,100,30,18000,0,0},257),half=fxSignal(rate,{80,50,30,18000,0,0},257);
        auto folded=fxSignal(rate,{80,100,30,18000,0,1},257),segmented=fxSignal(rate,{80,100,30,18000,0,1},1);double difference=0;
        for(size_t n=0;n<dry.size();++n)
        {
            const float expected=n<size_t(latency)?0:float(.25*std::sin((n-size_t(latency))*997*juce::MathConstants<double>::twoPi/rate));
            acidFxCheck(dry[n]==expected,"default FX changed the dry signal or timing");
            acidFxCheck(std::abs(half[n]-(dry[n]+wet[n])*.5)<2.e-6,"parallel blend is not phase aligned");
            acidFxCheck(std::isfinite(folded[n]) && std::abs(folded[n])<2 && std::abs(folded[n]-segmented[n])<2.e-6,"folding invalid or block-size dependent");difference+=std::abs(folded[n]-wet[n]);
        }
        acidFxCheck(difference>50,"Soft and Sine Fold do not differ");
        auto triangle=fxSignal(rate,{80,100,30,18000,0,0,1},257),triangleSegments=fxSignal(rate,{80,100,30,18000,0,0,1},79),triangleHalf=fxSignal(rate,{80,50,30,18000,0,0,1},257);
        double triangleDifference=0,triangleSineDifference=0;
        for(size_t n=0;n<triangle.size();++n)
        {
            acidFxCheck(std::isfinite(triangle[n]) && std::abs(triangle[n])<2 && std::abs(triangle[n]-triangleSegments[n])<2.e-6,"triangle fold invalid or block dependent");
            acidFxCheck(std::abs(triangleHalf[n]-(dry[n]+triangle[n])*.5)<2.e-6,"triangle dry/wet alignment changed");
            triangleDifference+=std::abs(triangle[n]-wet[n]);triangleSineDifference+=std::abs(triangle[n]-folded[n]);
        }
        acidFxCheck(triangleDifference>50 && triangleSineDifference>50,"Triangle Foldback does not differ from both existing shapes");
        fx.reset();fx.setControls({100,100,1500,20000,12,1,1});std::vector<float> triangleExtreme(6000,32);fx.process(triangleExtreme.data(),int(triangleExtreme.size()));for(float x:triangleExtreme)acidFxCheck(std::isfinite(x) && std::abs(x)<10,"triangle extreme unstable");
        fx.setControls({0,100,20,20000,0,0,0});fx.process(triangleExtreme.data(),int(triangleExtreme.size()));for(float x:triangleExtreme)acidFxCheck(std::isfinite(x) && std::abs(x)<10,"mode transition unstable");

        fx.setControls({NAN,INFINITY,NAN,-INFINITY,INFINITY,NAN});std::vector<float> bad(2001,NAN);fx.process(bad.data(),int(bad.size()));for(auto x:bad)acidFxCheck(std::isfinite(x),"non-finite parameter/input leaked");
        for(float shape:{0.f,1.f}){fx.reset();fx.setControls({100,100,1500,20000,12,shape});std::fill(bad.begin(),bad.end(),32);fx.process(bad.data(),int(bad.size()));for(auto x:bad)acidFxCheck(std::isfinite(x) && std::abs(x)<10,"extreme FX unstable");}
    }
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();host.createAcidDemo();host.closePianoRollView();host.openGeneratorControls(id,false);
    auto* plugin=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(id));acidFxCheck(plugin!=nullptr,"Acid native plugin missing");
    for(int i=0;i<9;++i)acidFxCheck(plugin->parameters[size_t(i)]->getCurrentValue()==orbit::synth::AcidBassPlugin::defaults[size_t(i)],"legacy parameter index/default changed");
    auto legacy=plugin->state.createCopy();for(size_t i=9;i<plugin->parameters.size();++i)legacy.removeProperty(orbit::synth::AcidBassPlugin::ids[i],nullptr);
    plugin->parameters[10]->setParameter(100,juce::sendNotificationSync);plugin->restorePluginStateFromValueTree(legacy);acidFxCheck(plugin->parameters[10]->getCurrentValue()==0,"legacy preset is not dry");acidFxCheck(plugin->parameters[16]->getCurrentValue()==0,"legacy preset enabled Triangle Foldback");
    // Raw native synth must preserve the original voice, including MIDI offset,
    // with exactly the newly declared constant delay and no wet coloration.
    {
        te::PluginInitialisationInfo info{};info.sampleRate=48000;info.blockSizeSamples=512;plugin->initialise(info);
        orbit::synth::AcidBassVoice reference;reference.prepare(48000);std::array<float,512> expected{};
        for(int n=0;n<512;++n){if(n==48){reference.controller(119,3);reference.noteOn(1,36,100);reference.finishEvents();}expected[size_t(n)]=reference.sample();}
        juce::AudioBuffer<float> audio(2,512);te::MidiMessageArray midi;
        midi.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,3),48./48000,{});midi.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),48./48000,{});
        te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),0,512,&midi,0,
            {tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(512./48000)},true,false,false,false);
        plugin->applyToBuffer(context);const int delay=juce::roundToInt(plugin->getLatencySeconds()*48000);
        for(int n=0;n<512;++n)acidFxCheck(audio.getSample(0,n)==(n<delay?0:expected[size_t(n-delay)]) && audio.getSample(0,n)==audio.getSample(1,n),"native dry voice/MIDI timing/stereo changed");
        midi.isAllNotesOff=true;plugin->applyToBuffer(context);acidFxCheck(audio.getMagnitude(0,512)==0,"panic left old FX/dry audio");plugin->deinitialise();
    }
    auto& w=*host.generatorControlWindows.at(id).window;w.setBounds(40,100,900,300);const auto original=w.getBounds();
    auto* editor=dynamic_cast<orbit::synth::AcidBassEditor*>(w.instrument.get());acidFxCheck(editor!=nullptr,"Acid editor missing");
    for(auto& knob:editor->knobs)acidFxCheck(knob.getTextBoxPosition()==juce::Slider::NoTextBox,"permanent value box remains");
    acidFxCheck(!editor->knobs[8].isVisible(),"FX not collapsed initially");editor->setEffectsExpanded(true);
    acidFxCheck(w.getHeight()>=404 && w.getPosition()==original.getPosition() && editor->knobs[12].isVisible() && editor->getLocalBounds().contains(editor->knobs[12].getBounds()),"FX expansion clipped/detached or moved window");
    auto undo=host.undoHistory.size();editor->knobs[8].onDragStart();editor->knobs[8].setValue(60,juce::sendNotificationSync);editor->knobs[8].setValue(81,juce::sendNotificationSync);editor->knobs[8].onDragEnd();
    acidFxCheck(plugin->parameters[9]->getCurrentValue()==81 && host.undoHistory.size()==undo+1,"Drive gesture did not bind or group Undo");
    int event=-1;editor->onEditEvents=[&](int p,juce::Component*){event=p;};editor->knobs[9].onEvents(&editor->knobs[9]);acidFxCheck(event==10,"FX event menu targets wrong native parameter");
    editor->setEffectsExpanded(false);acidFxCheck(w.getBounds()==original,"collapse lost previous bounds");editor->setEffectsExpanded(true);
    undo=host.undoHistory.size();editor->mode.setSelectedId(3,juce::sendNotificationSync);acidFxCheck(plugin->parameters[16]->getCurrentValue()==1 && plugin->parameters[14]->getCurrentValue()==0 && host.undoHistory.size()==undo+1,"Triangle selector changed old mode or lost Undo");
    undo=host.undoHistory.size();editor->mode.setSelectedId(2,juce::sendNotificationSync);acidFxCheck(plugin->parameters[16]->getCurrentValue()==0 && plugin->parameters[14]->getCurrentValue()==1 && host.undoHistory.size()==undo+1,"Sine selector did not restore old mode");
    acidFxCheck(plugin->parameters[14]->getValueRange()==juce::Range<float>(0,1),"legacy FX Mode automation range changed");
    host.addDrumRackTrack();const auto drum=host.selectedProjectTrackId();host.showPlaylist();host.showPianoRoll();host.showSongEventEditor();host.showMixer(false);host.addInsert(0,8);host.addInsert(1,9);
    auto effects=host.mixerWindow->openEffectWindows();effects.rbegin()->second->minimiseButtonPressed();
    auto stable=[&]
    {
        juce::String result;
        for(const auto& [key,e]:host.generatorControlWindows)result+=key+":"+e.window->getBounds().toString()+":"+juce::String(int(e.window->isVisible()))+":"+juce::String(int(e.window->isMinimised()))+";";
        for(const auto& [key,e]:host.mixerWindow->openEffectWindows())result+=key+":"+e->getBounds().toString()+":"+juce::String(int(e->isVisible()))+":"+juce::String(int(e->isMinimised()))+";";
        for(juce::Component* c:{static_cast<juce::Component*>(host.playlistWindow.get()),static_cast<juce::Component*>(host.pianoRollWindow.get()),static_cast<juce::Component*>(host.songEventWindow.get()),static_cast<juce::Component*>(host.mixerWindow.get())})result+=c->getBounds().toString()+":"+juce::String(int(c->isVisible()))+";";
        return result;
    };
    // Hosted buffers run real production transport paths without audio hardware.
    auto& device=host.engine.getDeviceManager().getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;params.sampleRate=48000;params.inputChannels=0;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);
    host.selectGeneratorControlTrack(id);host.project.setPlaylistClip(id,host.project.getPatternId(id,0),0,0,true);host.refreshEnginePlaylist();const auto layout=stable();
    auto click=[&](juce::Button& button){button.onClick();acidFxCheck(stable()==layout,"transport changed window visibility/minimize/bounds");};
    click(host.patternModeButton);click(host.playButton);acidFxCheck(host.edit->getTransport().isPlaying(),"Pattern Play never reached playing branch");click(host.playButton);acidFxCheck(!host.edit->getTransport().isPlaying(),"Play toggle did not stop");
    click(host.playButton);click(host.stopButton);click(host.rewindButton);click(host.previousButton);click(host.nextButton);
    click(host.songModeButton);click(host.playButton);acidFxCheck(host.edit->getTransport().isPlaying(),"Song Play never reached playing branch");click(host.stopButton);
    host.loopButton.setToggleState(true,juce::dontSendNotification);click(host.loopButton);host.loopButton.setToggleState(false,juce::dontSendNotification);click(host.loopButton);
    host.saveOption("recordContent",1);click(host.recordButton);acidFxCheck(host.automationTakeActive,"Automation Record did not arm");click(host.stopButton);
    host.saveOption("recordContent",0);click(host.recordButton);acidFxCheck(host.midiTake.active,"MIDI Record did not arm");click(host.stopButton);
    std::cout<<"PASS: real Play/Stop/play-toggle, Pattern/Song, Return, previous/next, Loop, MIDI/automation Record preserve open and deliberately minimized windows. Hosted buffers, no hardware.\n";
    // Isolate synth for actual engine render, PDC and automation checks.
    host.engine.getDeviceManager().deviceManager.closeAudioDevice();host.removeInsert(0,0);host.removeInsert(1,0);host.selectGeneratorControlTrack(id);
    host.setTrackEqEnabled(0,false);host.setMasterGainDb(0);host.setTrackEqEnabled(host.getMixerTracks().size(),false);
    auto dryRender=acidFxRender(host,"native-dry");acidFxCheck(dryRender.getMagnitude(0,32)>1.e-5,"native renderer did not compensate the synth FX latency");plugin->parameters[9]->setParameter(81,juce::sendNotificationSync);plugin->parameters[10]->setParameter(67,juce::sendNotificationSync);plugin->parameters[14]->setParameter(1,juce::sendNotificationSync);
    auto wetRender=acidFxRender(host,"native-wet");double changed=0;for(int i=0;i<dryRender.getNumSamples();++i)changed+=std::abs(dryRender.getSample(0,i)-wetRender.getSample(0,i));acidFxCheck(changed>1,"native FX did not change actual audio");
    plugin->parameters[16]->setParameter(1,juce::sendNotificationSync);auto triangleRender=acidFxRender(host,"native-triangle");double triangleChange=0;for(int i=0;i<wetRender.getNumSamples();++i)triangleChange+=std::abs(wetRender.getSample(0,i)-triangleRender.getSample(0,i));acidFxCheck(triangleChange>1,"native Triangle audio matches Sine Fold");
    auto triangleTarget=host.targetForParameter(*plugin->parameters[16]);orbit::automation::Points trianglePoints;trianglePoints.add({0,0});trianglePoints.add({16,1});host.project.replaceSongAutomationPoints(triangleTarget.trackId,triangleTarget.pluginName,triangleTarget.parameterId,triangleTarget.parameterName,trianglePoints,triangleTarget.pluginId);host.updateEngineAutomation();acidFxCheck(plugin->parameters[16]->hasAutomationPoints(),"Triangle automation missing");plugin->parameters[16]->updateStream();
    auto triangleAutomated=acidFxRender(host,"triangle-automated");double triangleAutomationChange=0;for(int i=0;i<triangleRender.getNumSamples();++i)triangleAutomationChange+=std::abs(triangleRender.getSample(0,i)-triangleAutomated.getSample(0,i));acidFxCheck(triangleAutomationChange>1,"Triangle automation did not change actual audio");
    host.project.replaceSongAutomationPoints(triangleTarget.trackId,triangleTarget.pluginName,triangleTarget.parameterId,triangleTarget.parameterName,{},triangleTarget.pluginId);host.updateEngineAutomation();plugin->parameters[16]->setParameter(0,juce::sendNotificationSync);
    auto target=host.targetForParameter(*plugin->parameters[13]);orbit::automation::Points points;points.add({0,0});points.add({16,1});host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,points,target.pluginId);host.updateEngineAutomation();acidFxCheck(plugin->parameters[13]->hasAutomationPoints(),"FX automation not projected");plugin->parameters[13]->updateStream();
    auto automated=acidFxRender(host,"native-automation");double automationDifference=0;for(int i=0;i<wetRender.getNumSamples();++i)automationDifference+=std::abs(wetRender.getSample(0,i)-automated.getSample(0,i));acidFxCheck(automationDifference>1,"FX automation did not affect rendered audio");
    host.project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,{},target.pluginId);host.updateEngineAutomation();plugin->parameters[13]->setParameter(0,juce::sendNotificationSync);
    plugin->parameters[16]->setParameter(1,juce::sendNotificationSync);auto oldModeTarget=host.targetForParameter(*plugin->parameters[14]);orbit::automation::Points oldModePoints;oldModePoints.add({0,1});oldModePoints.add({16,1});host.project.replaceSongAutomationPoints(oldModeTarget.trackId,oldModeTarget.pluginName,oldModeTarget.parameterId,oldModeTarget.parameterName,oldModePoints,oldModeTarget.pluginId);host.updateEngineAutomation();
    host.saveToFile(file);acidFxCheck(host.loadProjectFile(file),"same-process FX reopen failed");
    host.openGeneratorControls(id,false);auto& restored=*host.generatorControlWindows.at(id).window;auto* panel=dynamic_cast<orbit::synth::AcidBassEditor*>(restored.instrument.get());panel->setEffectsExpanded(true);acidFxCheck(panel->mode.getSelectedId()==3,"reopened Triangle selector lost selection");restored.setBounds(20,90,900,404);restored.toFront(false);
    {auto image=restored.createComponentSnapshot(restored.getLocalBounds());juce::FileOutputStream out(juce::File(ORBIT_ACID_CHECK_FOLDER "/acid-fx-expanded.png"));juce::PNGImageFormat().writeImageToStream(image,out);}
    std::cout<<"PASS: multirate dry identity/latency/blend/block invariance, distinct stable modes, missing-state dry defaults, compact UI expansion, grouped parameter gestures, Event target, native DSP/automation including Triangle Foldback, legacy mode-curve range and save/reopen.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
