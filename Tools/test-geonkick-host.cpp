// Production Orbit + real ChowKick VST3, isolated settings and no audio hardware.
bool saveNativePeer(juce::Component&,const juce::File&);
bool nativeGeometryMatchesPeer(juce::Component&);
class DrumAuditionProbe final:public juce::JUCEApplication,private juce::Timer
{
    std::unique_ptr<ProjectView> host;
    class Shell final:public juce::DocumentWindow
    {public:Shell():DocumentWindow("Orbit drum audition verification",juce::Colours::black,closeButton){setUsingNativeTitleBar(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->quit();}};
    std::unique_ptr<Shell> shell;
    std::vector<juce::String> paths{NATIVE_PLUGIN_PATHS};
    juce::String first,second,chow;bool fresh=false;int phase=0;
    const juce::File file{"OPTIONS_CHECK_FOLDER/drum-audition.orbit"};
    static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
    orbit::synth::GeonkickPlugin* plugin(const juce::String& id){return dynamic_cast<orbit::synth::GeonkickPlugin*>(host->generatorControlPlugin(id));}
    juce::AudioBuffer<float> render(const juce::String& name)
    {
        juce::WavAudioFormat format;te::Renderer::Parameters p(*host->edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&format;p.sampleRateForAudio=48000;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;
        p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)};
        te::Renderer::RenderTask task("Drum audition test",p,nullptr,nullptr);int count=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++count<100000){}
        check(count<100000 && task.errorMessage.isEmpty(),"engine render failed");
        std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(p.destFile),true));check(reader!=nullptr,"render unreadable");
        juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));reader->read(&audio,0,audio.getNumSamples(),0,true,true);check(audio.getMagnitude(0,audio.getNumSamples())>.001,"native render silent");return audio;
    }
    void directNative()
    {
        auto* p=plugin(first);check(p && p->engineReady(),"Geonkick not registered as instrument");
        te::PluginInitialisationInfo info{};info.sampleRate=48000;info.blockSizeSamples=512;p->initialise(info);
        auto* reference=orbit_geonkick_create();orbit_geonkick_prepare(reference,48000);orbit_geonkick_controls(reference,orbit::synth::GeonkickPlugin::defaults.data());
        juce::AudioBuffer<float> audio(2,530);audio.clear();te::MidiMessageArray midi;midi.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),48./48000,{});
        te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),7,512,&midi,0,{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(512./48000)},true,false,false,false);
        p->applyToBuffer(context);
        for(int n=0;n<512;++n){if(n==48)orbit_geonkick_trigger(reference,36,100);const auto expected=orbit_geonkick_sample(reference);check(audio.getSample(0,n+7)==expected && audio.getSample(1,n+7)==expected,"MIDI offset/stereo differs from actual DSP");}
        check(audio.getSample(0,6)==0 && audio.getSample(0,519)==0,"plugin overwrote adjacent audio");
        midi.isAllNotesOff=true;p->applyToBuffer(context);check(audio.getMagnitude(7,512)==0,"MIDI panic leaked percussion tail");p->deinitialise();orbit_geonkick_destroy(reference);
    }
    juce::PluginDescription chowDescription()
    {
        check(paths.size()==1,"Supply ChowKick VST3 path");juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,paths[0]);
        for(auto* d:found)if(d->isInstrument)return *d;throw std::runtime_error("ChowKick VST3 scan failed");
    }
    void chowAudio(const juce::PluginDescription& d)
    {
        juce::VST3PluginFormat format;juce::String error;auto processor=format.createInstanceFromDescription(d,48000,512,error);check(processor!=nullptr,"ChowKick instance failed");
        processor->prepareToPlay(48000,512);double energy=0;juce::AudioBuffer<float> buffer(juce::jmax(2,processor->getTotalNumOutputChannels()),512);juce::MidiBuffer midi;
        for(int b=0;b<50;b++){buffer.clear();midi.clear();if(b==0)midi.addEvent(juce::MidiMessage::noteOn(1,36,juce::uint8(110)),0);processor->processBlock(buffer,midi);for(int i=0;i<512;i++){float x=buffer.getSample(0,i);check(std::isfinite(x),"ChowKick produced invalid samples");energy+=x*x;}}
        check(energy>.0001,"ChowKick MIDI render silent");processor->releaseResources();std::cout<<"PASS: real universal ChowKick VST3 scanned, instantiated and rendered MIDI-triggered audio.\n"<<std::flush;
    }
    void load()
    {
        // Real Orbit initializes its device before projects/tracks. Match that
        // order so their default outputs bind to the hosted device correctly.
        auto& device=host->engine.getDeviceManager().getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;params.sampleRate=48000;params.inputChannels=0;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);
        if(fresh)
        {
            check(host->loadProjectFile(file),"fresh-process reopen failed");first=host->project.getTrackId(0);second=host->project.getTrackId(1);chow=host->project.getTrackId(2);
            check(plugin(first) && plugin(first)->parameters[6]->getCurrentValue()==77,"fresh native Drive lost");check(plugin(second) && plugin(second)->parameters[0]->getCurrentValue()==1,"fresh second native recipe lost");
            check(dynamic_cast<te::ExternalPlugin*>(host->generatorControlPlugin(chow))!=nullptr,"fresh ChowKick state lost");render("fresh-drum-render");std::cout<<"PASS: fresh-process Geonkick recipes/parameters/patterns and ChowKick plugin state reopen and render.\n";quit();return;
        }
        host->createProjectDiscardingCurrent();host->handleMenuResult(3,449);first=host->selectedProjectTrackId();directNative();host->createGeonkickDemo();const int count=int(host->project.getPatternNotes(first,0).size());check(count==8,"demo notes missing");host->createGeonkickDemo();check(host->project.getTrackSelectedPatternIndex(first)==1 && int(host->project.getPatternNotes(first,0).size())==count,"demo overwrote existing pattern");
        host->addInstrumentTrack(false,true);second=host->selectedProjectTrackId();plugin(second)->parameters[0]->setParameter(1,juce::sendNotificationSync);host->createGeonkickDemo();
        auto& a=*host->generatorControlWindows.at(first).window;auto& b=*host->generatorControlWindows.at(second).window;a.setBounds(15,85,850,310);b.setBounds(400,390,850,310);
        auto* editor=dynamic_cast<orbit::synth::GeonkickEditor*>(a.instrument.get());check(editor && editor->knobs[0].getTextBoxPosition()==juce::Slider::NoTextBox,"compact editor missing");
        const auto undo=host->undoHistory.size();editor->knobs[5].onDragStart();editor->knobs[5].setValue(55,juce::sendNotificationSync);editor->knobs[5].setValue(77,juce::sendNotificationSync);editor->knobs[5].onDragEnd();check(plugin(first)->parameters[6]->getCurrentValue()==77 && host->undoHistory.size()==undo+1,"native gesture/undo binding failed");
        int event=-1;editor->onEditEvents=[&](int p,juce::Component*){event=p;};editor->knobs[3].onEvents(&editor->knobs[3]);check(event==4,"event menu addresses wrong parameter");editor->onEditEvents={};
        for(int row=0;row<2;++row){auto id=host->project.getTrackId(row);host->project.setPlaylistClip(id,host->project.getPatternId(id,0),0,0,true);host->setTrackEqEnabled(row,false);}host->setTrackEqEnabled(host->getMixerTracks().size(),false);host->setMasterGainDb(0);host->songModeButton.onClick();host->refreshEnginePlaylist();render("native-drum-render");
        auto target=host->targetForParameter(*plugin(first)->parameters[7]);orbit::automation::Points points;points.add({0,0});points.add({16,1});host->project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,points,target.pluginId);host->updateEngineAutomation();check(plugin(first)->parameters[7]->hasAutomationPoints(),"native automation target not projected");plugin(first)->parameters[7]->updateStream();render("automated-drum-render");
        auto desc=chowDescription();chowAudio(desc);host->addVst3Instrument(desc);chow=host->selectedProjectTrackId();startTimer(1500);
    }
    void timerCallback()override
    {
        try
        {
            stopTimer();host->syncGeneratorNativeViews();auto& c=*host->generatorControlWindows.at(chow).window;auto* native=dynamic_cast<OrbitPluginEditorContent*>(c.instrument.get());check(native && native->editorReady() && native->nativeClip.attachedCount()>0 && nativeGeometryMatchesPeer(*native),"ChowKick GUI not embedded in Orbit");
            auto& a=*host->generatorControlWindows.at(first).window;auto& b=*host->generatorControlWindows.at(second).window;
            if(phase==0)
            {
                c.setTopLeftPosition(650,85);a.toFront(false);b.toFront(false);host->showPlaylist();host->showSongEventEditor();host->showMixer(false);
                const auto aa=a.getBounds(),bb=b.getBounds(),cc=c.getBounds();host->selectGeneratorControlTrack(first);host->stopPlayback();check(a.isVisible() && b.isVisible() && c.isVisible() && a.getBounds()==aa && b.getBounds()==bb && c.getBounds()==cc,"selection/Stop changed independent windows");
                // Let the newly allocated hosted graph settle before preview.
                host->edit->getTransport().ensureContextAllocated();phase=2;startTimer(250);return;
            }
            if(phase==2)
            {
                auto& device=host->engine.getDeviceManager().getHostedAudioDeviceInterface();
                const auto aa=a.getBounds(),bb=b.getBounds(),cc=c.getBounds();
                // Audition through the actual hosted graph with no hardware.
                auto* preview=dynamic_cast<orbit::synth::GeonkickEditor*>(b.instrument.get());preview->hitButton.onClick();juce::AudioBuffer<float> buffer(2,512);juce::MidiBuffer midi;double peak=0;
                for(int n=0;n<40;++n){buffer.clear();midi.clear();device.processBlock(buffer,midi);peak=juce::jmax(peak,double(buffer.getMagnitude(0,512)));}
                check(plugin(second)->auditionVelocity.load()==0,"live graph did not process the audition request");check(peak>.0001,"Hear hit produced no live host audio");
                host->patternModeButton.onClick();host->playButton.onClick();check(host->edit->getTransport().isPlaying(),"Pattern Play failed");host->stopButton.onClick();host->songModeButton.onClick();host->playButton.onClick();check(host->edit->getTransport().isPlaying(),"Song Play failed");host->stopButton.onClick();
                check(a.isVisible() && b.isVisible() && c.isVisible() && a.getBounds()==aa && b.getBounds()==bb && c.getBounds()==cc,"Play/Stop moved or hid audition panels");
                host->saveToFile(file);host->playlistWindow->setVisible(false);host->songEventWindow->setVisible(false);host->mixerWindow->setVisible(false);a.toFront(false);b.toFront(false);phase=1;startTimer(600);return;
            }
            check(a.isVisible() && b.isVisible() && c.isVisible(),"one audition replaced another");check(saveNativePeer(*host,juce::File("OPTIONS_CHECK_FOLDER/drum-auditions.png")),"native workspace screenshot failed");
            std::cout<<"PASS: actual Orbit native peer hosts two Geonkick windows plus ChowKick, independent placement, compact knob UI, grouped Undo, parameter Event target, protected demo slots, sample-accurate native MIDI/panic, project render/automation and save.\n"<<std::flush;quit();
        }
        catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";setApplicationReturnValue(1);quit();}
    }
public:
    const juce::String getApplicationName()override{return "Orbit drum audition verification";}
    const juce::String getApplicationVersion()override{return "test";}
    void initialise(const juce::String& args)override
    {
        fresh=args.contains("fresh");host=std::make_unique<ProjectView>();host->stopTimer();host->setSize(1440,880);shell=std::make_unique<Shell>();shell->setContentNonOwned(host.get(),false);shell->setBounds(20,35,1440,910);shell->setVisible(true);
        juce::MessageManager::callAsync([this]{try{load();}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";setApplicationReturnValue(1);quit();}});
    }
    void shutdown()override{stopTimer();if(shell)shell->clearContentComponent();host.reset();shell.reset();}
};
START_JUCE_APPLICATION(DrumAuditionProbe)
