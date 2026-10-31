// Included inside ProjectView. Native document notes remain authoritative;
// a finished take is staged atomically and adds exactly one undo snapshot.
int patternPlaybackCycleTicks() const
{
    std::int64_t cycle=0;
    for(int row=0;row<project.getTrackCount();++row)
        if(project.getTrackKind(row)!=ProjectDocument::TrackKind::audio)
        {
            const auto id=project.getTrackId(row);const int slot=project.getTrackSelectedPatternIndex(id);
            // Silent unrelated patterns do not lengthen an existing performance.
            const bool recording=automationTakeActive || std::any_of(midiTake.targets.begin(),midiTake.targets.end(),[&](const auto& t){return !t.song && t.id==id.toStdString();});
            if(!recording && !patternHasNotes(id,slot) && project.getPatternAutomationLaneCount(id,slot)==0)continue;
            const auto ticks=std::int64_t(project.getPatternLengthSteps(id,slot))*64;
            cycle=cycle==0?ticks:cycle/std::gcd(cycle,ticks)*ticks;
            if(cycle>1024*512)return 0;
        }
    return int(juce::jmax<std::int64_t>(256,cycle));
}
void showRecordingOptions()
{
    const int numerator=project.getInitialTimeSignature().numerator;
    auto panel=std::make_unique<OptionsPanel>("MIDI count-in sounds before a stopped transport records notes. Automation records touched parameter movements from the playback cursor and uses the engine clock. Stop commits one take. The metronome switch controls clicking after the MIDI count-in.");
    panel->choice("content","Record",{"MIDI notes","Automation (knobs / MIDI controls)"},optionInt("recordContent"));
    panel->choice("countIn","Count-in (quarter notes)",{"0 (off)","1","2",juce::String(numerator),juce::String(numerator*2)},optionInt("recordCountIn"));
    openOptionsTool("Recording Settings",std::move(panel),[this](const auto& p)
    {saveOption("recordCountIn",juce::jlimit(0,4,p.index("countIn")));saveOption("recordContent",p.index("content"));recordModeButton.setButtonText(p.index("content")==1?"AUTO":"MIDI");return true;});
}
void clearRecordingOverdub()
{
    if(edit!=nullptr)for(auto* track:getGeneratorTracks())for(auto* p:track->pluginList)
        if(auto* midi=dynamic_cast<orbit::generator::MidiPlugin*>(p))midi->overdub.clear();
}
void publishRecordingOverdub(double reference,bool newTake=false)
{
    for(const auto& target:midiTake.targets)if(!target.song)
        if(auto* midi=generatorMidiPlugin(juce::String(target.id),true))
        {
            std::vector<orbit::recording::LoopNote> notes;
            for(const auto& n:midiTake.notes)if(n.id==target.id)notes.push_back({n.pitch,n.start,n.length,n.velocity,n.replayAfter});
            midi->overdub.publish(std::move(notes),target.lengthTicks,recordingLoopTicks,reference,newTake);
        }
}
std::vector<ProjectDocument::PatternNote> recordingNotesForPianoRoll()
{
    std::vector<ProjectDocument::PatternNote> result;
    const auto id=selectedProjectTrackId();const int slot=selectedPatternIndex();
    for(const auto& target:midiTake.targets)
        if(!target.song && target.id==id.toStdString() && target.slot==slot)
            for(const auto& note:midiTake.preview(target.id,midiTake.active?recordingTick():recordingEndTick))
            {ProjectDocument::PatternNote n;n.pitch=note.pitch;n.startTick=note.start;n.lengthTicks=note.length;n.velocity=note.velocity;result.push_back(n);}
    return result;
}
std::int64_t recordingTick(double receivedAtMs=0)const
{
    if(receivedAtMs<=0)receivedAtMs=juce::Time::getMillisecondCounterHiRes();
    return std::max<std::int64_t>(0,recordingInitialTick+std::llround((receivedAtMs-recordingStartMs)*recordingBpm*256.0/60000.0));
}
bool prepareMidiRecording(double positionSeconds,double nowMs)
{
    if(edit==nullptr || midiTake.active || !midiTake.notes.empty())return false;
    const bool song=playbackMode==PlaybackMode::song;
    const auto meter=project.getInitialTimeSignature();
    if(song && (meter.numerator!=4 || meter.denominator!=4 || loopButton.getToggleState()))
    {status.setText("Song MIDI recording currently needs 4/4 and Song looping off.",juce::dontSendNotification);return false;}
    std::vector<orbit::recording::Target> targets;
    bool anyArmed=false;for(int i=0;i<project.getTrackCount();++i)anyArmed|=getTrackRecordArm(i);
    const int selected=getGeneratorTracks().indexOf(instrumentTrack);
    const int startBar=juce::jlimit(0,PlaylistGrid::barCount-1,int(std::floor(positionSeconds*project.getInitialTempoBpm()/240.0)));
    for(int i=0;i<project.getTrackCount();++i)
    {
        if(project.getTrackKind(i)==ProjectDocument::TrackKind::audio || !(getTrackRecordArm(i) || (!anyArmed && i==selected)))continue;
        const auto id=project.getTrackId(i);int slot=project.getTrackSelectedPatternIndex(id);
        int length=project.getPatternLengthSteps(id,slot)*64;
        if(song)
        {
            if(project.getPlaylistPatternId(id,startBar).isNotEmpty())
            {status.setText("Move the song cursor to an empty region on every armed generator before recording.",juce::dontSendNotification);return false;}
            slot=-1;for(int s=0;s<ProjectDocument::patternSlotCount;++s)
                if(!project.isPatternCreated(id,s) && project.getPatternNotes(id,s).empty()){slot=s;break;}
            if(slot<0){status.setText("An armed generator needs a free pattern slot for its song take.",juce::dontSendNotification);return false;}
            int endBar=PlaylistGrid::barCount;
            for(const auto& clip:project.getPlaylistClips(id))if(clip.startBar>startBar)endBar=juce::jmin(endBar,clip.startBar);
            length=(endBar-startBar)*1024;
        }
        targets.push_back({id.toStdString(),slot,length,startBar,song});
    }
    if(targets.empty()){status.setText("Select or arm an instrument generator for MIDI recording. Audio recording is not yet available.",juce::dontSendNotification);return false;}
    releaseLiveMidi();
    recordingBpm=project.getInitialTempoBpm();recordingStartMs=nowMs;recordingEndTick=0;
    recordingInitialTick=std::llround(positionSeconds*recordingBpm*256.0/60.0)-(song?startBar*1024:0);
    recordingProjectId=project.getProjectId();
    const bool quantize=edit->state.getProperty("orbitInputQuantize",false);
    const int resolution=juce::jmax(1,int(edit->state.getProperty("orbitInputQuantizeResolution",16)));
    midiTake.start(std::move(targets),quantize?juce::jmax(1,juce::roundToInt(1024.0/resolution)):0,
                   bool(edit->state.getProperty("orbitInputQuantizeDurations",false)));
    recordingLoopTicks=patternPlaybackCycleTicks();
    if(!song && recordingLoopTicks==0)
    {midiTake.clear();status.setText("These pattern lengths need a common recording loop longer than 512 bars. Use compatible pattern lengths before recording.",juce::dontSendNotification);return false;}
    publishRecordingOverdub(recordingInitialTick,true);
    recordButton.setToggleState(true,juce::dontSendNotification);recordButton.repaint();return true;
}
juce::StringArray recordingTargets(int channel)
{
    juce::StringArray result;
    for(const auto& target:midiTake.targets)if(auto* track=engineTrackForProjectId(juce::String(target.id)))
    {const int assigned=track->state.getProperty("orbitMidiChannel",0);if(assigned==0 || assigned==channel)result.add(juce::String(target.id));}
    return result;
}
void captureRecordingEvent(const juce::MidiMessage& message,const juce::String& device,const juce::StringArray& targets,double receivedAtMs)
{
    if(receivedAtMs<=0)receivedAtMs=juce::Time::getMillisecondCounterHiRes();
    if(!midiTake.active || receivedAtMs<recordingStartMs)return;
    const auto before=midiTake.notes.size();
    const auto tick=recordingTick(receivedAtMs);
    if(message.isAllNotesOff() || message.isAllSoundOff()){finishMidiRecording(receivedAtMs);return;}
    for(const auto& id:targets)midiTake.event(id.toStdString(),device.toStdString(),message.getChannel(),
        message.isNoteOnOrOff()?message.getNoteNumber():0,message.isNoteOn()?message.getVelocity():0,
        message.isNoteOn(),message.isNoteOff(),message.isController() && message.getControllerNumber()==64,
        message.isController() && message.getControllerValue()>=64,tick);
    if(midiTake.notes.size()!=before)publishRecordingOverdub(tick);
}
bool finishMidiRecording(double nowMs=0)
{
    finishAutomationRecording();
    if(finishingMidiRecording)return true;
    const juce::ScopedValueSetter<bool> guard(finishingMidiRecording,true);
    clearRecordingOverdub();recordingCountInDisplay=-1;
    if(recordingUsesEnginePreroll && edit!=nullptr)
    {
        recordingUsesEnginePreroll=false;
        const auto now=nowMs>0?nowMs:juce::Time::getMillisecondCounterHiRes();
        if(now<recordingStartMs)
        {edit->getTransport().stop(true,false);edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(recordingPunchSeconds));}
        else edit->getTransport().stopRecording(true);
        edit->setClickTrackRange({});
    }
    if(midiTake.active)recordingEndTick=recordingTick(nowMs);
    const auto endTick=recordingEndTick;midiTake.finish(endTick);
    recordButton.setToggleState(false,juce::dontSendNotification);recordButton.repaint();
    if(midiTake.notes.empty()){midiTake.clear();return true;}
    const auto fail=[this]
    {status.setText("Recorded take retained in memory. Free a pattern slot/region, then press Record or Save to retry committing it.",juce::dontSendNotification);return false;};
    if(recordingProjectId!=project.getProjectId())return fail();
    auto staged=project;
    int count=0;
    for(const auto& target:midiTake.targets)
    {
        const auto id=juce::String(target.id);if(engineTrackForProjectId(id)==nullptr)return fail();
        std::vector<orbit::recording::Note> notes;
        for(const auto& n:midiTake.notes)if(n.id==target.id)notes.push_back(n);
        if(notes.empty())continue;
        int slot=target.slot;
        if(target.song && (staged.isPatternCreated(id,slot) || !staged.getPatternNotes(id,slot).empty()))
        {
            slot=-1;for(int s=0;s<ProjectDocument::patternSlotCount;++s)
                if(!staged.isPatternCreated(id,s) && staged.getPatternNotes(id,s).empty()){slot=s;break;}
            if(slot<0)return fail();
        }
        int lastTick=1;for(const auto& n:notes)lastTick=juce::jmax(lastTick,n.start+n.length);
        staged.setPatternCreated(id,slot,true);
        if(target.song)
        {
            const int extent=int(std::max<std::int64_t>(lastTick,std::min<std::int64_t>(endTick,target.lengthTicks)));
            const int bars=juce::jmax(1,(extent+1023)/1024);
            for(int bar=target.startBar;bar<target.startBar+bars;++bar)
                if(staged.getPlaylistPatternId(id,bar).isNotEmpty())return fail();
            staged.setPatternLengthSteps(id,slot,bars*16);
            staged.setPatternName(id,slot,"Recorded take");
            staged.setPlaylistClip(id,staged.getPatternId(id,slot),target.startBar,bars,true);
        }
        for(const auto& n:notes)
        {if(!staged.appendPatternNoteTicks(id,slot,n.pitch,n.start,n.length,n.velocity))return fail();++count;}
    }
    if(count==0){midiTake.clear();return true;}
    rememberUndo("Record MIDI take");project=std::move(staged);midiTake.clear();isDirty=true;
    readPatternSteps();refreshEnginePlaylist();if(playlistWindow!=nullptr)playlistWindow->refresh();
    refreshProjectView("Recorded "+juce::String(count)+" MIDI notes. Undo restores the previous patterns and arrangement.");return true;
}
void toggleMidiRecording()
{
    if(automationTakeActive || optionInt("recordContent")==1){toggleAutomationRecording();return;}
    if(midiTake.active || !midiTake.notes.empty()){finishMidiRecording();return;}
    if(edit==nullptr)return;
    if(engine.getDeviceManager().deviceManager.getCurrentAudioDevice()==nullptr)
    {status.setText("Choose an audio output before recording MIDI.",juce::dontSendNotification);return;}
    auto& transport=edit->getTransport();const bool wasPlaying=transport.isPlaying();
    double position=playbackMode==PlaybackMode::pattern && !wasPlaying?0.0:transport.getPosition().inSeconds();
    if(wasPlaying)if(auto* context=edit->getCurrentPlaybackContext())position=context->getPosition().inSeconds();
    if(!prepareMidiRecording(position,juce::Time::getMillisecondCounterHiRes()))return;
    if(wasPlaying && playbackMode==PlaybackMode::pattern)
    {
        // A newly armed silent pattern may lengthen the shared cycle. Update
        // the engine projection once, before any take notes are captured.
        const auto end=recordingLoopTicks/256.0*60.0/recordingBpm;
        transport.setLoopRange({tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(end)});
        refreshEnginePlaylist();
    }
    if(!wasPlaying)
    {
        togglePlayback(true);
        if(!transport.isPlaying())
        {clearRecordingOverdub();recordingUsesEnginePreroll=false;edit->setClickTrackRange({});midiTake.clear();recordButton.setToggleState(false,juce::dontSendNotification);return;}
        alignRecordingClockToTransport(position,juce::Time::getMillisecondCounterHiRes());
    }
    status.setText(recordingStartMs>juce::Time::getMillisecondCounterHiRes()?"Counting in. Notes played before recording starts are auditioned only.":"Recording MIDI into armed generators. Stop or press Record again to finish the take.",juce::dontSendNotification);
}

void alignRecordingClockToTransport(double punchSeconds,double nowMs)
{
    recordingPunchSeconds=punchSeconds;
    double current=edit!=nullptr?edit->getTransport().getPosition().inSeconds():punchSeconds;
    if(edit!=nullptr && edit->getTransport().isPlaying())
        if(auto* context=edit->getCurrentPlaybackContext())current=context->getPosition().inSeconds();
    recordingStartMs=nowMs+(punchSeconds-current)*1000.0;
}

void updateRecordingCountInStatus(double nowMs)
{
    if(!midiTake.active || !recordingUsesEnginePreroll)return;
    const int left=juce::jlimit(0,edit!=nullptr?edit->getNumCountInBeats():8,int(std::ceil((recordingStartMs-nowMs)*recordingBpm/60000.0)));
    if(left==recordingCountInDisplay)return;
    recordingCountInDisplay=left;
    status.setText(left>0?"Count-in: "+juce::String(left)+" quarter note(s).":
        "Recording MIDI. Completed notes repeat on later Pattern passes; Stop finishes the take.",juce::dontSendNotification);
}
