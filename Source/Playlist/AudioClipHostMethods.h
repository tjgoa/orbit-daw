// Included inside ProjectView. Audio operations resolve stable IDs at commit.
struct AudioNormalizeJob
{
    juce::String projectID,trackID,clipID;juce::File file;
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> block;
    juce::int64 first=0,position=0,last=0;
    double start=0,end=0,offset=0,speed=1;float gain=0,peak=0;bool reversed=false;
};
struct AudioEditPump final:juce::Timer
{
    std::function<void()> action;
    void timerCallback()override{const auto callback=action;if(callback)callback();}
};
std::unique_ptr<AudioNormalizeJob> audioNormalizeJob;
std::unique_ptr<AudioEditPump> audioNormalizePump;
te::WaveAudioClip* audioClipForID(const juce::String& id,const juce::String& clipID)const
{
    if(auto* track=engineTrackForProjectId(id))for(auto* clip:track->getClips())
        if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip);wave && wave->itemID.toString()==clipID)return wave;
    return nullptr;
}
bool isPlaylistAudioTrack(int row)const
{
    return juce::isPositiveAndBelow(row,project.getTrackCount())
        && (project.getTrackKind(row)==ProjectDocument::TrackKind::audio || isGeneratorFrozen(project.getTrackId(row)));
}
std::pair<double,double> audioDisplayRange(te::WaveAudioClip& clip)
{
    const double offset=clip.getPosition().getOffset().inSeconds()*clip.getSpeedRatio(),length=clip.getEditTimeRange().getLength().inSeconds()*clip.getSpeedRatio();
    if(!clip.getIsReversed())return {offset,offset+length};
    const auto total=te::AudioFile(engine,clip.getOriginalFile()).getLength();return {total-offset-length,total-offset};
}
void cancelAudioNormalization(){audioNormalizePump.reset();audioNormalizeJob.reset();}
void processAudioNormalization()
{
    if(!audioNormalizeJob)return;auto& job=*audioNormalizeJob;
    const auto deadline=juce::Time::getMillisecondCounterHiRes()+15.;
    while(job.position<job.last && juce::Time::getMillisecondCounterHiRes()<deadline)
    {
        const auto count=int(juce::jmin<juce::int64>(32768,job.last-job.position));
        if(!job.reader->read(&job.block,0,count,job.position,true,true))
        {cancelAudioNormalization();status.setText("Could not read this audio file. Normalization was not applied.",juce::dontSendNotification);return;}
        const auto peak=job.block.getMagnitude(0,count);
        if(!std::isfinite(peak)){cancelAudioNormalization();status.setText("The audio contains invalid samples. Normalization was not applied.",juce::dontSendNotification);return;}
        job.peak=juce::jmax(job.peak,peak);job.position+=count;
    }
    if(job.position<job.last)return;
    auto* clip=audioClipForID(job.trackID,job.clipID);
    const bool unchanged=project.getProjectId()==job.projectID && clip && clip->getOriginalFile()==job.file
        && clip->getEditTimeRange().getStart().inSeconds()==job.start && clip->getEditTimeRange().getEnd().inSeconds()==job.end
        && clip->getPosition().getOffset().inSeconds()==job.offset && clip->getSpeedRatio()==job.speed
        && clip->getIsReversed()==job.reversed && clip->getGainDB()==job.gain;
    if(!unchanged || audioRecordingActive || midiTake.active || automationTakeActive || patternClipTake.active)
    {cancelAudioNormalization();status.setText("The clip or recording state changed. Normalization was canceled.",juce::dontSendNotification);return;}
    if(job.peak<=1.e-9f){cancelAudioNormalization();status.setText("This audio selection is silent; no normalization gain was applied.",juce::dontSendNotification);return;}
    const auto gain=juce::Decibels::gainToDecibels(juce::Decibels::decibelsToGain(-.1f)/job.peak);
    if(std::abs(gain-job.gain)>.0001f)
    {rememberUndo("Normalize audio clip");// The generic native setter caps gain at +24 dB. Normalization of
        // quiet recordings needs the full measured gain; LiveClipLevel follows
        // this same state property and persists it without altering the file.
        clip->state.setProperty(te::IDs::gain,gain,nullptr);isDirty=true;}
    cancelAudioNormalization();if(playlistWindow)playlistWindow->refresh();
    status.setText("Normalized the clip's audio selection to -0.1 dBFS using non-destructive clip gain. Undo restores its previous level.",juce::dontSendNotification);
}
void normalizeAudioClip(te::WaveAudioClip& clip)
{
    if(audioRecordingActive || midiTake.active || automationTakeActive || patternClipTake.active || freezeJob)
    {status.setText("Finish recording or rendering before normalizing audio.",juce::dontSendNotification);return;}
    cancelAudioNormalization();auto job=std::make_unique<AudioNormalizeJob>();job->file=clip.getOriginalFile();
    job->reader.reset(engine.getAudioFileFormatManager().readFormatManager.createReaderFor(job->file));
    if(!job->reader || job->reader->sampleRate<=0){status.setText("The audio file could not be opened for normalization.",juce::dontSendNotification);return;}
    for(int row=0;row<project.getTrackCount();++row)if(engineTrackForProjectId(project.getTrackId(row))==clip.getTrack())job->trackID=project.getTrackId(row);
    job->projectID=project.getProjectId();job->clipID=clip.itemID.toString();job->start=clip.getEditTimeRange().getStart().inSeconds();job->end=clip.getEditTimeRange().getEnd().inSeconds();
    job->offset=clip.getPosition().getOffset().inSeconds();job->speed=clip.getSpeedRatio();job->gain=clip.getGainDB();job->reversed=clip.getIsReversed();
    const auto range=audioDisplayRange(clip);const auto samples=job->reader->lengthInSamples;
    job->first=juce::jlimit<juce::int64>(0,samples,juce::int64(std::floor(range.first*job->reader->sampleRate)));job->position=job->first;
    job->last=juce::jlimit<juce::int64>(job->first,samples,juce::int64(std::ceil(range.second*job->reader->sampleRate)));
    job->block.setSize(int(job->reader->numChannels),32768);audioNormalizeJob=std::move(job);
    audioNormalizePump=std::make_unique<AudioEditPump>();const juce::Component::SafePointer<ProjectView> owner(this);
    audioNormalizePump->action=[owner]{if(owner)owner->processAudioNormalization();};audioNormalizePump->startTimer(1);
    status.setText("Measuring the selected audio for normalization...",juce::dontSendNotification);
}
void splitPlaylistAudioClip(const juce::String& trackID,const juce::String& clipID,double bar)
{
    auto* wave=audioClipForID(trackID,clipID);if(!wave || audioRecordingActive)return;
    const auto time=tracktion::TimePosition::fromSeconds(bar*playlistSecondsPerBar());const auto range=wave->getEditTimeRange();
    if(time<=range.getStart() || time>=range.getEnd())return;
    rememberUndo("Split audio clip");if(auto* track=dynamic_cast<te::ClipTrack*>(wave->getTrack()))track->splitClip(*wave,time);
    isDirty=true;if(playlistWindow)playlistWindow->refresh();
}
void showAudioClipProperties(const juce::String& trackID,const juce::String& clipID)
{
    auto* wave=audioClipForID(trackID,clipID);if(!wave)return;
    auto panel=std::make_unique<OptionsPanel>("Edit this audio placement non-destructively. Unfreeze restores the original generator arrangement; Undo restores your audio edits.");
    panel->number("start","Start (seconds)",wave->getEditTimeRange().getStart().inSeconds(),0,PlaylistGrid::barCount*playlistSecondsPerBar(),.001);
    panel->number("length","Length (seconds)",wave->getEditTimeRange().getLength().inSeconds(),.001,PlaylistGrid::barCount*playlistSecondsPerBar(),.001);
    panel->number("offset","Source offset (seconds)",wave->getPosition().getOffset().inSeconds(),0,PlaylistGrid::barCount*playlistSecondsPerBar(),.001);
    panel->number("gain","Clip gain (dB)",wave->getGainDB(),-100,180,.1);
    const auto projectID=project.getProjectId();
    openOptionsTool("Audio Clip",std::move(panel),[this,trackID,clipID,projectID](const auto& form)
    {
        auto* clip=audioClipForID(trackID,clipID);if(!clip || project.getProjectId()!=projectID || audioRecordingActive)return false;
        const auto start=form.value("start"),length=form.value("length"),offset=form.value("offset");
        const auto source=te::AudioFile(engine,clip->getOriginalFile()).getLength();
        if(start+length>PlaylistGrid::barCount*playlistSecondsPerBar() || (offset+length)*clip->getSpeedRatio()>source+.001)
        {status.setText("The placement or source excerpt extends past the available audio.",juce::dontSendNotification);return false;}
        if(auto* owner=dynamic_cast<te::ClipTrack*>(clip->getTrack()))for(auto* other:owner->getClips())if(other!=clip && dynamic_cast<te::WaveAudioClip*>(other))
        {const auto range=other->getEditTimeRange();if(range.getStart().inSeconds()<start+length-1.e-8 && range.getEnd().inSeconds()>start+1.e-8){status.setText("The destination overlaps another audio clip.",juce::dontSendNotification);return false;}}
        rememberUndo("Edit audio placement");clip->setPosition({{tracktion::TimePosition::fromSeconds(start),tracktion::TimePosition::fromSeconds(start+length)},tracktion::TimeDuration::fromSeconds(offset)});
        clip->state.setProperty(te::IDs::gain,float(form.value("gain")),nullptr);isDirty=true;if(playlistWindow)playlistWindow->refresh();return true;
    },true);
}

std::pair<double,double> selectedAudioTimeRange(te::WaveAudioClip& clip,const orbit::playlist::ClipKey& key)const
{
    const auto range=clip.getEditTimeRange();double a=range.getStart().inSeconds(),b=range.getEnd().inSeconds();
    if(key.rangeStart>=0 && key.rangeEnd>key.rangeStart){a=juce::jmax(a,key.rangeStart*playlistSecondsPerBar());b=juce::jmin(b,key.rangeEnd*playlistSecondsPerBar());}
    return {a,b};
}
bool retainAudioOutsideSelection(te::WaveAudioClip& clip,double a,double b)
{
    const auto range=clip.getEditTimeRange();const double start=range.getStart().inSeconds(),end=range.getEnd().inSeconds();
    const bool left=a>start+1.e-8,right=b<end-1.e-8;
    if(left && right)
    {
        auto duplicate=te::duplicateClip(clip);auto* tail=dynamic_cast<te::WaveAudioClip*>(duplicate.get());if(!tail)return false;
        tail->setStart(tracktion::TimePosition::fromSeconds(b),true,false);tail->setFadeIn({});clip.setEnd(tracktion::TimePosition::fromSeconds(a),true);clip.setFadeOut({});
    }
    else if(left){clip.setEnd(tracktion::TimePosition::fromSeconds(a),true);clip.setFadeOut({});}
    else if(right){clip.setStart(tracktion::TimePosition::fromSeconds(b),true,false);clip.setFadeIn({});}
    else clip.removeFromParent();
    return true;
}
std::pair<double,double> playlistAudioTrimLimits(const orbit::playlist::ClipKey& key)
{
    auto* clip=audioClipForID(project.getTrackId(key.row),key.id);if(!clip)return {key.startBar,key.startBar};
    const auto pos=clip->getPosition();const auto speed=juce::jmax(.0001,clip->getSpeedRatio());
    const double start=pos.time.getStart().inSeconds(),offset=pos.getOffset().inSeconds();
    const auto source=te::AudioFile(engine,clip->getOriginalFile()).getLength();
    return {juce::jmax(0.,start-offset)/playlistSecondsPerBar(),juce::jmin(PlaylistGrid::barCount*playlistSecondsPerBar(),start-offset+source/speed)/playlistSecondsPerBar()};
}
bool trimPlaylistAudioClip(const orbit::playlist::ClipKey& key,double startBar,double endBar)
{
    if(!juce::isPositiveAndBelow(key.row,project.getTrackCount()) || audioRecordingActive || midiTake.active || automationTakeActive || patternClipTake.active || freezeJob)return false;
    auto* clip=audioClipForID(project.getTrackId(key.row),key.id);if(!clip || !std::isfinite(startBar) || !std::isfinite(endBar))return false;
    const auto limits=playlistAudioTrimLimits(key);const double seconds=playlistSecondsPerBar(),a=juce::jmax(startBar,limits.first)*seconds,b=juce::jmin(endBar,limits.second)*seconds;
    if(b-a<.001)return false;const auto old=clip->getPosition();
    if(std::abs(old.time.getStart().inSeconds()/seconds-key.startBar)>1.e-8)return false;
    if(auto* track=dynamic_cast<te::ClipTrack*>(clip->getTrack()))for(auto* other:track->getClips())if(other!=clip && dynamic_cast<te::WaveAudioClip*>(other))
    {const auto r=other->getEditTimeRange();if(r.getStart().inSeconds()<b-1.e-8 && r.getEnd().inSeconds()>a+1.e-8){status.setText("Audio trim canceled: another clip occupies the extension.",juce::dontSendNotification);return false;}}
    if(std::abs(a-old.time.getStart().inSeconds())<1.e-8 && std::abs(b-old.time.getEnd().inSeconds())<1.e-8)return true;
    rememberUndo("Trim audio clip");clip->setPosition({{tracktion::TimePosition::fromSeconds(a),tracktion::TimePosition::fromSeconds(b)},old.getOffset()+tracktion::TimeDuration::fromSeconds(a-old.time.getStart().inSeconds())});
    isDirty=true;if(playlistWindow){playlistWindow->setArrangementSelection({{key.row,a/seconds,key.id}});playlistWindow->refresh();}
    status.setText("Trimmed audio non-destructively. Drag the edge back out to restore its tail.",juce::dontSendNotification);return true;
}
