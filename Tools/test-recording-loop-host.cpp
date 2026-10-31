#include <tracktion_graph/tracktion_graph.h>
#include <tracktion_engine/playback/graph/tracktion_ClickNode.h>
static void checkLoopRecording(ProjectView& host)
{
    host.createProjectDiscardingCurrent();host.addInstrumentTrack();host.setPlaybackMode(ProjectView::PlaybackMode::pattern);const auto id=host.selectedProjectTrackId();
    host.project.appendPatternNoteTicks(id,0,64,7,33,80);host.project.setPatternCreated(id,0,true);host.readPatternSteps();
    const auto authored=host.capturePatternData(id,0).notes;const auto undo=host.undoHistory.size();
    recordingCheck(host.prepareMidiRecording(0,10000),"overdub setup failed");
    host.processLiveMidi(juce::MidiMessage::noteOn(1,60,juce::uint8(101)),"one",10000);
    host.processLiveMidi(juce::MidiMessage::noteOff(1,60),"one",10250);
    auto* plugin=host.generatorMidiPlugin(id);recordingCheck(plugin!=nullptr,"overdub processor missing");
    plugin->initialise({tracktion::TimePosition::fromSeconds(0),48000,480});
    te::MidiMessageArray midi;juce::AudioBuffer<float> audio(2,480);
    struct Played {bool on;int pitch;double time;};std::vector<Played> played;
    auto render=[&](double time,bool playing=true)
    {
        midi.clear();const auto phase=std::fmod(time,2.0);
        te::PluginRenderContext context(&audio,juce::AudioChannelSet::stereo(),0,480,&midi,0,
            {tracktion::TimePosition::fromSeconds(phase),tracktion::TimePosition::fromSeconds(phase+.01)},playing,false,false,false);
        plugin->applyToBuffer(context);
        for(const auto& m:midi)if(m.isNoteOnOrOff())played.push_back({m.isNoteOn(),m.getNoteNumber(),time+m.getTimeStamp()});
    };
    for(int block=0;block<620;++block)
    {
        if(block==264)
        {
            host.processLiveMidi(juce::MidiMessage::noteOn(1,67,juce::uint8(90)),"one",12500);
            host.processLiveMidi(juce::MidiMessage::noteOff(1,67),"one",12625);
        }
        render(block*.01);
    }
    std::vector<double> first,second,offs;
    for(const auto& m:played){if(m.on && m.pitch==60)first.push_back(m.time);if(m.on && m.pitch==67)second.push_back(m.time);if(!m.on && m.pitch==60)offs.push_back(m.time);}
    recordingCheck(first.size()==3 && std::abs(first[0]-2)<1e-6 && std::abs(first[1]-4)<1e-6 && std::abs(first[2]-6)<1e-6,"overdub did not repeat exactly on subsequent loop passes");
    recordingCheck(second.size()==1 && std::abs(second[0]-4.5)<1e-6,"new pass replayed immediately or replaced earlier overdub");
    recordingCheck(offs.size()==2 && std::abs(offs[0]-2.25)<1e-6 && std::abs(offs[1]-4.25)<1e-6,"overdub note duration changed");
    recordingCheck(host.capturePatternData(id,0).notes==authored && host.undoHistory.size()==undo,"audible overdub changed the document or Undo before Stop");
    recordingCheck(host.finishMidiRecording(16200),"overdub commit failed");render(6.2,false);
    recordingCheck(!played.back().on && played.back().pitch==60,"stopping left an overdub voice held");
    const auto afterStop=played.size();for(int block=621;block<850;++block)render(block*.01);
    recordingCheck(played.size()==afterStop,"runtime overdub survived its commit");
    recordingCheck(host.project.getPatternNotes(id,0).size()==3 && host.undoHistory.size()==undo+1,"overdub did not commit as one take");
    host.undoProjectAction();recordingCheck(host.capturePatternData(id,0).notes==authored,"overdub Undo changed authored notes");host.redoProjectAction();
    const auto file=juce::File("OPTIONS_CHECK_FOLDER/loop-recording.orbit");host.saveToFile(file);recordingCheck(host.loadProjectFile(file),"overdub reopen failed");
    recordingCheck(host.project.getPatternNotes(id,0).size()==3,"overdub notes lost on reopen");
    // Delayed starts: precount notes audition, but neither they nor their later
    // note-offs enter the take. Native engine position determines the deadline.
    host.edit->setCountInMode(te::Edit::CountIn::oneBar);
    host.edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(-2));
    recordingCheck(host.prepareMidiRecording(0,20000),"count-in fixture failed");host.alignRecordingClockToTransport(0,20000);
    recordingCheck(std::abs(host.recordingStartMs-22000)<1e-6,"count-in clock did not align to transport");
    host.processLiveMidi(juce::MidiMessage::noteOn(1,55,juce::uint8(90)),"one",21999);
    host.processLiveMidi(juce::MidiMessage::noteOff(1,55),"one",22050);
    host.processLiveMidi(juce::MidiMessage::noteOn(1,72,juce::uint8(110)),"one",22000);
    host.processLiveMidi(juce::MidiMessage::noteOff(1,72),"one",22125);
    recordingCheck(host.midiTake.notes.size()==1 && host.midiTake.notes[0].pitch==72 && host.midiTake.notes[0].start==0,"count-in captured precount notes or shifted the first note");
    recordingCheck(host.finishMidiRecording(22200),"count-in take failed");
    const auto cancelUndo=host.undoHistory.size();recordingCheck(host.prepareMidiRecording(4,30000),"cancel fixture failed");
    host.edit->setCountInMode(te::Edit::CountIn::oneBar);host.recordingPunchSeconds=4;host.recordingStartMs=32000;host.recordingUsesEnginePreroll=true;host.updateRecordingCountInStatus(30000);
    recordingCheck(host.status.getText().contains("Count-in"),"no count-in status");
    recordingCheck(host.finishMidiRecording(30500) && host.undoHistory.size()==cancelUndo && !host.recordButton.getToggleState()
        && host.edit->getTransport().getPosition().inSeconds()==4,"canceling precount added Undo or lost the punch cursor");
    // Exercise the engine's own click generator: forced count-in with Metro off,
    // silence afterwards, then continuous clicking when Metro is enabled.
    host.edit->getTransport().ensureContextAllocated();host.edit->clickTrackEnabled=false;
    host.edit->setClickTrackRange({tracktion::TimePosition::fromSeconds(-2),tracktion::TimePosition::fromSeconds(-.25)});
    te::ClickGenerator click(*host.edit,true);click.prepareToPlay(48000,tracktion::TimePosition::fromSeconds(-2.25));
    std::vector<double> clicks;
    for(int block=0;block<275;++block)
    {const auto begin=tracktion::TimePosition::fromSamples(-108000+block*480,48000.0),end=tracktion::TimePosition::fromSamples(-108000+(block+1)*480,48000.0);
        const auto start=begin.inSeconds();midi.clear();click.processBlock(nullptr,&midi,{begin,end});
        for(const auto& m:midi)if(m.isNoteOn())clicks.push_back(start+m.getTimeStamp());}
    std::cout<<"Native click times:";for(auto t:clicks)std::cout<<" "<<t;std::cout<<"\n";
    recordingCheck(clicks.size()==4 && std::abs(clicks.front()+2)<1e-6 && std::abs(clicks.back()+.5)<1e-6,"native count-in click count/timing wrong with Metro off");
    host.edit->clickTrackEnabled=true;midi.clear();click.processBlock(nullptr,&midi,{tracktion::TimePosition::fromSeconds(.5),tracktion::TimePosition::fromSeconds(.51)});
    recordingCheck(std::any_of(midi.begin(),midi.end(),[](const auto& m){return m.isNoteOn();}),"Metro did not continue after precount");host.edit->setClickTrackRange({});
    host.showRecordingOptions();recordingCheck(checkedOptions!=nullptr,"Recording Settings missing");checkedOptions->choices.at("countIn")->setSelectedItemIndex(3);
    recordingCheck(checkedApply(*checkedOptions) && host.optionInt("recordCountIn")==3,"count-in option not saved");checkedApply={};checkedOptions.reset();
    host.edit->setCountInMode(te::Edit::CountIn::oneBar);recordingCheck(host.edit->getNumCountInBeats()==4,"default count-in not four quarters");
    // Different pattern lengths must repeat authored clips through the common
    // cycle, matching the recorder's modulo and runtime replay clocks.
    host.addInstrumentTrack();const auto other=host.selectedProjectTrackId();host.project.appendPatternNoteTicks(other,0,48,0,64,90);host.project.setPatternCreated(other,0,true);
    host.project.setPatternLengthSteps(other,0,24);recordingCheck(host.patternPlaybackCycleTicks()==3072,"common cycle for 4/6 quarter patterns wrong");
    host.auditionPatternPlayback=true;host.refreshEnginePlaylist();
    for(auto* track:host.getGeneratorTracks())for(auto* clip:track->getClips())if(auto* native=dynamic_cast<te::MidiClip*>(clip))
        recordingCheck(std::abs(native->getEditTimeRange().getLength().inSeconds()-6)<1e-6,"shorter authored pattern does not repeat through common cycle");
    host.finishPatternPlayback();
    std::cout<<"PASS: real MIDI-plugin overdub blocks across three passes, later publication, exact ons/offs and stop flush; unchanged pending document, one Undo and reopen; count-in deadline/exclusion/cancel, native click timing and saved settings, mixed-length common cycle. Hardware disabled.\n";
}
