// Production 9W9 native host integration, temporary preferences, hosted audio.
bool saveNativePeer(juce::Component&,const juce::File&);
class NineProbe final:public juce::JUCEApplication,private juce::Timer
{
    std::unique_ptr<ProjectView> host;
    class Shell final:public juce::DocumentWindow
    {public:Shell():DocumentWindow("Orbit 9W9 verification",juce::Colours::black,closeButton){setUsingNativeTitleBar(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->quit();}};
    std::unique_ptr<Shell> shell;juce::String first,second;bool fresh=false;int phase=0;
    const juce::File file{"OPTIONS_CHECK_FOLDER/nine-kit.orbit"};
    static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
    orbit::synth::NineKitPlugin* plugin(const juce::String& id){return dynamic_cast<orbit::synth::NineKitPlugin*>(host->generatorControlPlugin(id));}
    static size_t index(const char* name){for(size_t i=0;i<orbit::synth::nine::specs.size();++i)if(juce::String(name)==orbit::synth::nine::specs[i].id)return i;throw std::runtime_error("unknown parameter");}
    juce::AudioBuffer<float> render(const juce::String& name)
    {
        juce::WavAudioFormat format;te::Renderer::Parameters p(*host->edit);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.audioFormat=&format;p.sampleRateForAudio=48000;p.bitDepth=32;p.usePlugins=true;p.useMasterPlugins=true;
        p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(2)};te::Renderer::RenderTask task("9W9 check",p,nullptr,nullptr);int n=0;while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++n<100000){}
        check(n<100000 && task.errorMessage.isEmpty(),"native render failed");std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(p.destFile),true));check(reader!=nullptr,"render unreadable");juce::AudioBuffer<float> audio(2,int(reader->lengthInSamples));reader->read(&audio,0,audio.getNumSamples(),0,true,true);check(audio.getMagnitude(0,audio.getNumSamples())>.001,"native kit silent");return audio;
    }
    void directMidi()
    {
        auto* p=plugin(first);te::PluginInitialisationInfo info{};info.sampleRate=48000;info.blockSizeSamples=512;p->initialise(info);
        auto* reference=orbit_nine_create();orbit_nine_prepare(reference,48000);for(size_t i=0;i<p->accentIndex;++i)orbit_nine_set(reference,orbit::synth::nine::specs[i].id,p->parameters[i]->getCurrentValue());orbit_nine_tempo(reference,120);
        juce::AudioBuffer<float> audio(2,530);audio.clear();te::MidiMessageArray midi;
        // Deliberately put Note On before the simultaneous Accent controller.
        midi.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),37./48000,{});midi.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,1),37./48000,{});
        te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),9,512,&midi,0,{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(512./48000)},true,false,false,false);p->applyToBuffer(context);
        for(int n=0;n<512;++n){if(n==37)orbit_nine_trigger(reference,0,100,1,50);float x=0;orbit_nine_render(reference,&x,1);check(audio.getSample(0,n+9)==x && audio.getSample(1,n+9)==x,"MIDI timing, Accent ordering or stereo differs from DSP");}
        check(audio.getSample(0,8)==0 && audio.getSample(0,521)==0,"adjacent buffer data overwritten");midi.isAllNotesOff=true;p->applyToBuffer(context);check(audio.getMagnitude(9,512)==0,"MIDI panic leaked tails");p->deinitialise();orbit_nine_destroy(reference);
    }
    void load()
    {
        auto& device=host->engine.getDeviceManager().getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters params;params.sampleRate=48000;params.inputChannels=0;params.outputChannels=2;device.initialise(params);device.prepareToPlay(48000,512);
        if(fresh){check(host->loadProjectFile(file),"fresh reopen failed");first=host->project.getTrackId(0);second=host->project.getTrackId(1);check(plugin(first) && plugin(second),"native kit type lost");check(plugin(first)->parameters[index("bd_c_attack")]->getCurrentValue()==101,"native parameter lost");check(host->project.getPatternLengthSteps(first,1)==64 && !host->project.getPatternSwitches(first,1).empty(),"demo pattern/Accent lost");render("fresh-nine");std::cout<<"PASS: fresh-process native kit/parameters/notes/accents/automation reopen and actual audio render.\n";quit();return;}
        host->createProjectDiscardingCurrent();host->handleMenuResult(3,450);first=host->selectedProjectTrackId();check(plugin(first) && plugin(first)->engineReady(),"menu failed to create kit");directMidi();
        auto* e=dynamic_cast<orbit::synth::NineKitEditor*>(host->generatorControlWindows.at(first).window->instrument.get());check(e!=nullptr,"native editor missing");
        e->steps[0].onClick();check(host->project.getPatternNote(first,0,36,0),"Kick step click failed");e->accents[0].onClick();check(!host->project.getPatternSwitches(first,0).empty(),"Accent switch failed");e->setPage(1);e->steps[4].onClick();check(host->project.getPatternNote(first,0,38,4),"Snare step uses wrong note");
        e->setPage(0);auto undo=host->undoHistory.size();e->knobs[1].onDragStart();e->knobs[1].setValue(70,juce::sendNotificationSync);e->knobs[1].setValue(101,juce::sendNotificationSync);e->knobs[1].onDragEnd();check(plugin(first)->parameters[index("bd_c_attack")]->getCurrentValue()==101 && host->undoHistory.size()==undo+1,"parameter binding or gesture Undo failed");
        int event=-1;e->onEditEvents=[&](int p,juce::Component*){event=p;};e->knobs[1].onEvents(&e->knobs[1]);check(event==int(index("bd_c_attack")),"wrong Event target");e->onEditEvents={};
        e->length.setSelectedId(32,juce::sendNotificationSync);e->next.onClick();e->steps[0].onClick();check(host->project.getPatternNote(first,0,36,16),"paged sequencer edits wrong step");
        e->demo.onClick();check(host->project.getTrackSelectedPatternIndex(first)==1 && host->project.getPatternLengthSteps(first,1)==64,"demo overwrote occupied slot or length wrong");std::set<int> pitches;for(const auto& n:host->project.getPatternNotes(first,1))pitches.insert(n.pitch);check(pitches.size()==11,"demo does not exercise eleven drums");
        plugin(first)->parameters[plugin(first)->mapIndex]->setParameter(1,juce::sendNotificationSync);for(int v=0;v<11;++v)check(plugin(first)->voiceForNote(plugin(first)->noteForVoice(v))==v,"pad map not invertible");plugin(first)->parameters[plugin(first)->mapIndex]->setParameter(0,juce::sendNotificationSync);check(plugin(first)->voiceForNote(45)==2 && plugin(first)->voiceForNote(50)==4,"GM aliases wrong");
        host->addInstrumentTrack(false,false,true);second=host->selectedProjectTrackId();check(plugin(second)->parameters[index("bd_c_attack")]->getCurrentValue()!=101,"instances share controls");host->createNineKitDemo();host->addInstrumentTrack(false,true);
        for(int row=0;row<2;++row){auto id=host->project.getTrackId(row);int slot=host->project.getTrackSelectedPatternIndex(id);host->project.setPlaylistClip(id,host->project.getPatternId(id,slot),0,0,true);host->setTrackEqEnabled(row,false);}host->setTrackEqEnabled(host->getMixerTracks().size(),false);host->setMasterGainDb(0);host->songModeButton.onClick();host->refreshEnginePlaylist();auto raw=render("nine-raw");
        auto* p=plugin(first);auto target=host->targetForParameter(*p->parameters[index("volume")]);orbit::automation::Points points;points.add({0,0});points.add({16,1});host->project.replaceSongAutomationPoints(target.trackId,target.pluginName,target.parameterId,target.parameterName,points,target.pluginId);host->updateEngineAutomation();check(p->parameters[index("volume")]->hasAutomationPoints(),"automation not projected");p->parameters[index("volume")]->updateStream();auto automated=render("nine-automated");double difference=0;for(int i=0;i<juce::jmin(raw.getNumSamples(),automated.getNumSamples());++i)difference+=std::abs(raw.getSample(0,i)-automated.getSample(0,i));check(difference>.1,"automation did not affect audio");
        host->generatorControlWindows.at(first).window->setBounds(10,90,1000,520);host->generatorControlWindows.at(second).window->setBounds(350,360,950,520);host->showPlaylist();host->showSongEventEditor();host->showMixer(false);phase=0;startTimer(400);
    }
    void timerCallback()override
    {
        try
        {
            stopTimer();auto& a=*host->generatorControlWindows.at(first).window;auto& b=*host->generatorControlWindows.at(second).window;
            if(phase==0)
            {
                const auto aa=a.getBounds(),bb=b.getBounds();host->selectGeneratorControlTrack(second);auto* e=dynamic_cast<orbit::synth::NineKitEditor*>(b.instrument.get());e->setPage(1);e->hit.onClick();juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;double peak=0;
                for(int n=0;n<40;++n){audio.clear();midi.clear();host->engine.getDeviceManager().getHostedAudioDeviceInterface().processBlock(audio,midi);peak=juce::jmax(peak,double(audio.getMagnitude(0,512)));}check(peak>.001 && plugin(second)->auditionMask.load()==0,"stopped preview silent");
                host->patternModeButton.onClick();host->playButton.onClick();check(host->edit->getTransport().isPlaying(),"Pattern Play failed");host->stopButton.onClick();host->songModeButton.onClick();host->playButton.onClick();check(host->edit->getTransport().isPlaying(),"Song Play failed");host->stopButton.onClick();check(a.isVisible() && b.isVisible() && a.getBounds()==aa && b.getBounds()==bb,"transport/selection closed or moved kit windows");
                e->setPage(1);check(e->tabs[1].getToggleState() && !e->tabs[0].getToggleState(),"selected drum tab lost");check(e->steps[4].getToggleState() && e->steps[12].getToggleState() && !e->steps[0].getToggleState(),"demo step lights disagree with notes");check(e->accents[0].getToggleState() && !e->accents[1].getToggleState(),"demo Accent lights disagree with switches");check(e->steps[4].findColour(juce::TextButton::buttonOnColourId)!=e->steps[4].findColour(juce::TextButton::buttonColourId),"active steps lack a distinct fill");
                host->saveToFile(file);host->playlistWindow->setVisible(false);host->songEventWindow->setVisible(false);host->mixerWindow->setVisible(false);a.toFront(false);b.toFront(false);phase=1;startTimer(500);return;
            }
            check(saveNativePeer(*host,juce::File("OPTIONS_CHECK_FOLDER/nine-kit.png")),"screenshot failed");std::cout<<"PASS: sample-accurate native MIDI/Accent, all eleven demo drums, paged step editor, native controls/Undo/Event, GM/pad maps, actual audio automation, live preview, independent windows, transport and save.\n"<<std::flush;quit();
        }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';setApplicationReturnValue(1);quit();}
    }
public:
    const juce::String getApplicationName()override{return "Orbit 9W9 verification";}
    const juce::String getApplicationVersion()override{return "test";}
    void initialise(const juce::String& args)override
    {fresh=args.contains("fresh");host=std::make_unique<ProjectView>();host->stopTimer();host->setSize(1440,1000);shell=std::make_unique<Shell>();shell->setContentNonOwned(host.get(),false);shell->setBounds(15,30,1440,1030);shell->setVisible(true);juce::MessageManager::callAsync([this]{try{load();}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';setApplicationReturnValue(1);quit();}});}
    void shutdown()override{stopTimer();if(shell)shell->clearContentComponent();host.reset();shell.reset();}
};
START_JUCE_APPLICATION(NineProbe)
