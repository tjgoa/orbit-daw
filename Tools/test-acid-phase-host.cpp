// Native phase switch: settings ownership, Undo, actual MIDI/DSP and persistence.
#define ORBIT_ACID_CHECK_FOLDER "OPTIONS_CHECK_FOLDER"
#define main acidFxRegressionMain
#include "test-acid-fx-host.cpp"
#undef main
static void phaseNativeMidi(orbit::synth::AcidBassPlugin& plugin,bool enabled)
{
    constexpr int index=int(orbit::synth::AcidBassPlugin::phaseRetriggerIndex);
    plugin.parameters[index]->setParameter(enabled?1:0,juce::sendNotificationSync);
    te::PluginInitialisationInfo info{};info.sampleRate=48000;info.blockSizeSamples=512;plugin.initialise(info);
    orbit::synth::AcidBassVoice voice;voice.prepare(48000);orbit::synth::AcidParameters p;p.phaseRetrigger=enabled;voice.setParameters(p);
    std::array<float,512> expected{};
    for(int n=0;n<512;++n)
    {
        if(n==3)voice.noteOn(1,37,90);if(n==100)voice.noteOff(1,37);if(n==102)voice.noteOn(1,44,90);if(n==350)voice.noteOff(1,44);
        voice.finishEvents();expected[size_t(n)]=voice.sample();
    }
    juce::AudioBuffer<float> audio(2,530);audio.clear();te::MidiMessageArray midi;
    midi.addMidiMessage(juce::MidiMessage::noteOn(1,37,juce::uint8(90)),3./48000,{});
    midi.addMidiMessage(juce::MidiMessage::noteOff(1,37),100./48000,{});
    midi.addMidiMessage(juce::MidiMessage::noteOn(1,44,juce::uint8(90)),102./48000,{});
    midi.addMidiMessage(juce::MidiMessage::noteOff(1,44),350./48000,{});
    te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),9,512,&midi,0,{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(512./48000)},true,false,false,false);
    plugin.applyToBuffer(context);const int latency=juce::roundToInt(plugin.getLatencySeconds()*48000);
    for(int n=0;n<512;++n)acidFxCheck(audio.getSample(0,n+9)==(n<latency?0:expected[size_t(n-latency)]) && audio.getSample(0,n+9)==audio.getSample(1,n+9),"phase setting not applied at native MIDI sample position");
    acidFxCheck(audio.getSample(0,8)==0 && audio.getSample(0,521)==0,"phase processor overwrote buffer boundaries");
    midi.isAllNotesOff=true;plugin.applyToBuffer(context);acidFxCheck(audio.getMagnitude(9,512)==0,"phase-enabled native Panic leaked audio");plugin.deinitialise();
}
int main(int argc,char** argv)
{
    try
    {
        if(argc>1 && juce::String(argv[1]).contains("fxfresh"))return acidFxRegressionMain(argc,argv);
        juce::ScopedJuceInitialiser_GUI gui;
        // The harness owns its captured Settings panel globally. Release it
        // before JUCE's Desktop is torn down, including exception paths.
        struct PanelCleanup {~PanelCleanup(){checkedOptions.reset();checkedApply={};}} cleanup;
        ProjectView host;host.stopTimer();host.setSize(1440,900);
        const juce::File file("OPTIONS_CHECK_FOLDER/acid-phase.orbit");
        auto plugin=[&](const juce::String& id){return dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(id));};
        constexpr auto index=orbit::synth::AcidBassPlugin::phaseRetriggerIndex;
        if(argc>1)
        {
            acidFxCheck(host.loadProjectFile(file),"fresh phase project reopen failed");auto* a=plugin(host.project.getTrackId(0));auto* b=plugin(host.project.getTrackId(1));
            acidFxCheck(a && b && a->parameters[index]->getCurrentValue()==1 && b->parameters[index]->getCurrentValue()==0,"fresh instance phase states lost");
            auto audio=acidFxRender(host,"phase-fresh");acidFxCheck(audio.getMagnitude(0,audio.getNumSamples())>.001,"fresh phase-enabled render silent");
            std::cout<<"PASS: fresh-process per-instance phase state and native audio render.\n";return 0;
        }
        host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto a=host.selectedProjectTrackId();
        acidFxCheck(plugin(a)->parameters[index]->getCurrentValue()==0,"new phase switch not off by default");phaseNativeMidi(*plugin(a),false);phaseNativeMidi(*plugin(a),true);
        auto old=plugin(a)->state.createCopy();old.removeProperty("phaseRetrigger",nullptr);plugin(a)->restorePluginStateFromValueTree(old);acidFxCheck(plugin(a)->parameters[index]->getCurrentValue()==0,"legacy state enabled phase retrigger");
        host.createAcidDemo();host.closePianoRollView();host.addInstrumentTrack(true);const auto b=host.selectedProjectTrackId();
        // Open A's settings while B stays selected: the target must remain A.
        host.handleGeneratorMenuResult(a,0,2,{});acidFxCheck(checkedOptions && checkedOptions->checks.count("phaseRetrigger"),"Generator Settings lacks phase checkbox");
        auto undo=host.undoHistory.size();checkedOptions->checks.at("phaseRetrigger")->setToggleState(true,juce::dontSendNotification);
        acidFxCheck(plugin(a)->parameters[index]->getCurrentValue()==0,"checkbox changed sound before Apply");
        acidFxCheck(checkedApply(*checkedOptions) && plugin(a)->parameters[index]->getCurrentValue()==1 && plugin(b)->parameters[index]->getCurrentValue()==0,"phase Apply followed selected generator");
        acidFxCheck(host.undoHistory.size()==undo+1 && host.isDirty,"phase Apply missing Undo/dirty state");
        acidFxCheck(checkedApply(*checkedOptions) && host.undoHistory.size()==undo+1,"unchanged settings created another Undo");
        host.undoProjectAction();acidFxCheck(plugin(a)->parameters[index]->getCurrentValue()==0,"phase Undo failed");host.redoProjectAction();acidFxCheck(plugin(a)->parameters[index]->getCurrentValue()==1,"phase Redo failed");
        host.showGeneratorProperties(a);acidFxCheck(checkedOptions->checked("phaseRetrigger"),"settings did not restore saved checkbox");
        checkedOptions->choices.at("input")->setSelectedItemIndex(2,juce::dontSendNotification);checkedOptions->numbers.at("transpose")->setValue(12,juce::dontSendNotification);checkedOptions->checks.at("phaseRetrigger")->setToggleState(false,juce::dontSendNotification);
        undo=host.undoHistory.size();acidFxCheck(checkedApply(*checkedOptions) && host.undoHistory.size()==undo+1 && host.generatorSettings(a).transpose==12 && plugin(a)->parameters[index]->getCurrentValue()==0,"combined MIDI/phase Apply failed");
        host.undoProjectAction();acidFxCheck(host.generatorSettings(a).transpose==0 && plugin(a)->parameters[index]->getCurrentValue()==1,"combined Undo did not restore MIDI and phase together");
        host.addDrumRackTrack();host.showGeneratorProperties(host.selectedProjectTrackId());acidFxCheck(!checkedOptions->checks.count("phaseRetrigger"),"phase option appeared for a different instrument");
        host.selectGeneratorControlTrack(a);host.project.setPlaylistClip(a,host.project.getPatternId(a,0),0,0,true);host.refreshEnginePlaylist();host.songModeButton.onClick();host.setTrackEqEnabled(0,false);host.setMasterGainDb(0);host.setTrackEqEnabled(host.getMixerTracks().size(),false);
        auto audio=acidFxRender(host,"phase-native");acidFxCheck(audio.getMagnitude(0,audio.getNumSamples())>.001,"phase-enabled native song render silent");
        host.showGeneratorProperties(a);checkedOptions->setSize(640,390);{auto image=checkedOptions->createComponentSnapshot(checkedOptions->getLocalBounds());juce::FileOutputStream out(juce::File("OPTIONS_CHECK_FOLDER/acid-phase-settings.png"));juce::PNGImageFormat().writeImageToStream(image,out);}
        host.saveToFile(file);auto stale=checkedApply;host.createProjectDiscardingCurrent();acidFxCheck(!stale(*checkedOptions),"stale generator settings applied to another project");
        acidFxCheck(host.loadProjectFile(file) && plugin(a)->parameters[index]->getCurrentValue()==1,"same-process phase state lost");
        std::cout<<"PASS: phase native MIDI/audio/Panic, default/legacy Off, settings target/Apply/Undo/Redo, combined MIDI settings, stale project guard and per-instance persistence.\n";
        return acidFxRegressionMain(1,nullptr);
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
