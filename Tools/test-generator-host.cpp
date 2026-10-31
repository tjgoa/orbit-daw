// Production generator processing, panel handlers and persistence; called by
// test-options-host.py --generator with hardware/display setup disabled.
static void generatorCheck(bool test,const char* reason){if(!test)throw std::runtime_error(reason);}
int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        struct Cleanup
        {~Cleanup(){checkedApply={};checkedOptions.reset();checkedArpeggiatorCommit={};checkedArpeggiator.reset();checkedGeneratorMenuResult={};checkedGeneratorMenuAnchor=nullptr;}} cleanup;
        using namespace orbit::generator;
        Settings s;s.arp=true;s.octaves=1;
        const std::vector<Note> chord{{60,0,1024,100},{64,0,1024,110},{67,0,1024,90}};
        auto output=render(chord,1024,s);
        generatorCheck(output.size()==16,"arp did not split sustained chord into 16 notes");
        for(size_t i=0;i<output.size();++i){generatorCheck(output[i].pitch==chord[i%3].pitch,"ascending arp order wrong");generatorCheck(output[i].startTick==int(i)*64 && output[i].lengthTicks==64,"arp timing or gate wrong");}
        s.order=1;output=render(chord,1024,s);generatorCheck(output[0].pitch==67 && output[1].pitch==64 && output[2].pitch==60,"descending order wrong");
        s.order=2;output=render(chord,1024,s);generatorCheck(output[0].pitch==60 && output[2].pitch==67 && output[3].pitch==64 && output[4].pitch==60,"up/down order wrong");
        s.order=0;s.octaves=2;s.distance=7;output=render(chord,1024,s);generatorCheck(output[3].pitch==67 && output[4].pitch==71,"non-octave distance ignored");
        s.octaves=1;s.repeats=1;output=render(chord,1024,s);generatorCheck(output.size()==3,"finite repeats ignored");
        s.repeats=0;s.rhythm=0x5555;output=render(chord,1024,s);generatorCheck(output.size()==8,"rhythm rests ignored");
        s.rhythm=65535;s.retrigger=4;output=render(chord,1024,s);generatorCheck(output[3].pitch==60 && output[4].pitch==60 && output[5].pitch==64,"retrigger did not reset after its specified phrase");s.retrigger=0;
        const std::vector<Note> separated{{60,0,128,100},{64,0,128,100},{67,0,128,100},{72,256,128,100},{76,256,128,100},{79,256,128,100}};
        s.resetOnChord=true;output=render(separated,512,s);generatorCheck(output[2].pitch==72,"chord trigger did not reset sequence");
        s.resetOnChord=false;output=render(separated,512,s);generatorCheck(output[2].pitch==79,"disabled chord reset did not continue sequence");s.resetOnChord=true;
        s.rhythm=65535;s.gate=-50;output=render(chord,1024,s);generatorCheck(output[0].lengthTicks==32,"gate ignored");
        s.gate=0;s.shuffle=50;output=render(chord,1024,s);generatorCheck(output[1].startTick==96 && output[2].startTick==128,"arp shuffle timing wrong");
        s.arp=false;output=render({{60,64,32,100}},256,s);generatorCheck(output.size()==1 && output[0].startTick==96 && output[0].lengthTicks==32,"standalone shuffle changed note length");
        s.shuffle=0;s.arp=true;s.order=5;s.customSize=3;s.custom={0,7,3};output=render({{60,0,1024,100}},1024,s);generatorCheck(output[1].pitch==67 && output[2].pitch==63,"user note order ignored");
        generatorCheck(decode(encode(s))==s,"generator settings roundtrip failed");
        // Full-range input and overlapping note-offs remain bounded.
        for(int pitch:{0,127}){s.order=0;s.octaves=5;output=render({{pitch,0,1024,100}},1024,s);for(const auto& n:output)generatorCheck(n.pitch>=0 && n.pitch<=127 && n.lengthTicks>0,"arp emitted invalid MIDI range");}
        // Exercise the actual engine plugin on consecutive audio blocks, using
        // event timestamps in seconds exactly as PluginNode supplies them.
        ProjectView host;host.stopTimer();host.createProjectDiscardingCurrent();host.addInstrumentTrack();
        const auto id=host.selectedProjectTrackId();auto* plugin=host.generatorMidiPlugin(id,true);generatorCheck(plugin!=nullptr,"custom generator MIDI plugin was not registered");
        generatorCheck(host.getInsertPlugins(0).isEmpty(),"generator processor consumed an effect insert slot");
        plugin->initialise({tracktion::TimePosition::fromSeconds(0),48000,480});
        s={};s.arp=true;s.octaves=1;plugin->setSettings(s);
        te::MidiMessageArray midi;juce::AudioBuffer<float> audio(2,480);std::vector<juce::MidiMessage> events;
        for(int block=0;block<60;++block)
        {
            midi.clear();if(block==0)for(int pitch:{60,64,67})midi.addMidiMessage(juce::MidiMessage::noteOn(1,pitch,juce::uint8(100)),{});
            te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),0,480,&midi,0,
                {tracktion::TimePosition::fromSeconds(block*.01),tracktion::TimePosition::fromSeconds((block+1)*.01)},true,false,false,false);
            plugin->applyToBuffer(context);for(const auto& m:midi)events.push_back(m);
        }
        std::vector<int> pitches;for(const auto& m:events)if(m.isNoteOn())pitches.push_back(m.getNoteNumber());
        generatorCheck(pitches.size()==5 && pitches[0]==60 && pitches[1]==64 && pitches[2]==67,"live audio-block arp did not continue across blocks");
        midi.clear();midi.isAllNotesOff=true;te::PluginRenderContext panic(&audio,juce::AudioChannelSet::stereo(),0,480,&midi,0,{tracktion::TimePosition::fromSeconds(.6),tracktion::TimePosition::fromSeconds(.61)},false,false,false,false);plugin->applyToBuffer(panic);
        generatorCheck(std::none_of(midi.begin(),midi.end(),[](const auto& m){return m.isNoteOn();}),"panic restarted held arpeggio");
        s={};s.transpose=12;s.velocity=50;s.outputChannel=2;plugin->setSettings(s);midi.clear();midi.addMidiMessage(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),{});plugin->applyToBuffer(panic);
        generatorCheck(std::any_of(midi.begin(),midi.end(),[](const auto& m){return m.isNoteOn() && m.getNoteNumber()==72 && m.getChannel()==2 && m.getVelocity()==50;}),"general generator MIDI controls did not process real MIDI");
        // Pending take replay enters before the same transpose/velocity/channel
        // path as live input, and is flushed instead of exported offline.
        plugin->overdub.publish({{60,0,64,100,0}},1024,1024,0,true);midi.clear();
        te::PluginRenderContext replay(&audio,juce::AudioChannelSet::stereo(),0,480,&midi,0,
            {tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(.01)},true,false,false,false);
        plugin->applyToBuffer(replay);
        generatorCheck(std::any_of(midi.begin(),midi.end(),[](const auto& m){return m.isNoteOn() && m.getNoteNumber()==72 && m.getChannel()==2 && m.getVelocity()==50;}),"overdub bypassed generator processing");
        midi.clear();replay.isRendering=true;plugin->applyToBuffer(replay);
        generatorCheck(std::none_of(midi.begin(),midi.end(),[](const auto& m){return m.isNoteOn();}),"runtime take leaked into an offline render");plugin->overdub.clear();
        // Real menu and form handlers, keeping generator identity pinned.
        auto menu=host.makeGeneratorMenu(id);generatorCheck(menu.getNumItems()==7,"generator menu hierarchy wrong");
        bool separateControls=false;
        for(juce::PopupMenu::MenuItemIterator item(menu);item.next();)
            if(item.getItem().itemID==5)separateControls=item.getItem().isEnabled;
        generatorCheck(separateControls,"built-in generator lacks separate control window menu");
        host.patternToolbarCollapsed=true;host.resized();
        generatorCheck(host.generatorSettingsButton.isVisible() && !host.pianoRollButton.isVisible(),"hiding pattern controls hid the generator menu");
        generatorCheck(host.generatorSurface.getLocalBounds().withHeight(30).contains(host.generatorSettingsButton.getBounds())
            && !host.generatorSettingsButton.getBounds().intersects(host.generatorDragHandle.getBounds()),"generator dropdown is not a separate header hit target");
        host.generatorSettingsButton.onClick();generatorCheck(checkedGeneratorMenuAnchor==&host.generatorSettingsButton && bool(checkedGeneratorMenuResult),"header dropdown did not present its generator menu");
        checkedGeneratorMenuResult(4);generatorCheck(checkedArpeggiator!=nullptr,"header dropdown did not open arpeggiator");
        host.generatorCollapsed=true;host.resized();generatorCheck(host.generatorSettingsButton.isVisible(),"rolling up generator hid its menu");
        host.allGeneratorsMinimized=true;host.resized();generatorCheck(!host.generatorSettingsButton.isVisible(),"minimized generator left a stray menu button");
        host.allGeneratorsMinimized=false;host.generatorCollapsed=false;host.patternToolbarCollapsed=false;host.resized();
        host.showGeneratorProperties(id);generatorCheck(checkedOptions!=nullptr,"generator settings did not open");checkedOptions->numbers.at("transpose")->setValue(12);generatorCheck(checkedApply(*checkedOptions),"generator settings apply failed");
        generatorCheck(host.generatorSettings(id).transpose==12,"generator transpose not stored");host.undoProjectAction();generatorCheck(host.generatorSettings(id).transpose==0,"generator settings undo failed");host.redoProjectAction();generatorCheck(host.generatorSettings(id).transpose==12,"generator settings redo failed");
        host.selectPatternSlot(0);for(const auto& n:chord)host.project.appendPatternNoteTicks(id,0,n.pitch,n.startTick,n.lengthTicks,n.velocity);host.project.setPatternCreated(id,0,true);host.readPatternSteps();
        const auto original=host.capturePatternData(id,0).notes;s=host.generatorSettings(id);s.arp=true;s.octaves=1;
        generatorCheck(host.applyGeneratorArpeggio(id,0,s,false),"arp apply failed");generatorCheck(host.capturePatternData(id,0).notes==original,"arp apply overwrote authored notes");
        host.showGeneratorArpeggiator(id,0);generatorCheck(checkedArpeggiator!=nullptr && checkedArpeggiator->on.getToggleState(),"arp panel did not restore settings");
        generatorCheck(checkedArpeggiatorCommit(checkedArpeggiator->getSettings(),true),"arp Render handler failed");
        generatorCheck(host.capturePatternData(id,0).notes.size()==16 && !host.generatorSettings(id).arp,"render did not bake and disable arp");
        generatorCheck(host.capturePatternData(id,0).notes[0].pitch==60 && host.generatorSettings(id).transpose==12,"render double-applied generator transpose");
        host.undoProjectAction();generatorCheck(host.capturePatternData(id,0).notes==original && host.generatorSettings(id).arp,"render undo did not restore notes and arp together");
        host.renameGeneratorOrPattern(id,0,false);checkedOptions->texts.at("name")->setText("Arp Lead");generatorCheck(checkedApply(*checkedOptions) && host.engineTrackForProjectId(id)->getName()=="Arp Lead","generator rename failed");
        host.renameGeneratorOrPattern(id,0,true);checkedOptions->texts.at("name")->setText("First Arp");generatorCheck(checkedApply(*checkedOptions) && host.project.getPatternName(id,0)=="First Arp","pattern rename failed");
        const auto targets=host.generatorControllerTargets(id);host.handleGeneratorMenuResult(id,0,1001,targets);host.processLiveMidi(juce::MidiMessage::controllerEvent(1,74,50),"fixture");generatorCheck(host.generatorLearnTarget.isEmpty(),"generator MIDI learn remained armed");
        host.addInstrumentTrack();const auto second=host.selectedProjectTrackId();host.applyControllerBindings(juce::MidiMessage::controllerEvent(1,74,127),"fixture");generatorCheck(host.engineTrackForProjectId(id)->getVolumePlugin()->getPan()>.99f && std::abs(host.engineTrackForProjectId(second)->getVolumePlugin()->getPan())<.01f,"learned controller followed selection instead of original generator");
        generatorCheck(host.setGeneratorMidiSource(second,id),"generator MIDI receive route could not be established");
        auto* input=host.edit->getCurrentInstanceForInputDevice(&host.engineTrackForProjectId(id)->getMidiInputDevice());generatorCheck(input!=nullptr && input->getTargets().contains(host.engineTrackForProjectId(second)->itemID),"engine MIDI destination missing");
        const auto undoCount=host.undoHistory.size();generatorCheck(!host.setGeneratorMidiSource(id,second) && host.undoHistory.size()==undoCount,"feedback routing was not rejected atomically");
        generatorCheck(host.setGeneratorMidiSource(second,{}),"generator MIDI disconnect failed");host.undoProjectAction();generatorCheck(host.engineTrackForProjectId(second)->state.getProperty("orbitMidiSource").toString()==id,"MIDI routing undo lost source identity");
        const auto file=juce::File("OPTIONS_CHECK_FOLDER/generator.orbit");const auto before=host.generatorSettings(id);host.saveToFile(file);generatorCheck(host.loadProjectFile(file),"generator native reopen failed");generatorCheck(host.generatorSettings(id)==before && host.project.getPatternName(id,0)=="First Arp","generator settings/name lost on native reopen");
        host.edit->getTransport().ensureContextAllocated();input=host.edit->getCurrentInstanceForInputDevice(&host.engineTrackForProjectId(id)->getMidiInputDevice());generatorCheck(input!=nullptr && input->getTargets().contains(host.engineTrackForProjectId(second)->itemID),"MIDI route lost on native reopen");
        juce::MidiFile orderFile;orderFile.setTicksPerQuarterNote(96);juce::MidiMessageSequence orderNotes;
        for(int i=0;i<3;++i){auto note=juce::MidiMessage::noteOn(1,60+i*4,juce::uint8(100));note.setTimeStamp(i*96);orderNotes.addEvent(note);}orderFile.addTrack(orderNotes);
        const auto midiPath=juce::File("OPTIONS_CHECK_FOLDER/order.mid");{juce::FileOutputStream stream(midiPath);generatorCheck(orderFile.writeTo(stream),"MIDI order fixture write failed");}
        Settings loadedOrder;juce::String orderError;generatorCheck(host.loadGeneratorNoteOrder(midiPath,loadedOrder,orderError) && loadedOrder.order==5 && loadedOrder.customSize==3 && loadedOrder.custom[2]==8,"user MIDI order import failed");
        for(int skin:{5,6})
        {
            host.setSkin(skin);host.showGeneratorArpeggiator(id,0);auto image=checkedArpeggiator->createComponentSnapshot(checkedArpeggiator->getLocalBounds());juce::FileOutputStream out(juce::File(juce::String("OPTIONS_CHECK_FOLDER/arpeggiator-skin")+juce::String(skin)+".png"));juce::PNGImageFormat format;generatorCheck(format.writeImageToStream(image,out),"arp render failed");
        }
        // The rack's own header must target its original generator and remembered
        // pattern, even while another generator is selected behind that window.
        host.addDrumRackTrack();const auto rackId=host.selectedProjectTrackId();host.selectPatternBank(1);host.selectPatternSlot(1);
        for(const auto& n:chord)host.project.appendPatternNoteTicks(rackId,9,n.pitch,n.startTick,n.lengthTicks,n.velocity);
        host.project.setPatternCreated(rackId,9,true);host.readPatternSteps();
        generatorCheck(host.drumRackWindow!=nullptr && host.drumRackWindow->embedded && host.generatorSettingsButton.isVisible(),"Drum Rack has no shared generator header");
        generatorCheck(host.drumRackWindow->getParentComponent()==&host.generatorBody,"Drum Rack is detached from its generator");
        host.generatorSettingsButton.onClick();
        generatorCheck(checkedGeneratorMenuAnchor==&host.generatorSettingsButton,"Drum Rack dropdown used another menu anchor");
        host.addInstrumentTrack();const auto otherId=host.selectedProjectTrackId();host.selectPatternSlot(0);
        checkedGeneratorMenuResult(4);
        s=checkedArpeggiator->getSettings();s.arp=true;s.octaves=1;
        generatorCheck(checkedArpeggiatorCommit(s,true),"Drum Rack dropdown could not render arp");
        generatorCheck(host.project.getPatternNotes(rackId,9).size()==16 && host.project.getPatternNotes(otherId,0).empty(),"Drum Rack menu lost original generator or remembered pattern");
        std::cout<<"PASS: visible header dropdown with collapsed controls and rolled generator, Drum Rack dropdown with pinned generator/pattern, generator menu/settings, ascending/descending/bounce/custom arps, range/distance, finite repeats, rhythm, gate, shuffle, full MIDI range, production audio-block processing and panic, untouched authored notes, Render/undo, pinned controller learn, names and native reopen, two panel renders. Hardware/native display disabled.\n";
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
