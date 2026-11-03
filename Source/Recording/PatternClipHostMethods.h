// Included inside ProjectView. Pattern performance is transient until one
// atomic document commit. MIDI clips are prepared ahead of their bar boundary,
// not launched from the GUI timer.
int recordContentMode(){return juce::jlimit(0,3,optionInt("recordContent",playbackMode==PlaybackMode::song?1:0));}
juce::String recordContentLabel(){return juce::StringArray{"MIDI","AUTO","CLIPS","AUDIO"}[recordContentMode()];}
bool hasArmedPatternTracks()const
{
    for(int row=0;row<project.getTrackCount();++row)
        if(project.getTrackKind(row)!=ProjectDocument::TrackKind::audio && getTrackRecordArm(row))return true;
    return false;
}
double patternClipPosition()const
{
    if(edit==nullptr)return 0;
    if(edit->getTransport().isPlaying())if(auto* context=edit->getCurrentPlaybackContext())return context->getPosition().inSeconds();
    return edit->getTransport().getPosition().inSeconds();
}
struct RuntimePatternClip
{
    juce::String trackId;int slot=0,startBar=0,lengthBars=0;te::MidiClip::Ptr clip;
};
void updatePatternClipPreview(double seconds)
{
    const int end=patternClipTake.active?patternClipTake.visibleEnd(seconds):patternClipTake.finishedBar;
    patternClipVisible=patternClipTake.segments(end);
    if(playlistWindow!=nullptr)playlistWindow->repaintGrid();
}
std::optional<orbit::recording::PatternClipTake::Segment> pendingPatternClip(const juce::String& id,double bar)const
{
    for(const auto& s:patternClipVisible)if(s.id==id.toStdString() && bar>=s.startBar && bar<s.startBar+s.lengthBars)return s;
    return {};
}
juce::String visiblePlaylistPatternId(const juce::String& id,double bar)const
{if(auto s=pendingPatternClip(id,bar))return project.getPatternId(id,s->slot);return project.getPlaylistPatternId(id,bar);}
double visiblePlaylistClipStart(const juce::String& id,double bar)const
{if(auto s=pendingPatternClip(id,bar))return s->startBar;return project.getPlaylistClipStartBar(id,bar);}
double visiblePlaylistClipLength(const juce::String& id,double bar)const
{if(auto s=pendingPatternClip(id,bar))return s->lengthBars;return project.getPlaylistClipLengthBars(id,bar);}
void syncPatternClipProjection()
{
    if(edit==nullptr || !patternClipTake.active)return;
    const auto segments=patternClipTake.segments(patternClipTake.endBar);
    patternClipProjection=project;
    for(const auto& s:segments)
    {
        const auto id=juce::String(s.id);patternClipProjection.setPatternCreated(id,s.slot,true);
        patternClipProjection.setPlaylistClipExactBars(id,project.getPatternId(id,s.slot),s.startBar,s.lengthBars);
    }
    for(auto it=patternClipRuntime.begin();it!=patternClipRuntime.end();)
    {
        const auto wanted=std::find_if(segments.begin(),segments.end(),[&](const auto& s){return s.id==it->trackId.toStdString() && s.startBar==it->startBar && s.slot==it->slot;});
        if(wanted==segments.end()){if(it->clip!=nullptr)it->clip->removeFromParent();it=patternClipRuntime.erase(it);}else ++it;
    }
    for(const auto& s:segments)
    {
        const auto id=juce::String(s.id);
        auto found=std::find_if(patternClipRuntime.begin(),patternClipRuntime.end(),[&](const auto& r){return r.trackId==id && r.startBar==s.startBar && r.slot==s.slot;});
        const auto end=tracktion::TimePosition::fromSeconds((s.startBar+s.lengthBars)*patternClipTake.secondsPerBar);
        if(found!=patternClipRuntime.end())
        {
            if(found->lengthBars!=s.lengthBars){found->clip->setEnd(end,false);found->lengthBars=s.lengthBars;}
            continue;
        }
        if(auto* track=engineTrackForProjectId(id))
        {
            auto clip=track->insertMIDIClip(project.getPatternName(id,s.slot),{tracktion::TimePosition::fromSeconds(s.startBar*patternClipTake.secondsPerBar),end},nullptr);
            if(clip==nullptr)continue;
            const int length=project.getPatternLengthSteps(id,s.slot);
            clip->setLoopRangeBeats({tracktion::BeatPosition::fromBeats(0),tracktion::BeatPosition::fromBeats(length*.25)});
            for(const auto& n:project.getPatternNotes(id,s.slot))clip->getSequence().addNote(n.pitch,tracktion::BeatPosition::fromBeats(n.startTick/256.),tracktion::BeatDuration::fromBeats(n.lengthTicks/256.),n.velocity,0,nullptr);
            addMidiAutomationToClip(*clip,id,s.slot,s.startBar,length);
            patternClipRuntime.push_back({id,s.slot,s.startBar,s.lengthBars,clip});
        }
    }
    updateEngineAutomation();
    updatePatternClipPreview(patternClipTake.lastSeconds);
}
bool preparePatternClipRecording(double seconds)
{
    if(edit==nullptr || playbackMode!=PlaybackMode::song || patternClipSnapshot || midiTake.active || !midiTake.notes.empty() || automationTakeActive || !std::isfinite(seconds))return false;
    const auto meter=project.getInitialTimeSignature();
    if(meter.numerator!=4 || meter.denominator!=4 || loopButton.getToggleState())
    {status.setText("Pattern Clips recording currently needs Song mode, 4/4 and Loop off.",juce::dontSendNotification);return false;}
    const double barSeconds=240./project.getInitialTempoBpm();
    const int start=juce::jlimit(0,PlaylistGrid::barCount,int(std::ceil(juce::jmax(0.,seconds)/barSeconds-1.e-8)));
    if(start>=PlaylistGrid::barCount){status.setText("Move the play cursor before the end of the Playlist.",juce::dontSendNotification);return false;}
    const bool armed=hasArmedPatternTracks();std::vector<orbit::recording::PatternClipTake::Target> targets;int end=PlaylistGrid::barCount;
    for(int row=0;row<project.getTrackCount();++row)
    {
        const auto id=project.getTrackId(row);
        if(isGeneratorFrozen(id) || project.getTrackKind(row)==ProjectDocument::TrackKind::audio || !(getTrackRecordArm(row) || (!armed && id==selectedProjectTrackId())))continue;
        if(project.getPlaylistPatternId(id,start).isNotEmpty())
        {status.setText("Choose an empty region on every armed generator before recording Pattern Clips.",juce::dontSendNotification);return false;}
        for(const auto& clip:project.getPlaylistClips(id))if(clip.startBar>start)end=juce::jmin(end,int(std::floor(clip.startBar)));
        targets.push_back({id.toStdString(),project.getTrackSelectedPatternIndex(id)});
    }
    if(end<=start){status.setText("Pattern Clips recording needs an empty whole bar before the next clip.",juce::dontSendNotification);return false;}
    if(targets.empty()){status.setText("Select or arm an instrument generator for Pattern Clips recording.",juce::dontSendNotification);return false;}
    releaseLiveMidi();recordingBpm=project.getInitialTempoBpm();recordingCountInDisplay=-1;patternClipVisibleEnd=-1;
    patternClipSnapshot=captureProjectSnapshot();patternClipProjectId=project.getProjectId();
    patternClipTake.start(std::move(targets),start,end,barSeconds,seconds);
    syncPatternClipProjection();recordButton.setToggleState(true,juce::dontSendNotification);recordButton.repaint();
    status.setText("Recording Pattern Clips. Pattern changes launch at the next bar; Stop commits the take.",juce::dontSendNotification);return true;
}
void disarmPatternClipTracks()
{
    for(const auto& t:patternClipTake.targets)if(auto* track=engineTrackForProjectId(juce::String(t.id)))track->state.setProperty("orbitRecordArmed",false,nullptr);
}
bool finishPatternClipRecording(std::optional<double> position={})
{
    if(!patternClipSnapshot)return true;
    const auto seconds=position.value_or(patternClipPosition());
    patternClipTake.finish(seconds);updatePatternClipPreview(seconds);
    if(recordingUsesEnginePreroll && !midiTake.active && edit!=nullptr)
    {
        recordingUsesEnginePreroll=false;
        if(seconds<patternClipTake.startBar*patternClipTake.secondsPerBar)
        {edit->getTransport().stop(true,false);edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(recordingPunchSeconds));}
        else edit->getTransport().stopRecording(true);
        edit->setClickTrackRange({});
    }
    recordButton.setToggleState(false,juce::dontSendNotification);recordButton.repaint();
    const auto segments=patternClipTake.segments(patternClipTake.finishedBar);
    const auto failed=[this]
    {
        patternClipRuntime.clear();refreshEnginePlaylist();
        status.setText("Pattern Clips take retained in memory. Clear the conflicting region, then press Record or Save to retry.",juce::dontSendNotification);return false;
    };
    if(patternClipProjectId!=project.getProjectId())return failed();
    auto staged=project;
    for(const auto& s:segments)
    {
        const auto id=juce::String(s.id);
        if(engineTrackForProjectId(id)==nullptr || staged.getPatternId(id,s.slot).isEmpty())return failed();
        for(const auto& clip:staged.getPlaylistClips(id))if(clip.startBar<s.startBar+s.lengthBars && clip.startBar+clip.lengthBars>s.startBar)return failed();
        staged.setPatternCreated(id,s.slot,true);
        if(!staged.setPlaylistClipExactBars(id,staged.getPatternId(id,s.slot),s.startBar,s.lengthBars))return failed();
    }
    // No clip is committed if stopped during count-in/before the first bar.
    if(!segments.empty())
    {rememberUndoSnapshot(std::move(*patternClipSnapshot),"Record pattern clips");project=std::move(staged);isDirty=true;}
    disarmPatternClipTracks();patternClipSnapshot.reset();patternClipTake.clear();patternClipRuntime.clear();patternClipVisible.clear();recordingCountInDisplay=-1;
    refreshEnginePlaylist();if(playlistWindow!=nullptr)playlistWindow->refresh();
    status.setText(segments.empty()?"Pattern Clips recording canceled before the first bar.":"Recorded Pattern Clips. Undo restores the previous arrangement.",juce::dontSendNotification);return true;
}
void advancePatternClipRecording(double seconds,bool playing)
{
    if(!patternClipTake.active)return;
    if(!playing){finishPatternClipRecording(seconds);return;}
    if(recordingUsesEnginePreroll && seconds<patternClipTake.startBar*patternClipTake.secondsPerBar)return;
    if(seconds<patternClipTake.lastSeconds-.05){finishPatternClipRecording(patternClipTake.lastSeconds);return;}
    patternClipTake.lastSeconds=seconds;
    if(seconds>=patternClipTake.endBar*patternClipTake.secondsPerBar){finishPatternClipRecording(patternClipTake.endBar*patternClipTake.secondsPerBar);return;}
    const int visible=patternClipTake.visibleEnd(seconds);
    if(visible!=patternClipVisibleEnd){patternClipVisibleEnd=visible;updatePatternClipPreview(seconds);}
}
bool recordPatternClipSelection(const juce::String& id,int slot)
{
    if(!patternClipTake.active || !patternClipTake.contains(id.toStdString()))return false;
    if(!patternClipTake.select(id.toStdString(),slot,patternClipPosition()))return false;
    project.setTrackSelectedPatternIndex(id,slot);syncPatternClipProjection();
    status.setText("Queued "+project.getPatternName(id,slot)+" for the next bar. Pattern Clips recording continues.",juce::dontSendNotification);return true;
}
void togglePatternClipRecording()
{
    if(patternClipSnapshot){finishPatternClipRecording();return;}
    if(edit==nullptr)return;
    if(playbackMode!=PlaybackMode::song){status.setText("Choose Song mode to record Pattern Clips.",juce::dontSendNotification);return;}
    if(engine.getDeviceManager().deviceManager.getCurrentAudioDevice()==nullptr)
    {status.setText("Choose an audio output before recording Pattern Clips.",juce::dontSendNotification);return;}
    const bool playing=edit->getTransport().isPlaying();
    if(!preparePatternClipRecording(patternClipPosition()))return;
    if(!playing)
    {
        togglePlayback(true);
        if(!edit->getTransport().isPlaying())finishPatternClipRecording(0.);
    }
}
