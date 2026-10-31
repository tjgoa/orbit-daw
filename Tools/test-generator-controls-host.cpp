static void controlCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1220,850);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto acidId=host.selectedProjectTrackId();host.addDrumRackTrack();const auto drumId=host.selectedProjectTrackId();
    controlCheck(host.generatorControlWindows.size()==2,"normal insertion did not open independent controls");
    auto& a=*host.generatorControlWindows.at(acidId).window;auto& d=*host.generatorControlWindows.at(drumId).window;
    controlCheck(a.getParentComponent()==&host && d.getParentComponent()==&host && !a.isOnDesktop() && !d.isOnDesktop(),"generator controls escaped Orbit");
    a.setBounds(0,90,600,300);d.setBounds(610,90,600,600);const auto acidBefore=a.getBounds();
    host.generatorDockButtons[0]->onClick();host.generatorDockButtons[1]->onClick();
    controlCheck(a.isVisible() && d.isVisible() && a.getBounds()==acidBefore && !host.generatorSurface.isVisible(),"dock replaced/repositioned another generator or left duplicate shared UI");
    d.maximiseButtonPressed();controlCheck(d.getBounds()==d.workspaceArea(),"generator maximize missed workspace edges");d.maximiseButtonPressed();
    d.minimiseButtonPressed();controlCheck(d.isMinimised() && a.isVisible(),"minimize affected another generator");d.minimiseButtonPressed();
    host.openGeneratorControls(acidId,false);host.openGeneratorControls(drumId,false);
    std::cout << "Initial drum window " << host.generatorControlWindows.at(drumId).window->getBounds().toString();
    if (const auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        std::cout << " desktop " << display->userArea.toString();
    std::cout << "\n";
    controlCheck(host.generatorControlWindows.size()==2 && host.generatorControlWindows.at(acidId).window->isVisible() && host.generatorControlWindows.at(drumId).window->isVisible(),"Acid Bass and Drum Rack did not stay open together");
    a.closeButtonPressed();
    auto playlistEvent=juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),juce::Point<float>(25,float(host.playlistWindow->grid->headerHeight()+10)),juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,host.playlistWindow->grid,host.playlistWindow->grid,juce::Time::getCurrentTime(),{},juce::Time::getCurrentTime(),2,false);
    host.playlistWindow->grid->mouseDoubleClick(playlistEvent);controlCheck(a.isVisible() && d.isVisible(),"Playlist name double click failed");
    a.closeButtonPressed();host.showMixer(false);auto* footer=&host.mixerWindow->channelStrips[0]->footer;
    auto mixerEvent=juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),juce::Point<float>(5,5),juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,footer,footer,juce::Time::getCurrentTime(),{},juce::Time::getCurrentTime(),2,false);
    host.mixerWindow->channelStrips[0]->mouseDoubleClick(mixerEvent);controlCheck(a.isVisible() && d.isVisible(),"Mixer name double click failed");
    host.selectGeneratorControlPattern(drumId,5);host.selectGeneratorControlTrack(acidId);
    auto* drum=dynamic_cast<DrumRackWindow*>(host.generatorControlWindows.at(drumId).window->instrument.get());controlCheck(drum!=nullptr,"Drum Rack popout has no real drum controls");
    const auto undo=host.undoHistory.size();drum->undoGestureHandler("Draw popout drum note");drum->setStepCallback(0,2,true);drum->setStepVelocityCallback(0,2,45);
    controlCheck(host.project.getPatternNote(drumId,5,48,2) && host.project.getPatternNoteVelocity(drumId,5,48,2)==45 && host.project.getPatternNotes(acidId,0).empty(),"Drum popout edited the active generator or wrong pattern");
    controlCheck(host.undoHistory.size()==undo+1 && host.generatorControlWindows.at(drumId).window->slots[5].getToggleState(),"drum gesture split Undo or lost the independent pattern header");
    const auto sample=juce::File("OPTIONS_CHECK_FOLDER/control-sample.wav");juce::WavAudioFormat wav;
    {auto stream=std::make_unique<juce::FileOutputStream>(sample);std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,1,16,{},0));juce::AudioBuffer<float> audio(1,4800);audio.clear();writer->writeFromAudioSampleBuffer(audio,0,4800);}
    auto* sampler=host.findSamplerPlugin(drumId);controlCheck(sampler!=nullptr && sampler->addSound(sample.getFullPathName(),"Control fixture",0,0,0).isEmpty(),"drum sample fixture failed");sampler->setSoundParams(sampler->getNumSounds()-1,48,48,48);
    drum->undoGestureHandler("Adjust popout drum gain");drum->setPadControlCallback(0,DrumRackWindow::PadControl::gain,-6);
    controlCheck(std::abs(host.getDrumRackPadInfo(sampler,0).gainDb+6)<.01,"drum controls edited another sampler");
    host.selectGeneratorControlTrack(drumId);auto* acid=dynamic_cast<orbit::synth::AcidBassEditor*>(host.generatorControlWindows.at(acidId).window->instrument.get());controlCheck(acid!=nullptr,"Acid popout lost its native GUI");
    const auto cutoff=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(acidId))->parameters[1]->getCurrentValue();acid->knobs[1].setValue(2000,juce::sendNotificationSync);
    controlCheck(std::abs(dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(acidId))->parameters[1]->getCurrentValue()-2000)<.01 && host.project.getTrackSelectedPatternIndex(drumId)==5,"Acid popout wrote to the selected drum generator");
    host.undoProjectAction();controlCheck(host.generatorControlWindows.size()==2 && host.generatorControlWindows.at(drumId).window->isVisible(),"Undo closed independent controls");
    controlCheck(std::abs(dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(acidId))->parameters[1]->getCurrentValue()-cutoff)<.01 && host.project.getPatternNoteVelocity(drumId,5,48,2)==45,"popout Undo changed another generator's pattern");host.redoProjectAction();
    host.addInstrumentTrack();const auto fourId=host.selectedProjectTrackId();
    host.handleGeneratorMenuResult(fourId,0,5,{});
    controlCheck(host.generatorControlWindows.size()==3 && dynamic_cast<OrbitNativeInstrumentEditor*>(host.generatorControlWindows.at(fourId).window->instrument.get())!=nullptr,"generator menu did not open a real independent 4OSC editor");
    host.generatorControlWindows.at(fourId).window->closeButtonPressed();
    auto& acidWindow=*host.generatorControlWindows.at(acidId).window;acidWindow.setBounds(180,220,910,330);acidWindow.closeButtonPressed();
    controlCheck(!acidWindow.isVisible() && host.generatorControlWindows.at(drumId).window->isVisible(),"closing one control window hid the other");host.openGeneratorControls(acidId,false);controlCheck(acidWindow.isVisible(),"closed controls did not reopen");
    const auto bounds=acidWindow.getBounds();host.generatorControlWindows.clear();host.openGeneratorControls(acidId,false);controlCheck(host.generatorControlWindows.at(acidId).window->getBounds()==bounds,"per-generator window placement did not persist");host.openGeneratorControls(drumId,false);
    auto& window=*host.generatorControlWindows.at(drumId).window;
    if (juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()==nullptr) window.removeFromDesktop();
    window.setSize(1160,760);
    controlCheck(window.instrument->getWidth()==window.getContentComponent()->getWidth() && window.instrument->getHeight()==window.getContentComponent()->getHeight()-58,"generator body detached from host bounds");
    host.project.setPatternNote(drumId,5,49,4,true);window.onRefresh();
    controlCheck(dynamic_cast<DrumRackWindow*>(window.instrument.get())->content.patternSteps[1].size()==1,"detached drum display did not follow external pattern edits");
    auto* rack=dynamic_cast<DrumRackWindow*>(window.instrument.get());
    const auto height=window.getHeight();rack->onSampleEditorExpanded(true);rack->onSampleEditorExpanded(false);
    controlCheck(window.getHeight()==height && rack->getHeight()==window.getContentComponent()->getHeight()-58,"sample expansion detached the rack from its parent window");
    auto image=window.createComponentSnapshot(window.getLocalBounds());
    {juce::FileOutputStream output(juce::File("OPTIONS_CHECK_FOLDER/generator-control-drum.png"));juce::PNGImageFormat().writeImageToStream(image,output);}
    const auto project=juce::File("OPTIONS_CHECK_FOLDER/generator-controls.orbit");host.saveToFile(project);controlCheck(host.loadProjectFile(project),"generator controls project reopen failed");
    controlCheck(host.generatorControlWindows.empty(),"old control windows retained plugin refs across project replacement");host.openGeneratorControls(acidId,false);host.openGeneratorControls(drumId,false);
    controlCheck(host.generatorControlWindows.size()==2 && host.project.getPatternNoteVelocity(drumId,5,48,2)==45,"independent controls/note data failed after reopen");
    host.createProjectDiscardingCurrent();controlCheck(host.generatorControlWindows.empty(),"new project retained old control windows");
    const juce::String vstPath=GENERATOR_VST_PATH_LITERAL;
    if(vstPath.isNotEmpty())
    {
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,vstPath);const juce::PluginDescription* description=nullptr;
        for(auto* d:found)if(d->isInstrument){description=d;break;}controlCheck(description!=nullptr,"fixture contains no VST3 instrument");
        host.addVst3Instrument(*description);const auto first=host.selectedProjectTrackId();host.addVst3Instrument(*description);const auto second=host.selectedProjectTrackId();
        std::cout<<"External instance tracks="<<host.project.getTrackCount()<<" windows="<<host.generatorControlWindows.size()<<" selected="<<first<<","<<second<<"\n";
        for(auto* track:host.getGeneratorTracks())for(auto* plugin:track->pluginList)if(auto* ext=dynamic_cast<te::ExternalPlugin*>(plugin))std::cout<<"External "<<ext->getName()<<" synth="<<ext->isSynth()<<" missing="<<ext->isMissing()<<" instance="<<(ext->getAudioPluginInstance()!=nullptr)<<"\n";
        controlCheck(first!=second && host.generatorControlWindows.size()==2,"inserting two VST instances did not open independent windows");
        auto& left=*host.generatorControlWindows.at(first).window;auto& right=*host.generatorControlWindows.at(second).window;
        controlCheck(dynamic_cast<OrbitPluginEditorContent*>(left.instrument.get())!=nullptr && dynamic_cast<OrbitPluginEditorContent*>(right.instrument.get())!=nullptr,"external instruments are not hosted by independent native wrappers");
        left.setBounds(20,95,500,500);right.setBounds(560,95,500,500);const auto leftBounds=left.getBounds();host.openGeneratorForRow(0);host.openGeneratorForRow(1);
        controlCheck(left.isVisible() && right.isVisible() && left.getBounds()==leftBounds,"changing selected VST closed/repositioned another VST");
        controlCheck(host.activeInstrumentEditor==nullptr,"shared generator created a duplicate VST editor");
        host.addInstrumentTrack(true);controlCheck(left.isVisible() && right.isVisible(),"built-in insertion closed external generators");
        host.createProjectDiscardingCurrent();controlCheck(host.generatorControlWindows.empty(),"native VST windows survived owning Edit deletion");
        std::cout<<"PASS: two real VST3 instrument instances auto-open independent wrappers, retain selection/placement alongside built-ins, and clear safely with their Edit. Native peer interaction remains manual.\n";
    }

    std::cout<<"PASS: simultaneous Acid Bass/Drum Rack controls, real GUI bodies, independent track/pattern edits, grouped Undo/Redo with window restoration, close/reopen isolation, saved placement, native note/sample/parameter reopen and safe Edit replacement. Hardware disabled.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
