#define ORBIT_ACID_CHECK_FOLDER "OPTIONS_CHECK_FOLDER"
#define main acidLegacyRegressionMain
#include "test-acid-fx-host.cpp"
#undef main
static juce::Component* rackControl(juce::Component& root,const juce::String& id)
{if(root.getComponentID()==id)return &root;for(auto* child:root.getChildren())if(auto* found=rackControl(*child,id))return found;return nullptr;}
static std::vector<float> expandedFx(int block,orbit::synth::AcidBassFx::StageControls tube,orbit::synth::AcidBassFx::BandControls band)
{
    orbit::synth::AcidBassFx fx;fx.prepare(48000,block);fx.setControls({65,35,30,12000,0,1,0});fx.setExtraControls(tube,band);
    std::vector<float> samples(12000);for(size_t i=0;i<samples.size();++i)samples[i]=float(.3*std::sin(i*997*juce::MathConstants<double>::twoPi/48000));
    for(size_t i=0;i<samples.size();i+=size_t(block))fx.process(samples.data()+i,int(std::min(size_t(block),samples.size()-i)));return samples;
}
static double audioDifference(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b)
{double total=0;for(int i=0;i<juce::jmin(a.getNumSamples(),b.getNumSamples());++i)total+=std::abs(a.getSample(0,i)-b.getSample(0,i));return total;}
static juce::AudioBuffer<float> isolatedModulation(ProjectView& host,orbit::synth::AcidBassPlugin& source,int mode)
{
    source.flushPluginStateToValueTree();auto state=source.state.createCopy();te::AutomationCurve::removeAllAutomationCurvesRecursively(state);te::EditItemID::remapIDs(state,nullptr,*host.edit);
    auto owned=host.edit->getPluginCache().createNewPlugin(state);auto* synth=dynamic_cast<orbit::synth::AcidBassPlugin*>(owned.get());acidFxCheck(synth!=nullptr,"isolated modulation synth factory failed");
    auto set=[&](size_t index,float value){synth->parameters[index]->setParameter(value,juce::dontSendNotification);};set(15,1);set(35,mode==0?0.f:1.f);set(40,mode==0?0.f:1.f);
    if(mode==2){set(36,2);set(41,2);set(37,70);set(42,-70);set(38,0);set(43,0);set(39,1);set(44,1);}
    te::PluginInitialisationInfo info{};info.sampleRate=48000;info.blockSizeSamples=512;synth->initialise(info);
    juce::AudioBuffer<float> output(2,48000),block(2,512);te::MidiMessageArray midi;
    for(int first=0;first<48000;first+=512){const int count=juce::jmin(512,48000-first);if(first==0){midi.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,1),0,{});midi.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),0,{});}
        te::PluginRenderContext context(&block,juce::AudioChannelSet::stereo(),0,count,&midi,0,{tracktion::TimePosition::fromSeconds(first/48000.),tracktion::TimePosition::fromSeconds((first+count)/48000.)},true,false,false,true);synth->applyToBuffer(context);for(int ch=0;ch<2;++ch)output.copyFrom(ch,first,block,ch,0,count);}
    synth->deinitialise();return output;
}
#ifndef ORBIT_RACK_MAIN
#define ORBIT_RACK_MAIN main
#endif
int ORBIT_RACK_MAIN(int argc,char** argv){try{
    if(argc>1 && juce::String(argv[1])=="--legacy")return acidLegacyRegressionMain(1,nullptr);
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.setSize(1680,1200);
    auto acid=[&](int row){return dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(h.project.getTrackId(row)));};
    auto rack=[&](int row){for(auto* p:h.getInsertPlugins(row))if(auto* r=dynamic_cast<te::RackInstance*>(p);orbit::mixing::isMultiFx(r))return r;return static_cast<te::RackInstance*>(nullptr);};
    const juce::File file("OPTIONS_CHECK_FOLDER/acid-rack.orbit"),preset("OPTIONS_CHECK_FOLDER/expanded.orbitpreset");
    if(argc>1){acidFxCheck(h.loadProjectFile(file),"fresh expanded project failed");acidFxCheck(!h.edit->state.getParent().isValid(),"loaded engine state is still attached to project document");acidFxCheck(acid(0) && acid(0)->parameters[21]->getCurrentValue()==1 && acid(0)->parameters[35]->getCurrentValue()==1 && acid(0)->parameters[40]->getCurrentValue()==1 && acid(0)->parameters[44]->getCurrentValue()==13,"fresh synth FX/LFO lost");acidFxCheck(rack(0) && rack(1) && rack(0)->type!=rack(1)->type && rack(0)->type->getPlugins().size()==2,"fresh rack chain lost/linked");h.songModeButton.onClick();h.updateEngineAutomation();auto audio=acidFxRender(h,"expanded-fresh");        acidFxCheck(audio.getMagnitude(0,audio.getNumSamples())>1.e-6,"fresh expanded audio silent");std::cout<<"PASS: fresh synth FX/LFO presets, independent rack chains and native audio.\n";return 0;}
    auto a=expandedFx(257,{1,24,60,14000,1,-12,60},{1,900,10,22,17000,-6,75});auto b=expandedFx(1,{1,24,60,14000,1,-12,60},{1,900,10,22,17000,-6,75});auto off=expandedFx(257,{},{});double difference=0;
    for(size_t i=0;i<a.size();++i){acidFxCheck(std::isfinite(a[i]) && std::abs(a[i])<5 && std::abs(a[i]-b[i])<3.e-6,"new stages unstable/block dependent");difference+=std::abs(a[i]-off[i]);}acidFxCheck(difference>20,"new stages inaudible");
    for(float rate:{44100.f,48000.f,96000.f}){orbit::synth::AcidBassFx fx;fx.prepare(rate,65);fx.setControls({100,100,1500,20000,12,1,1});fx.setExtraControls({1,48,20,20000,2,12,100},{1,12000,48,48,24000,12,100});std::vector<float> signal(6000,32);fx.process(signal.data(),int(signal.size()));for(float x:signal)acidFxCheck(std::isfinite(x) && std::abs(x)<100,"extreme extra distortion unstable");}
    h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);const auto id=h.selectedProjectTrackId();auto* p=acid(0);acidFxCheck(p && p->parameters.size()==orbit::synth::AcidBassPlugin::parameterCount,"expanded Acid parameter layout missing");
    for(size_t index:{21,28,35,40})acidFxCheck(p->parameters[index]->getCurrentValue()==0,"new effect/modulator not Off");
    p->parameters[21]->setParameter(1,juce::sendNotification);p->parameters[22]->setParameter(26,juce::sendNotification);p->parameters[28]->setParameter(1,juce::sendNotification);p->parameters[31]->setParameter(18,juce::sendNotification);p->parameters[35]->setParameter(1,juce::sendNotification);p->parameters[36]->setParameter(2,juce::sendNotification);p->parameters[37]->setParameter(70,juce::sendNotification);p->parameters[39]->setParameter(1,juce::sendNotification);p->parameters[40]->setParameter(1,juce::sendNotification);p->parameters[41]->setParameter(.5f,juce::sendNotification);p->parameters[42]->setParameter(-35,juce::sendNotification);p->parameters[43]->setParameter(1,juce::sendNotification);p->parameters[44]->setParameter(13,juce::sendNotification);
    acidFxCheck(h.saveGeneratorPresetFile(id,preset),"expanded synth preset save failed");p->parameters[22]->setParameter(2,juce::sendNotification);acidFxCheck(h.loadGeneratorPresetFile(id,preset) && p->parameters[22]->getCurrentValue()==26 && p->parameters[39]->getCurrentValue()==1,"expanded preset load lost settings");
    auto* editor=dynamic_cast<orbit::synth::AcidBassEditor*>(h.generatorControlWindows.at(id).window->instrument.get());acidFxCheck(editor,"expanded editor missing");editor->setEffectsExpanded(true);editor->setAdvancedExpanded(true);editor->setLfoExpanded(true);
    acidFxCheck(dynamic_cast<FirstSynthReusableHomeKnobLookAndFeel*>(&editor->knobs[0].getLookAndFeel()),"actual Labyrinth Home knob renderer missing");
    for(auto& k:editor->extraKnobs)acidFxCheck(k.isVisible() && editor->getLocalBounds().contains(k.getBounds()) && k.getTextBoxPosition()==juce::Slider::NoTextBox,"extra FX knob clipped/hidden/value box");for(auto& k:editor->lfoKnobs)acidFxCheck(k.isVisible() && editor->getLocalBounds().contains(k.getBounds()),"LFO knob clipped");
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);juce::Timer::callPendingTimersSynchronously();
    acidFxCheck(editor->stageEnabled[0].getToggleState() && editor->stageEnabled[1].getToggleState() && editor->lfoEnabled[0].getToggleState() && editor->lfoTarget[0].getSelectedId()==2,"preset values did not refresh the expanded controls");
    acidFxCheck(editor->stageEnabled[0].findColour(juce::ToggleButton::tickColourId)==juce::Colour(35,45,58),"enable switch has insufficient silver-panel contrast");
    {auto im=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream stream(juce::File("OPTIONS_CHECK_FOLDER/acid-expanded-white.png"));juce::PNGImageFormat().writeImageToStream(im,stream);}
    h.createAcidDemo();h.closePianoRollView();h.project.setPlaylistClip(id,h.project.getPatternId(id,0),0,0,true);h.refreshEnginePlaylist();h.songModeButton.onClick();
    h.setTrackEqEnabled(0,false);h.setTrackEqEnabled(h.getMixerTracks().size(),false);h.setMasterGainDb(0);
    const float baseCutoff=p->parameters[1]->getCurrentValue(),baseDrive=p->parameters[22]->getCurrentValue();
    auto steady=isolatedModulation(h,*p,0),modulated=isolatedModulation(h,*p,1),cancelled=isolatedModulation(h,*p,2);
    acidFxCheck(p->parameters[1]->getCurrentValue()==baseCutoff && p->parameters[22]->getCurrentValue()==baseDrive,"LFO overwrote saved base parameters");
    acidFxCheck(audioDifference(modulated,steady)>5,"LFO routes did not modulate fresh native audio");
    double squared=0;float largest=0;for(int i=0;i<steady.getNumSamples();++i){const float error=std::abs(cancelled.getSample(0,i)-steady.getSample(0,i));squared+=double(error)*error;largest=juce::jmax(largest,error);}
    const double cancellationRms=std::sqrt(squared/steady.getNumSamples());
    std::cout<<"LFO audio difference="<<audioDifference(modulated,steady)<<"; cancellation RMS="<<cancellationRms<<", max="<<largest<<"\n";
    acidFxCheck(cancellationRms<1.e-6 && largest<1.e-5,"equal opposite LFO routes did not cancel within float DSP precision");
    h.addInstrumentTrack(true);h.selectGeneratorControlTrack(id);
    auto& hosted=h.engine.getDeviceManager().getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters device;device.sampleRate=48000;device.inputChannels=0;device.outputChannels=2;hosted.initialise(device);hosted.prepareToPlay(48000,512);
    h.patternModeButton.onClick();h.saveOption("recordContent",1);h.recordButton.onClick();h.playButton.onClick();acidFxCheck(h.edit->getTransport().isPlaying() && h.automationTakeActive,"live rack setup did not start armed automation");
    auto undo=h.undoHistory.size();h.addInsert(0,11);acidFxCheck(rack(0) && h.undoHistory.size()==undo+1,"rack insertion missing/created multiple Undo steps");
    auto rackWindow=h.mixerWindow->effectWindow;acidFxCheck(rackWindow->rackPanel!=nullptr,"rack did not automatically open rack editor");
    auto* add=dynamic_cast<juce::ComboBox*>(rackControl(*rackWindow->rackPanel,"multiFxAdd"));acidFxCheck(add,"rack Add control missing");
    for(int kind=1;kind<=10;++kind){add->setSelectedId(kind,juce::sendNotificationSync);acidFxCheck(h.edit->getTransport().isPlaying(),"rack edit stopped playback");}
    h.stopButton.onClick();h.songModeButton.onClick();
    acidFxCheck(rack(0)->type->getPlugins().size()==10,"not all ten existing effects could enter rack");
    h.refreshSongEventEditor();auto child=rack(0)->type->getPlugins()[8];auto target=h.targetForParameter(*child->getAutomatableParameter(0));acidFxCheck(target.pluginId==child->itemID.toString(),"rack child automation discovery failed");
    auto all=acidFxRender(h,"rack-all-effects");acidFxCheck(all.getMagnitude(0,all.getNumSamples())>1.e-8,"all-effects rack silent");
    auto* chain=dynamic_cast<juce::ComboBox*>(rackControl(*rackWindow->rackPanel,"multiFxChain"));auto* remove=dynamic_cast<juce::TextButton*>(rackControl(*rackWindow->rackPanel,"multiFxRemove"));
    for(int i=0;i<8;++i){chain->setSelectedId(1,juce::sendNotificationSync);remove->onClick();}
    acidFxCheck(rack(0)->type->getPlugins().size()==2,"rack Remove failed");chain->setSelectedId(2,juce::sendNotificationSync);dynamic_cast<juce::TextButton*>(rackControl(*rackWindow->rackPanel,"multiFxEarlier"))->onClick();acidFxCheck(rack(0)->type->getPlugins()[0]->getName()=="Two-Band Distortion","rack reorder failed");
    auto* enabled=dynamic_cast<juce::ToggleButton*>(rackControl(*rackWindow->rackPanel,"multiFxEnabled"));enabled->setToggleState(false,juce::dontSendNotification);enabled->onClick();acidFxCheck(!rack(0)->type->getPlugins()[0]->isEnabled(),"rack per-effect bypass failed");
    auto image=rackWindow->createComponentSnapshot(rackWindow->getLocalBounds());{juce::FileOutputStream stream(juce::File("OPTIONS_CHECK_FOLDER/multi-fx-rack.png"));juce::PNGImageFormat().writeImageToStream(image,stream);}
    h.copyInsertEffects(0);undo=h.undoHistory.size();acidFxCheck(h.pasteInsertEffects(1,false) && h.undoHistory.size()==undo+1,"rack copy not atomic");acidFxCheck(rack(0)->type!=rack(1)->type && rack(0)->type->getPlugins()[0]->itemID!=rack(1)->type->getPlugins()[0]->itemID,"rack paste shared instances");
    h.undoProjectAction();acidFxCheck(rack(1)==nullptr,"rack paste Undo failed");h.redoProjectAction();acidFxCheck(rack(1) && rack(1)->type->getPlugins().size()==2,"rack paste Redo failed");
    auto* automated=rack(0)->type->getPlugins()[1];auto drive=automated->getAutomatableParameter(0);
    juce::Array<ProjectDocument::AutomationPoint> rackEvents;rackEvents.add({0,.25f});rackEvents.add({256,.65f});
    h.project.replaceSongAutomationPoints(h.project.getTrackId(0),automated->getName(),drive->paramID,drive->getParameterName(),rackEvents,automated->itemID.toString());h.updateEngineAutomation();
    h.saveToFile(file);acidFxCheck(h.loadProjectFile(file),"expanded project reopen failed");acidFxCheck(acid(0)->parameters[21]->getCurrentValue()==1 && acid(0)->parameters[35]->getCurrentValue()==1 && rack(0) && rack(1),"expanded persistence lost");
    h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);h.mergeSelectedProjectTracks(file,{0},false);
    acidFxCheck(h.project.getTrackCount()==2 && rack(1) && rack(1)->type->getPlugins().size()==2,"project merge lost rack definition/slots");
    acidFxCheck(!h.edit->state.getParent().isValid(),"merge attached the live engine state");
    acidFxCheck(h.project.getSongAutomationLaneCount(h.project.getTrackId(1))==1 && h.project.getSongAutomationLane(h.project.getTrackId(1),0).pluginId==rack(1)->type->getPlugins()[1]->itemID.toString(),"merge lost rack child automation mapping");h.songModeButton.onClick();auto merged=acidFxRender(h,"rack-merged");acidFxCheck(merged.getMagnitude(0,merged.getNumSamples())>1.e-6,"merged rack audio silent");
    h.undoProjectAction();acidFxCheck(h.project.getTrackCount()==1,"rack merge Undo failed");h.redoProjectAction();acidFxCheck(h.project.getTrackCount()==2 && rack(1),"rack merge Redo lost rack");
    std::cout<<"PASS: rack-aware project merge audio and Undo/Redo.\n";
    std::cout<<"PASS: extra-stage DSP/defaults/block sizes/extremes, synth preset roundtrip, Home knobs and expanded bounds, LFO audio without base writes, all ten rack effects/native audio, child automation, add/remove/reorder/bypass, independent rack copy/Undo/Redo, save/reopen.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
