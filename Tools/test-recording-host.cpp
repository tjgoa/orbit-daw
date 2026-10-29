static void recordingCheck(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
int main()
{
 try
 {
  juce::ScopedJuceInitialiser_GUI gui;
  using orbit::recording::MidiTake;
  MidiTake take;take.start({{"a",0,1024,0,false}});
  const auto event=[&](const char* device,int pitch,int velocity,bool on,bool off,bool sustain,bool down,int tick)
  {take.event("a",device,1,pitch,velocity,on,off,sustain,down,tick);};
  event("one",127,111,true,false,false,false,1000);event("one",127,0,false,true,false,false,1100);
  recordingCheck(take.notes.size()==2 && take.notes[0].start==1000 && take.notes[0].length==24 && take.notes[1].start==0 && take.notes[1].length==76,"pattern boundary lost note duration");
  take.start({{"a",0,1024,0,false}});
  event("one",60,100,true,false,false,false,0);event("two",60,90,true,false,false,false,10);
  event("one",0,0,false,false,true,true,20);event("one",60,0,false,true,false,false,30);
  event("two",60,0,false,true,false,false,40);event("one",0,0,false,false,true,false,50);
  recordingCheck(take.notes.size()==2 && take.notes[0].velocity==90 && take.notes[0].length==30 && take.notes[1].length==50,"sustain/device identity conflated notes");
  take.start({{"a",0,2048,0,true}},64,true);
  event("one",0,100,true,false,false,false,31);event("one",0,0,false,true,false,false,131);
  recordingCheck(take.notes[0].start==0 && take.notes[0].length==128,"independent input quantization wrong");
  take.start({{"a",0,1024,0,true}});event("one",60,100,true,false,false,false,1000);take.finish(1200);
  recordingCheck(take.notes[0].length==24,"song boundary overflowed occupied region");
  ProjectView host;host.stopTimer();host.createProjectDiscardingCurrent();host.addInstrumentTrack();
  const auto id=host.selectedProjectTrackId();recordingCheck(host.recordButton.isEnabled(),"Record remains disabled");
  host.project.appendPatternNoteTicks(id,0,64,7,33,80);host.project.setPatternCreated(id,0,true);host.readPatternSteps();
  const auto before=host.capturePatternData(id,0).notes;const auto undoCount=host.undoHistory.size();
  recordingCheck(host.prepareMidiRecording(0,10000),"empty/selected pattern could not record");
  host.processLiveMidi(juce::MidiMessage::noteOn(1,60,juce::uint8(101)),"one",10000);
  host.processLiveMidi(juce::MidiMessage::noteOff(1,60),"one",10250);
  recordingCheck(host.capturePatternData(id,0).notes==before,"pending take altered authored notes");
  recordingCheck(host.finishMidiRecording(10300),"pattern take commit failed");
  auto notes=host.project.getPatternNotes(id,0);
  recordingCheck(notes.size()==2 && notes[1].startTick==0 && notes[1].lengthTicks==128 && notes[1].velocity==101,"host recording timestamps/velocity wrong");
  recordingCheck(host.undoHistory.size()==undoCount+1,"take did not create exactly one undo");host.undoProjectAction();
  recordingCheck(host.capturePatternData(id,0).notes==before,"take undo lost authored notes");host.redoProjectAction();
  recordingCheck(host.project.getPatternNotes(id,0).size()==2,"take redo failed");
  host.setTrackRecordArm(0,true);host.addInstrumentTrack();const auto other=host.selectedProjectTrackId();
  recordingCheck(host.prepareMidiRecording(0,20000),"armed non-selected generator could not record");
  host.processLiveMidi(juce::MidiMessage::noteOn(1,72,juce::uint8(90)),"one",20000);
  host.processLiveMidi(juce::MidiMessage::noteOff(1,72),"one",20125);host.finishMidiRecording(20200);
  recordingCheck(host.project.getPatternNotes(id,0).size()==3 && host.project.getPatternNotes(other,0).empty(),"record routing followed selected rather than armed generator");
  host.setTrackRecordArm(0,false);host.setPlaybackMode(ProjectView::PlaybackMode::song);
  const auto songUndo=host.undoHistory.size();recordingCheck(host.prepareMidiRecording(8,30000),"song take could not begin at cursor");
  host.processLiveMidi(juce::MidiMessage::noteOn(1,67,juce::uint8(100)),"one",30125);
  host.processLiveMidi(juce::MidiMessage::noteOff(1,67),"one",30500);
  recordingCheck(host.finishMidiRecording(30600),"song take commit failed");
  recordingCheck(host.project.getPlaylistClipStartBar(other,4)==4 && host.project.getPatternNotes(other,0).size()==1,"song take not placed at cursor bar");
  recordingCheck(host.project.getPatternNotes(other,0)[0].startTick==64 && host.project.getPatternNotes(other,0)[0].lengthTicks==192,"song note time wrong");
  recordingCheck(host.undoHistory.size()==songUndo+1,"song take undo grouping wrong");host.undoProjectAction();
  recordingCheck(host.project.getPlaylistPatternId(other,4).isEmpty() && host.project.getPatternNotes(other,0).empty(),"song take undo left clip or notes");host.redoProjectAction();
  recordingCheck(!host.prepareMidiRecording(8,40000),"song take overwrote an occupied clip");
  host.loopButton.setToggleState(true,juce::dontSendNotification);recordingCheck(!host.prepareMidiRecording(0,40000),"song looping was silently misrecorded");host.loopButton.setToggleState(false,juce::dontSendNotification);
  const auto file=juce::File("OPTIONS_CHECK_FOLDER/recording.orbit");host.saveToFile(file);recordingCheck(host.loadProjectFile(file),"recording reopen failed");
  recordingCheck(host.project.getPatternNotes(id,0).size()==3 && host.project.getPatternNotes(other,0).size()==1 && host.project.getPlaylistClipStartBar(other,4)==4,"recording was lost on native reopen");
  // A conflicting edit cannot partially overwrite the project or discard the take.
  recordingCheck(host.prepareMidiRecording(0,50000),"conflict take setup failed");host.processLiveMidi(juce::MidiMessage::noteOn(1,55,juce::uint8(80)),"one",50000);host.processLiveMidi(juce::MidiMessage::noteOff(1,55),"one",50125);
  host.project.setPlaylistClip(other,host.project.getPatternId(other,0),0,1,true);
  const auto conflictUndo=host.undoHistory.size();recordingCheck(!host.finishMidiRecording(50200) && !host.midiTake.notes.empty() && host.undoHistory.size()==conflictUndo,"conflicting take was discarded or partially committed");
  host.project.setPlaylistClip(other,{},0,1,false);recordingCheck(host.finishMidiRecording(50200) && host.midiTake.notes.empty(),"retained take could not be committed after conflict resolution");
  std::cout<<"PASS: pattern/song MIDI takes, source timestamps, full range, sustain and overlapping devices, quantization, loop split/bounds, armed routing, unchanged pending notes, atomic commit/conflict retry, one take undo/redo and native save/reopen. Hardware disabled.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
