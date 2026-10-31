// Driver included by test-options-host.py after the production host source.
// Hardware, modal presentation and desktop display setup are isolated by that runner.
static void requireOption(bool condition,const char* message)
{if(!condition)throw std::runtime_error(message);}
static void optionNumber(const char* id,double value)
{checkedOptions->numbers.at(id)->setValue(value,juce::dontSendNotification);}
static void optionChoice(const char* id,int value)
{checkedOptions->choices.at(id)->setSelectedItemIndex(value,juce::dontSendNotification);}
static void optionCheck(const char* id,bool value)
{checkedOptions->checks.at(id)->setToggleState(value,juce::dontSendNotification);}
static void applyOptions()
{requireOption(checkedOptions!=nullptr && bool(checkedApply),"panel not opened");requireOption(checkedApply(*checkedOptions),"valid settings refused");checkedApply={};checkedOptions.reset();}
static void renderOptions(const char* name)
{
    auto image=checkedOptions->createComponentSnapshot(checkedOptions->getLocalBounds());
    requireOption(image.isValid(),"panel snapshot failed");
    juce::FileOutputStream stream(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+name+".png"));
    juce::PNGImageFormat png;requireOption(png.writeImageToStream(image,stream),"panel render failed");
    for(auto* child:checkedOptions->getChildren())
        requireOption(checkedOptions->getLocalBounds().contains(child->getBounds()) && !child->getBounds().isEmpty(),"panel viewport or action clipped");
}
struct OptionsMidiListener : te::AudioTrack::Listener
{
    std::vector<juce::MidiMessage> events;
    void injectLiveMidiMessage(te::AudioTrack&,const te::MidiMessageWithSource& message,bool& used) override
    {events.push_back(message);used=true;}
    void recordedMidiMessageSentToPlugins(te::AudioTrack&,const juce::MidiMessage&) override {}
    void clear(){events.clear();}
};
// Mimic the SDK's device recreation on setEnabled. This catches stale pointers
// in the production policy without opening any OS MIDI endpoint.
struct OptionsMidiOutput : te::MidiOutputDevice
{
    explicit OptionsMidiOutput(te::Engine& engine,std::shared_ptr<std::vector<juce::MidiMessage>> messages)
        :MidiOutputDevice(engine,juce::MidiDeviceInfo("Options Check Output","options-check-output")),sent(std::move(messages)){enabled=false;}
    void setEnabled(bool on) override
    {
        if(enabled==on)return;auto& outputs=engine.getDeviceManager().midiOutputs;
        for(size_t i=0;i<outputs.size();++i)if(outputs[i].get()==this)
        {
            auto replacement=std::make_shared<OptionsMidiOutput>(engine,sent);replacement->enabled=on;
            outputs[i]=std::move(replacement);return; // this may now be destroyed
        }
    }
    void sendMessageNow(const juce::MidiMessage& message) override {sent->push_back(message);}
    std::shared_ptr<std::vector<juce::MidiMessage>> sent;
};
static void requireWorkspaceSkin(ProjectView& host,int skin)
{
    const auto preset=orbit::options::workspaceAppearance(skin);
    requireOption(bool(preset),"unknown coordinated skin");
    requireOption(host.optionInt("skin")==skin && host.skinIsSelected(skin),"workspace preset checkmark/persistence failed");
    requireOption(host.playlistWindow->isLightTheme()==preset->lightCanvas,"Playlist did not follow preset");
    requireOption(host.pianoRollWindow->isLightTheme()==preset->lightCanvas,"Piano Roll did not follow preset");
    requireOption(host.mixerWindow->getMixerSkin()==preset->mixerSkin,"Mixer did not follow preset");
}
static double renderWorkspaceSkin(ProjectView& host,const char* name)
{
    host.playlistWindow->setSize(1000,380);host.playlistWindow->refresh();
    host.pianoRollWindow->setSize(1000,440);host.pianoRollWindow->refresh("A1");
    host.mixerWindow->setSize(600,540);host.mixerWindow->setEqSectionVisible(true);
    double brightness=0;
    for(auto entry:std::vector<std::pair<const char*,juce::Component*>>{{"playlist",host.playlistWindow.get()},{"piano-roll",host.pianoRollWindow.get()},{"mixer",host.mixerWindow.get()}})
    {
        auto image=entry.second->createComponentSnapshot(entry.second->getLocalBounds());requireOption(image.isValid(),"workspace render failed");
        juce::FileOutputStream stream(juce::File(juce::String("OPTIONS_CHECK_FOLDER/")+name+"-"+entry.first+".png"));
        juce::PNGImageFormat png;requireOption(png.writeImageToStream(image,stream),"workspace image write failed");
        if(juce::String(entry.first)=="piano-roll")
            for(int y=40;y<image.getHeight();y+=16)for(int x=140;x<image.getWidth();x+=16)brightness+=image.getPixelAt(x,y).getBrightness();
    }
    return brightness;
}
int main(int argc,char** argv)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        if(argc>=3 && juce::String(argv[1])=="--verify-skin")
        {
            ProjectView reopened;reopened.stopTimer();const int skin=juce::String(argv[2]).getIntValue();requireWorkspaceSkin(reopened,skin);
            std::cout<<"PASS: coordinated workspace preset restored in a fresh process: "<<skin<<"\n";
            if(argc>=4)reopened.setSkin(juce::String(argv[3]).getIntValue());return 0;
        }
        OptionsMidiListener first,second;
        ProjectView host;host.stopTimer();host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.addInstrumentTrack();
        const auto a=host.project.getTrackId(0),b=host.project.getTrackId(1);
        const auto select=[&](const juce::String& id){host.instrumentTrack=host.engineTrackForProjectId(id);host.readPatternSteps();};
        const auto listen=[&]{host.engineTrackForProjectId(a)->getListeners().add(&first);host.engineTrackForProjectId(b)->getListeners().add(&second);first.clear();second.clear();};
        listen();select(a);
        host.project.appendPatternNoteTicks(a,0,60,1,1,100);host.project.appendPatternNoteTicks(a,0,72,123,51,70);host.project.setPatternCreated(a,0,true);host.readPatternSteps();
        const auto original=host.capturePatternData(a,0).notes;
        auto menu=host.makeOptionsMenu();std::set<int> ids;
        for(juce::PopupMenu::MenuItemIterator it(menu,true);it.next();){const auto& item=it.getItem();if(item.itemID>0){ids.insert(item.itemID);requireOption(!item.text.containsIgnoreCase("not available") && !item.text.containsIgnoreCase("coming"),"Options placeholder remains");}}
        for(int id:{501,502,503,504,505,506,507,508,509,510,511,512,513,516,517,518,519,5500,5501,5502,5503,5504,5505,5506,5600,5601,5602,5603,5610,5611,5612})requireOption(ids.contains(id),"Options command missing");
        juce::StringArray topEntries;
        for(juce::PopupMenu::MenuItemIterator it(menu);it.next();)if(!it.getItem().isSeparator)topEntries.add(it.getItem().text);
        const juce::StringArray orionOrder{"Skins","Wallpapers","Audio Input Settings...","Audio Output Settings...","MIDI In Settings...","MIDI In Controllers...","Enable MIDI Out","Virtual MIDI Keyboard","Sample Paths...","VST Paths...","Time Signature...","Timebase..."};
        requireOption(topEntries.size()>=orionOrder.size(),"Options menu incomplete");
        for(int i=0;i<orionOrder.size();++i)requireOption(topEntries[i]==orionOrder[i],"Options menu does not follow Orion order");
        host.handleMenuResult(4,5505);requireWorkspaceSkin(host,5);const auto navyBrightness=renderWorkspaceSkin(host,"navy-blue");
        host.handleMenuResult(4,5506);requireWorkspaceSkin(host,6);const auto lightBrightness=renderWorkspaceSkin(host,"light-blue");
        requireOption(lightBrightness>navyBrightness,"light preset did not lighten the Piano Roll");
        host.handleMenuResult(4,5505);requireWorkspaceSkin(host,5);
        host.pianoRollWindow->setLightTheme(true);host.pianoRollWindow->onThemeChanged(true);requireOption(!host.skinIsSelected(5),"custom override left a misleading preset checkmark");
        host.handleMenuResult(4,5505);requireWorkspaceSkin(host,5);
        host.handleMenuResult(4,503);renderOptions("audio-output");optionNumber("cpu",85);optionCheck("clip",true);applyOptions();requireOption(host.optionInt("cpuLimit")==85 && host.optionBool("outputClip"),"audio output policy not applied");
        host.handleMenuResult(4,504);renderOptions("audio-input");optionCheck("route",true);requireOption(!checkedApply(*checkedOptions),"input routing accepted a synth track");optionCheck("route",false);optionChoice("depth",0);optionNumber("latency",24);applyOptions();requireOption(host.optionInt("inputLatency")==24 && host.optionInt("audioDepth")==0,"audio input options not applied");
        host.handleMenuResult(4,505);renderOptions("midi-input");optionChoice("mode",1);optionChoice("channel",3);optionCheck("bend",false);optionCheck("pressure",false);optionCheck("program",false);optionChoice("pedals",2);applyOptions();requireOption(int(host.engineTrackForProjectId(a)->state.getProperty("orbitMidiChannel",0))==3,"generator MIDI channel not saved");
        host.engineTrackForProjectId(b)->state.setProperty("orbitMidiChannel",4,nullptr);first.clear();second.clear();
        host.processLiveMidi(juce::MidiMessage::noteOn(3,60,(juce::uint8)99),"Device A");requireOption(first.events.size()==1 && second.events.empty(),"advanced routing ignored channel");
        host.processLiveMidi(juce::MidiMessage::noteOff(3,60),"Device A");
        auto count=first.events.size();host.processLiveMidi(juce::MidiMessage::pitchWheel(3,8192),"Device A");host.processLiveMidi(juce::MidiMessage::channelPressureChange(3,99),"Device A");host.processLiveMidi(juce::MidiMessage::programChange(3,7),"Device A");requireOption(first.events.size()==count,"MIDI message filters ignored");
        host.processLiveMidi(juce::MidiMessage::controllerEvent(3,64,127),"Device A");requireOption(first.events.back().getControllerValue()==0,"inverted pedal ignored");
        host.saveOption("midiMode",0);host.saveOption("pedals",1);first.clear();second.clear();select(a);
        host.processLiveMidi(juce::MidiMessage::noteOn(1,64,(juce::uint8)100),"Device A");select(b);host.processLiveMidi(juce::MidiMessage::noteOff(1,64),"Device A");requireOption(first.events.size()==2 && first.events.back().isNoteOff() && second.events.empty(),"note-off followed newly selected generator");
        select(a);host.processLiveMidi(juce::MidiMessage::noteOn(1,67,(juce::uint8)100),"Device A");select(b);host.processLiveMidi(juce::MidiMessage::noteOn(1,67,(juce::uint8)100),"Device B");requireOption(host.liveMidiNotes.size()==2,"overlapping devices lost held notes");
        host.processLiveMidi(juce::MidiMessage::noteOff(1,67),"Device A");requireOption(host.liveMidiNotes.size()==1,"first device released another device's note");host.releaseLiveMidi();requireOption(host.liveMidiNotes.empty(),"panic left held MIDI notes");
        select(a);host.handleMenuResult(4,506);renderOptions("midi-controllers");
        optionNumber("cc",17);optionChoice("channel",2);optionChoice("target",1);optionNumber("low",0);optionNumber("high",100);applyOptions();
        auto bindings=host.controllerBindings();requireOption(bindings.size()==1 && bindings[0].cc==17 && bindings[0].channel==2,"controller assignment not saved");
        auto history=host.undoHistory.size();host.processLiveMidi(juce::MidiMessage::controllerEvent(2,17,0),"Device A");host.processLiveMidi(juce::MidiMessage::controllerEvent(2,17,127),"Device A");
        requireOption(std::abs(host.engineTrackForProjectId(a)->getVolumePlugin()->getPan()-1)<0.001 && host.undoHistory.size()==history+1,"CC mapping or grouped undo failed");
        host.undoProjectAction();listen();select(a);requireOption(std::abs(host.instrumentTrack->getVolumePlugin()->getPan())<0.001,"controller undo failed");
        host.handleMenuResult(4,506);host.learnControllerPanel=checkedOptions.get();host.learnControllerDevices={juce::String{}};
        host.processLiveMidi(juce::MidiMessage::controllerEvent(6,74,99),"Device A");requireOption(checkedOptions->value("cc")==74 && checkedOptions->index("channel")==6 && host.learnControllerPanel==nullptr,"CC learn did not fill controls");checkedApply={};checkedOptions.reset();
        // A real instrument parameter must use its normalized (possibly skewed)
        // range. Repeated identical values do not open extra undo transactions.
        auto* synth=host.selectedInstrumentPlugin();te::AutomatableParameter* mapped=nullptr;
        for(auto* candidate:synth->getAutomatableParameters())if(candidate->paramID=="filterFreq"){mapped=candidate;break;}
        requireOption(mapped!=nullptr,"continuous synth parameter missing");const auto parameterId=mapped->paramID;
        host.saveControllerBindings({{juce::String{},"parameter\n"+synth->getName()+"\n"+parameterId,0,71,0,1}});
        host.processLiveMidi(juce::MidiMessage::controllerEvent(1,71,87),"Device A");
        requireOption(std::abs(mapped->getCurrentNormalisedValue()-87.0f/127)<0.01f,"CC did not map through normalized parameter range");
        history=host.undoHistory.size();host.processLiveMidi(juce::MidiMessage::controllerEvent(1,71,87),"Device A");requireOption(host.undoHistory.size()==history,"identical CC consumed undo history");
        auto sent=std::make_shared<std::vector<juce::MidiMessage>>();auto& outputs=host.engine.getDeviceManager().midiOutputs;
        outputs.push_back(std::make_shared<OptionsMidiOutput>(host.engine,sent));const auto outputId=outputs.back()->getDeviceID();host.saveOption("midiOutputDevice",outputId);
        host.setMidiOutputEnabled(true);requireOption(host.engine.getDeviceManager().findOutputDeviceForID(outputId)->isEnabled(),"MIDI output did not enable after recreation");
        host.processLiveMidi(juce::MidiMessage::noteOn(1,75,(juce::uint8)100),"Device A");requireOption(!sent->empty() && sent->back().isNoteOn(),"live MIDI thru did not reach selected output");
        host.setMidiOutputEnabled(false);requireOption(!host.engine.getDeviceManager().findOutputDeviceForID(outputId)->isEnabled(),"MIDI output did not disable after recreation");
        requireOption(std::any_of(sent->begin(),sent->end(),[](const auto& m){return m.isAllNotesOff();}),"disabling output did not release external notes");
        std::erase_if(outputs,[&](const auto& output){return output->getDeviceID()==outputId;});host.saveOption("midiOutputDevice",juce::String{});
        host.handleMenuResult(4,518);renderOptions("midi-output");checkedApply={};checkedOptions.reset();
        host.handleMenuResult(4,510);renderOptions("timebase");optionNumber("ppq",96);optionChoice("display",2);applyOptions();requireOption(host.midiExportPpq()==96 && host.capturePatternData(a,0).notes==original,"timebase retimed native notes");requireOption(host.transportTimeText(1.5,1)=="1:4:0","time display incorrect");
        host.undoProjectAction();listen();select(a);requireOption(host.midiExportPpq()==te::Edit::ticksPerQuarterNote,"timebase undo failed");host.redoProjectAction();listen();select(a);
        for(int ppq:{24,96,960,15360})
        {
            host.edit->state.setProperty("orbitMidiExportPpq",ppq,nullptr);juce::MidiMessageSequence tempo,sequence;host.appendPatternToMidiSequence(sequence,a,0,0);
            auto file=host.encodeMidiExport(tempo,{sequence});juce::MemoryOutputStream bytes;requireOption(file.writeTo(bytes),"MIDI encoding failed");juce::MemoryInputStream input(bytes.getData(),bytes.getDataSize(),false);juce::MidiFile read;requireOption(read.readFrom(input) && read.getTimeFormat()==ppq,"export timebase not written");
            auto* track=read.getTrack(1);requireOption(track!=nullptr,"note track missing");for(int i=0;i<track->getNumEvents();++i)if(track->getEventPointer(i)->message.isNoteOn()){const int off=track->getIndexOfMatchingKeyUp(i);requireOption(off>=0 && track->getEventTime(off)>=track->getEventTime(i)+1,"low PPQ generated zero-length note");}
        }
        host.edit->state.setProperty("orbitMidiExportPpq",96,nullptr);
        host.handleMenuResult(4,509);renderOptions("time-signature");optionNumber("numerator",7);optionChoice("denominator",3);optionCheck("triplet",true);applyOptions();requireOption(host.project.getInitialTimeSignature().numerator==7 && host.project.getInitialTimeSignature().denominator==8 && bool(host.edit->state.getProperty("orbitTripletGrid",false)),"meter/triplet policy not applied");requireOption(host.capturePatternData(a,0).notes==original,"meter changed native notes");
        const auto folder=juce::File("OPTIONS_CHECK_FOLDER");auto sampleFolder=folder.getChildFile("Sample Folder");sampleFolder.createDirectory();host.applicationProperties.getUserSettings()->setValue("samplePaths",sampleFolder.getFullPathName());requireOption(host.firstSampleFolder()==sampleFolder,"configured sample path ignored");
        host.saveToFile(folder.getChildFile("Options Check.orbit"));requireOption(host.loadProjectFile(folder.getChildFile("Options Check.orbit")),"native reopen failed");select(a);requireOption(host.midiExportPpq()==96 && bool(host.edit->state.getProperty("orbitTripletGrid",false)) && int(host.instrumentTrack->state.getProperty("orbitMidiChannel",0))==3 && host.capturePatternData(a,0).notes==original,"native options roundtrip failed");
        const auto base=orbit::options::colour(40,60,80);
        for(int i=1;i<5;++i){host.handleMenuResult(4,5500+i);requireOption(host.optionInt("skin")==i && orbit::options::colour(40,60,80)!=base,"skin had no effect");}
        host.handleMenuResult(4,5500);requireOption(orbit::options::colour(40,60,80)==base,"default skin did not restore original drawing");
        for(int i=0;i<3;++i){host.handleMenuResult(4,5600+i);requireOption(host.optionInt("wallpaper")==i,"wallpaper command ignored");auto image=host.createComponentSnapshot(host.getLocalBounds());requireOption(image.isValid(),"wallpaper render failed");}
        host.handleMenuResult(4,5612);requireOption(host.optionInt("wallpaperMode")==2,"wallpaper placement ignored");
        const auto library=host.wallpaperLibrary();
        requireOption(library.folder.isDirectory() && library.folder.getFullPathName().startsWith("OPTIONS_CHECK_FOLDER"),"wallpaper folder missing or escaped isolated settings");
        const auto external=juce::File("OPTIONS_CHECK_FOLDER/external/wallpaper.png");external.getParentDirectory().createDirectory();
        auto writeImage=[&](juce::Colour colour)
        {juce::Image image(juce::Image::RGB,20,20,true);juce::Graphics g(image);g.fillAll(colour);
         juce::FileOutputStream out(external);out.setPosition(0);out.truncate();juce::PNGImageFormat png;requireOption(png.writeImageToStream(image,out),"test image write failed");};
        writeImage(juce::Colours::red);requireOption(host.useWallpaper(external,true),"chosen wallpaper could not be copied");
        const auto stored=juce::File(host.optionString("wallpaperFile"));
        requireOption(stored.getParentDirectory()==library.folder && external.existsAsFile() && stored.hasIdenticalContentTo(external),"wallpaper did not retain a private copy");
        const auto wallpaperCount=library.images().size();requireOption(host.useWallpaper(external,true) && library.images().size()==wallpaperCount,"same wallpaper copied twice");
        writeImage(juce::Colours::blue);requireOption(host.useWallpaper(external,true),"same-name image import failed");
        requireOption(juce::File(host.optionString("wallpaperFile"))!=stored && !stored.hasIdenticalContentTo(external),"same-name import overwrote old wallpaper");
        const auto added=library.folder.getChildFile("Added.png");external.copyFileTo(added);host.makeOptionsMenu();
        const int addedIndex=host.wallpaperMenuFiles.indexOf(added);requireOption(addedIndex>=0,"folder image not added to menu");
        host.handleMenuResult(4,ProjectView::wallpaperMenuBase+addedIndex);requireOption(host.optionString("wallpaperFile")==added.getFullPathName(),"folder menu did not select its image");
        const auto broken=library.folder.getChildFile("Broken.png");broken.replaceWithText("not an image");const auto previous=host.optionString("wallpaperFile");
        requireOption(!host.useWallpaper(broken,false) && host.optionString("wallpaperFile")==previous,"bad image replaced the selected wallpaper");
        added.deleteFile();host.makeOptionsMenu();requireOption(!host.wallpaperMenuFiles.contains(added),"removed wallpaper stayed in menu");
        requireOption(host.handleEditShortcut(juce::KeyPress(juce::KeyPress::F12Key)) && checkedKeyboardToggles==1,"F12 did not dispatch keyboard toggle");
        std::vector<juce::MidiMessage> keyboardEvents;
        {orbit::options::VirtualKeyboard::Content keyboard([&](const auto& m){keyboardEvents.push_back(m);});keyboard.state.noteOn(1,60,0.7f);keyboard.state.noteOn(1,64,0.8f);keyboard.state.noteOff(1,60,0);requireOption(keyboardEvents.size()==3 && keyboardEvents[0].isNoteOn() && keyboardEvents[1].getNoteNumber()==64 && keyboardEvents[2].isNoteOff(),"virtual keyboard not polyphonic");keyboard.release();requireOption(keyboardEvents.size()>3,"virtual keyboard panic ignored");}
        orbit::options::MidiInbox inbox;int delivered=0;bool overflow=false;inbox.deliver=[&](const auto& messages,bool lost){delivered=int(messages.size());overflow=lost;};
        for(int i=0;i<4100;++i)inbox.handleIncomingMidiMessage(nullptr,juce::MidiMessage::noteOn(1,60,(juce::uint8)100));inbox.dispatchNow();requireOption(overflow && delivered<=4096 && delivered>0,"bounded MIDI inbox overflow policy failed");
        requireOption(host.capturePatternData(a,0).notes==original,"global options changed note data");
        std::cout<<"PASS: Orion Options order, coordinated Navy Blue/Light Blue workspace presets and renders; Options menu entries; real input/output/MIDI/controller/meter/timebase panels and renders; channel routing and filters; original-recipient note-offs; overlapping-device notes; panic; CC mappings, normalized parameter ranges, learn and undo; recreated MIDI output enable/disable, live thru and panic; PPQ MIDI encoding with nonzero lengths; unchanged native timing; save/reopen; skins/wallpapers; F12 dispatch; polyphonic keyboard state; bounded asynchronous MIDI delivery. Hardware and native desktop presentation disabled.\n";
        host.handleMenuResult(4,5505);requireWorkspaceSkin(host,5);
        checkedApply={};checkedOptions.reset();
    }
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
