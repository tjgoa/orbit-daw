// Included inside ProjectView. Physical capture, disk streaming, count-in and
// latency compensation use Tracktion's native WaveInput recording path.
juce::StringArray availableAudioInputIDs()
{
    juce::StringArray ids;
    for(auto* input:engine.getDeviceManager().getWaveInputDevices())
        if(!input->isTrackDevice() && input->isEnabled())ids.add(input->getDeviceID());
    return ids;
}
void syncAudioRecordingInputs()
{
    if(edit==nullptr || audioRecordingActive)return;
    edit->getTransport().ensureContextAllocated();
    for(auto* instance:edit->getAllInputDevices())
        if(auto* wave=dynamic_cast<te::WaveInputDevice*>(&instance->getInputDevice());wave!=nullptr && !wave->isTrackDevice())
        {
            const auto deviceID=wave->getDeviceID();
            const auto oldTargets=instance->getTargets();
            for(const auto target:oldTargets)
            {
                instance->setRecordingEnabled(target,false);
                bool wanted=false;
                for(int row=0;row<project.getTrackCount();++row)
                    if(auto* track=engineTrackForProjectId(project.getTrackId(row));track!=nullptr && track->itemID==target)
                        wanted=project.getTrackKind(row)==ProjectDocument::TrackKind::audio && track->state.getProperty("orbitAudioInputId").toString()==deviceID;
                if(!wanted){auto result=instance->removeTarget(target,nullptr);juce::ignoreUnused(result);}
            }
            for(int row=0;row<project.getTrackCount();++row)
                if(project.getTrackKind(row)==ProjectDocument::TrackKind::audio)
                    if(auto* track=engineTrackForProjectId(project.getTrackId(row));track!=nullptr && track->state.getProperty("orbitAudioInputId").toString()==deviceID)
                        if(!instance->getTargets().contains(track->itemID))
                        {auto result=instance->setTarget(track->itemID,false,nullptr);juce::ignoreUnused(result);}
        }
}
void showAudioTrackRecordingOptions(const juce::String& trackID)
{
    if(!finishMidiRecording())return;
    auto* track=engineTrackForProjectId(trackID);if(track==nullptr)return;
    const auto ids=availableAudioInputIDs();juce::StringArray names{"None"};
    for(const auto& id:ids)if(auto* input=engine.getDeviceManager().findInputDeviceForID(id))names.add(input->getAlias());
    auto panel=std::make_unique<OptionsPanel>("Choose this track's mono input or stereo pair. Enable channels and choose mono/stereo in Options > Audio Input Settings first. Arm R, choose Song and AUDIO, then Record and Play. Input monitoring follows Audio Input Settings.");
    panel->choice("source","Recording input",names,ids.indexOf(track->state.getProperty("orbitAudioInputId").toString())+1);
    panel->button("folder","Open Recordings Folder").onClick=[this]{auto folder=audioRecordingRoot();if(folder.createDirectory().wasOk())folder.revealToUser();};
    panel->button("hardware","Input Device / Channels...").onClick=[this]{showHardwareSettings(true);};
    openOptionsTool("Audio Recording Input - "+track->getName(),std::move(panel),[this,trackID,ids](const auto& p)
    {
        if(!finishMidiRecording())return false;
        auto* target=engineTrackForProjectId(trackID);if(target==nullptr)return false;
        const int selected=p.index("source");const auto id=selected>0 && selected<=ids.size()?ids[selected-1]:juce::String{};
        if(id.isNotEmpty() && !availableAudioInputIDs().contains(id))
        {status.setText("That input is no longer available. Reopen Recording Input after configuring the device.",juce::dontSendNotification);return false;}
        rememberUndo("Assign audio recording input");target->state.setProperty("orbitAudioInputId",id,nullptr);syncAudioRecordingInputs();isDirty=true;
        status.setText("Audio input assigned. Arm R and use AUDIO in Song mode.",juce::dontSendNotification);return true;
    });
}
juce::File audioRecordingRoot()
{
    if(project.getFile()!=juce::File{})return project.getFile().getSiblingFile(project.getFile().getFileNameWithoutExtension()+" Audio").getChildFile("Recordings");
    return juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Orbit/Recordings").getChildFile(project.getProjectId());
}
bool writeAudioTakeManifest(const juce::String& state,const juce::String& error={})
{
    if(audioTakeFolder==juce::File{})return false;
    juce::var manifest(new juce::DynamicObject);auto* data=manifest.getDynamicObject();
    data->setProperty("state",state);data->setProperty("projectID",project.getProjectId());
    data->setProperty("projectFile",project.getFile()!=juce::File{}?project.getFile().getFullPathName():juce::String{});data->setProperty("startSeconds",audioRecordingStart);
    data->setProperty("limitSeconds",audioRecordingLimit);data->setProperty("error",error);
    juce::Array<juce::var> targets;
    for(const auto& id:audioTakeTrackIDs)
        if(auto* track=engineTrackForProjectId(id))
        {
            juce::var target(new juce::DynamicObject);auto* t=target.getDynamicObject();t->setProperty("trackID",id);t->setProperty("name",track->getName());
            t->setProperty("inputID",track->state.getProperty("orbitAudioInputId"));
            for(auto* input:edit->getAllInputDevices())if(input->getTargets().contains(track->itemID))
                if(const auto file=input->getRecordingFile(track->itemID);file!=juce::File{})t->setProperty("file",file.getFullPathName());
            targets.add(target);
        }
    data->setProperty("targets",targets);
    return audioTakeFolder.getChildFile("take.json").replaceWithText(juce::JSON::toString(manifest));
}
bool prepareAudioRecording(double seconds)
{
    if(edit==nullptr || audioRecordingActive || midiTake.active || automationTakeActive || patternClipSnapshot || !midiTake.notes.empty())return false;
    const auto fail=[this](const juce::String& message){status.setText(message,juce::dontSendNotification);return false;};
    const auto meter=project.getInitialTimeSignature();
    if(playbackMode!=PlaybackMode::song || loopButton.getToggleState() || meter.numerator!=4 || meter.denominator!=4)
        return fail("Audio recording currently needs Song mode, 4/4 and Loop off.");
    if(!optionBool("audioInput"))return fail("Enable audio input in Options > Audio Input Settings, then assign an input to the audio track.");
    auto* hardware=engine.getDeviceManager().deviceManager.getCurrentAudioDevice();
    if(hardware==nullptr || hardware->getActiveInputChannels().isZero())return fail("Choose an audio input device and enable its input channels first.");
    if(!std::isfinite(seconds) || seconds<0 || seconds>=PlaylistGrid::barCount*playlistSecondsPerBar())return fail("Move the cursor inside the Playlist before recording.");
    const auto available=availableAudioInputIDs();
    bool anyAudioArmed=false;
    for(int row=0;row<project.getTrackCount();++row)if(project.getTrackKind(row)==ProjectDocument::TrackKind::audio && getTrackRecordArm(row))anyAudioArmed=true;
    audioTakeTrackIDs.clear();audioRecordingStart=seconds;audioRecordingLimit=PlaylistGrid::barCount*playlistSecondsPerBar();
    for(int row=0;row<project.getTrackCount();++row)
        if(project.getTrackKind(row)==ProjectDocument::TrackKind::audio && (getTrackRecordArm(row) || (!anyAudioArmed && engineTrackForProjectId(project.getTrackId(row))==instrumentTrack)))
        {
            auto* track=engineTrackForProjectId(project.getTrackId(row));if(track==nullptr)return fail("Audio track is unavailable.");
            if(!available.contains(track->state.getProperty("orbitAudioInputId").toString()))return fail("Assign an available input using the audio track's Recording Input menu.");
            for(auto* clip:track->getClips())
            {
                const auto range=clip->getEditTimeRange();
                if(range.getStart().inSeconds()<=seconds+1.e-9 && range.getEnd().inSeconds()>seconds+1.e-9)return fail("Move the cursor to an empty region on every armed audio track.");
                if(range.getStart().inSeconds()>seconds)audioRecordingLimit=juce::jmin(audioRecordingLimit,range.getStart().inSeconds());
            }
            audioTakeTrackIDs.push_back(project.getTrackId(row));
        }
    if(audioTakeTrackIDs.empty())return fail("Add/select an audio track and assign its Recording Input, or arm audio tracks with R.");
    if(audioRecordingLimit-seconds<.02)return fail("There is not enough empty space before the next clip.");
    // A durable location is created even for an untitled project. Undo never
    // deletes recorded media, so Redo and previously saved versions remain safe.
    audioTakeFolder=audioRecordingRoot().getChildFile("Take-"+juce::Uuid().toString());
    auto result=audioTakeFolder.createDirectory();if(result.failed())return fail(result.getErrorMessage());
    auto recovery=project;edit->flushState();orbit::project::capturePluginCatalog(*edit);recovery.setTracktionEditState(edit->state);juce::String error;
    if(!recovery.saveToFile(audioTakeFolder.getChildFile("before-take.orbit"),error))return fail("Could not save the take recovery snapshot: "+error);
    if(!writeAudioTakeManifest("preparing"))return fail("Could not save the audio take manifest.");
    syncAudioRecordingInputs();
    audioTakeOldClipIDs.clear();
    for(auto* track:getGeneratorTracks())for(auto* clip:track->getClips())audioTakeOldClipIDs.insert(clip->itemID.toString());
    bool armed=false;
    for(auto* instance:edit->getAllInputDevices())
    {
        // Recording content modes are exclusive; do not capture a stale MIDI
        // or hardware destination in the engine while making this audio take.
        for(const auto target:instance->getTargets())instance->setRecordingEnabled(target,false);
        if(auto* wave=dynamic_cast<te::WaveInputDevice*>(&instance->getInputDevice());wave!=nullptr && !wave->isTrackDevice())
        {
            wave->setOutputFormat(engine.getAudioFileFormatManager().getWavFormat()->getFormatName());
            wave->setMergeMode(te::WaveInputDevice::getMergeModes()[0]);wave->setRecordTriggerDb(-60);
            // UUID per track avoids identical names mapping two inputs to one file.
            for(const auto& id:audioTakeTrackIDs)if(auto* track=engineTrackForProjectId(id))
                if(track->state.getProperty("orbitAudioInputId").toString()==wave->getDeviceID() && instance->getTargets().contains(track->itemID))
                {wave->setFilenameMask(audioTakeFolder.getChildFile("%track%_%take%_").getFullPathName());instance->setRecordingEnabled(track->itemID,true);armed=true;}
        }
    }
    if(!armed)return fail("Audio input routing could not be prepared. Reopen Recording Input and assign an available channel.");
    edit->recordingPunchInOut=false;audioRecordingActive=true;return true;
}
void toggleAudioRecording()
{
    if(audioRecordingActive){finishAudioRecording();return;}
    if(edit==nullptr)return;
    const bool wasPlaying=edit->getTransport().isPlaying();
    if(!prepareAudioRecording(patternClipPosition()))return;
    auto& transport=edit->getTransport();
    const std::array<te::Edit::CountIn,5> modes{te::Edit::CountIn::none,te::Edit::CountIn::oneBeat,te::Edit::CountIn::twoBeat,te::Edit::CountIn::oneBar,te::Edit::CountIn::twoBar};
    edit->setCountInMode(modes[size_t(juce::jlimit(0,4,optionInt("recordCountIn")))]);
    transport.looping=false;transport.record(false,false);
    bool capturing=false;
    for(auto* input:edit->getAllInputDevices())capturing|=input->isRecording();
    if(!transport.isRecording() || !capturing)
    {finishAudioRecording();status.setText("Could not start audio recording. Check the input device and recording folder.",juce::dontSendNotification);return;}
    if(!writeAudioTakeManifest("recording")){finishAudioRecording();status.setText("Recording stopped because its take manifest could not be saved.",juce::dontSendNotification);return;}
    recordingPunchSeconds=audioRecordingStart;recordingBpm=project.getInitialTempoBpm();recordingCountInDisplay=-1;
    alignRecordingClockToTransport(audioRecordingStart,juce::Time::getMillisecondCounterHiRes());
    recordButton.setToggleState(true,juce::dontSendNotification);playButton.setButtonText("Stop");
    if(playlistWindow!=nullptr)playlistWindow->refresh();
    status.setText(!wasPlaying && optionInt("recordCountIn")>0?"Counting in for audio recording. Stop cancels before the take begins.":"Recording audio inputs to WAV. Stop or Record finishes one take; Undo removes its clips.",juce::dontSendNotification);
}
bool finishAudioRecording(const juce::String& reason={})
{
    if(!audioRecordingActive || finishingAudioRecording)return true;
    const juce::ScopedValueSetter<bool> guard(finishingAudioRecording,true);
    // A device/error stop may already have committed clips. Remove only those
    // new audio clips from the copied Undo state; keep live mixer/parameter
    // changes made during the take. Never mutate the playing Edit here.
    auto before=captureProjectSnapshot();
    std::function<void(juce::ValueTree)> withoutTake=[&](juce::ValueTree node)
    {
        for(int i=node.getNumChildren();--i>=0;)
        {
            auto child=node.getChild(i);
            if(child.hasType("AUDIOCLIP") && !audioTakeOldClipIDs.contains(child.getProperty("id").toString()))node.removeChild(i,nullptr);
            else withoutTake(child);
        }
    };
    withoutTake(before.editState);before.document.setTracktionEditState(before.editState);
    juce::String error=reason;int clips=0;
    const bool precount=patternClipPosition()<audioRecordingStart;
    const auto folder=audioTakeFolder;
    if(edit!=nullptr)
    {
        if(auto* context=edit->getCurrentPlaybackContext())
        {
            const auto end=tracktion::TimePosition::fromSeconds(juce::jmin(audioRecordingLimit,context->getUnloopedPosition().inSeconds()));
            // Native stop freezes every writer before flushing disk queues. It
            // returns the newly created clips and any disk/commit error.
            auto result=context->stopRecording(end,false);
            if(!result)error=result.error();
        }
        for(auto* track:getGeneratorTracks())for(auto* clip:track->getClips())
            if(!audioTakeOldClipIDs.contains(clip->itemID.toString()))++clips;
        edit->getTransport().stopRecording(true); // Writers are already closed.
        if(precount){edit->getTransport().stop(false,false);edit->getTransport().setPosition(tracktion::TimePosition::fromSeconds(audioRecordingStart));}
        edit->setClickTrackRange({});
    }
    audioRecordingActive=false;syncAudioRecordingInputs();
    recordButton.setToggleState(false,juce::dontSendNotification);
    if(clips>0)
    {
        rememberUndoSnapshot(std::move(before),"Record audio take");isDirty=true;
        // Save the committed state beside the WAVs before the next project Save.
        auto recovery=project;edit->flushState();orbit::project::capturePluginCatalog(*edit);recovery.setTracktionEditState(edit->state);juce::String saveError;
        if(!recovery.saveToFile(folder.getChildFile("recorded-take.orbit"),saveError))error+=" Recovery snapshot: "+saveError;
    }
    // Keep the original file mapping recorded at start; add outcome separately.
    auto manifest=juce::JSON::parse(folder.getChildFile("take.json"));
    if(auto* data=manifest.getDynamicObject())
    {data->setProperty("state",error.isNotEmpty()?"needs-review":clips>0?"committed":"cancelled");data->setProperty("clips",clips);data->setProperty("error",error);if(!folder.getChildFile("take.json").replaceWithText(juce::JSON::toString(manifest)))error+=" Could not update the take manifest.";}
    audioTakeTrackIDs.clear();audioTakeOldClipIDs.clear();
    if(playlistWindow!=nullptr)playlistWindow->refresh();
    if(!edit->getTransport().isPlaying())playButton.setButtonText("Play");
    if(error.isNotEmpty())status.setText("Audio take files retained at "+folder.getFullPathName()+". "+error,juce::dontSendNotification);
    else status.setText(clips>0?"Recorded "+juce::String(clips)+" audio clip(s). Undo removes the take; WAV files are retained.":"Audio recording cancelled before a usable take. No clips added.",juce::dontSendNotification);
    return error.isEmpty();
}
void updateAudioRecording()
{
    if(!audioRecordingActive || edit==nullptr)return;
    bool missing=false;
    const auto ids=availableAudioInputIDs();
    for(const auto& id:audioTakeTrackIDs)if(auto* track=engineTrackForProjectId(id))missing|=!ids.contains(track->state.getProperty("orbitAudioInputId").toString());else missing=true;
    const auto position=patternClipPosition();
    if(playlistWindow!=nullptr)playlistWindow->repaintGrid();
    if(missing || !edit->getTransport().isRecording() || position>=audioRecordingLimit)
    {
        finishAudioRecording(missing?"Audio input disconnected or changed.":juce::String{});edit->getTransport().stop(false,false);playButton.setButtonText("Play");
        if(missing)status.setText("Audio input disconnected or changed. Available take files were retained.",juce::dontSendNotification);
    }
    else
    {
        const int left=juce::jmax(0,int(std::ceil((audioRecordingStart-position)*recordingBpm/60.0)));
        if(left!=recordingCountInDisplay)
        {
            recordingCountInDisplay=left;
            status.setText(left>0?"Audio count-in: "+juce::String(left)+" quarter note(s).":
                "Recording audio inputs to WAV. Stop or Record finishes one take; Undo removes its clips.",juce::dontSendNotification);
        }
    }
}

std::vector<orbit::playlist::SelectionItem> pendingAudioRecordingItems()const
{
    std::vector<orbit::playlist::SelectionItem> items;
    if(!audioRecordingActive)return items;
    const auto position=juce::jmin(audioRecordingLimit,patternClipPosition());
    if(position<audioRecordingStart)return items;
    const double barSeconds=playlistSecondsPerBar();
    for(int row=0;row<project.getTrackCount();++row)
        if(std::find(audioTakeTrackIDs.begin(),audioTakeTrackIDs.end(),project.getTrackId(row))!=audioTakeTrackIDs.end())
            items.push_back({{row,audioRecordingStart/barSeconds,"Recording audio"},juce::jmax(.03,(position-audioRecordingStart)/barSeconds)});
    return items;
}

void paintLiveAudioRecording(juce::Graphics& g,int row,juce::Rectangle<int> bounds,double seconds)
{
    if(!audioRecordingActive || !edit || seconds<=0)return;
    auto* track=engineTrackForProjectId(project.getTrackId(row));if(!track)return;
    for(auto* input:edit->getAllInputDevices())if(input->isRecording() && input->getTargets().contains(track->itemID))
    {
        const auto file=input->getRecordingFile(track->itemID);if(file==juce::File{})continue;
        auto thumbnail=engine.getRecordingThumbnailManager().getThumbnailFor(file);
        if(thumbnail && thumbnail->thumb->getTotalLength()>0)
        {g.setColour(juce::Colours::white.withAlpha(.92f));thumbnail->thumb->drawChannels(g,bounds,0,seconds,.95f);}
    }
}
