// Included inside ProjectView. Clipboard retains real pattern/audio placements,
// not the piano-roll note selection. Native media is shared non-destructively.
struct PlaylistClipboardEntry {int row=0;double start=0,length=1;juce::String pattern;juce::ValueTree audio;bool follows=true,exact=false;};
struct PlaylistClipboardData
{
    juce::String projectID;ProjectDocument source;std::vector<PlaylistClipboardEntry> entries;int firstRow=0;double firstBar=0;
};
std::optional<PlaylistClipboardData> playlistClipboard;
bool playlistEditActive=false;
bool playlistEditContext()const{return playlistEditActive && playlistWindow && playlistWindow->isVisible();}
void copyPlaylistClips(bool cut)
{
    if(!playlistWindow)return;const auto selected=playlistWindow->arrangementSelection();if(selected.empty())return;
    PlaylistClipboardData data;data.projectID=project.getProjectId();data.source=project;data.source.setTracktionEditState({});data.firstRow=project.getTrackCount();data.firstBar=PlaylistGrid::barCount;
    for(const auto& key:selected)
    {
        PlaylistClipboardEntry entry;entry.row=key.row;entry.start=key.startBar;
        if(isPlaylistAudioTrack(key.row))
        {
            auto* clip=audioClipForID(project.getTrackId(key.row),key.id);if(!clip)continue;
            const auto [a,b]=selectedAudioTimeRange(*clip,key);if(b-a<.001)continue;
            entry.start=a/playlistSecondsPerBar();entry.length=(b-a)/playlistSecondsPerBar();entry.audio=clip->state.createCopy();
            entry.audio.setProperty(te::IDs::start,a,nullptr);entry.audio.setProperty(te::IDs::length,b-a,nullptr);
            entry.audio.setProperty(te::IDs::offset,clip->getPosition().getOffset().inSeconds()+a-clip->getEditTimeRange().getStart().inSeconds(),nullptr);
            if(a>clip->getEditTimeRange().getStart().inSeconds()+1.e-8)entry.audio.setProperty(te::IDs::fadeIn,0.,nullptr);
            if(b<clip->getEditTimeRange().getEnd().inSeconds()-1.e-8)entry.audio.setProperty(te::IDs::fadeOut,0.,nullptr);
            if(entry.audio.hasProperty("orbitFrozenWasMuted")){entry.audio.setProperty(te::IDs::mute,entry.audio.getProperty("orbitFrozenWasMuted"),nullptr);entry.audio.removeProperty("orbitFrozenWasMuted",nullptr);}
        }
        else
        {const auto id=project.getTrackId(key.row);entry.pattern=key.id;entry.length=project.getPlaylistClipLengthBars(id,key.startBar);entry.follows=project.playlistClipFollowsPatternLength(id,key.startBar);for(const auto& clip:project.getPlaylistClips(id))if(clip.startBar==key.startBar && clip.patternId==key.id)entry.exact=clip.exactLength;}
        data.firstRow=juce::jmin(data.firstRow,key.row);data.firstBar=juce::jmin(data.firstBar,entry.start);data.entries.push_back(std::move(entry));
    }
    if(data.entries.empty())return;playlistClipboard=std::move(data);
    if(cut){if(deletePlaylistSelection(selected))playlistWindow->clearArrangementSelection();}
    status.setText("Copied "+juce::String(playlistClipboard->entries.size())+" Playlist clips. Click a destination, then Paste.",juce::dontSendNotification);
}
void pastePlaylistClips()
{
    if(!playlistClipboard || !playlistWindow || !edit)return;
    const auto& data=*playlistClipboard;
    const auto fail=[this](const juce::String& message){status.setText("Playlist paste canceled. "+message,juce::dontSendNotification);};
    if(data.projectID!=project.getProjectId()){fail("This clipboard belongs to another project.");return;}
    if(audioRecordingActive || midiTake.active || automationTakeActive || patternClipTake.active){fail("Finish recording first.");return;}
    const auto destination=playlistWindow->arrangementPastePosition();const int rowDelta=(destination.first>=0?destination.first:data.firstRow)-data.firstRow;
    const double delta=project.normalisePlaylistBar(destination.second)-data.firstBar,seconds=playlistSecondsPerBar();
    auto staged=project;const auto tracks=getGeneratorTracks();std::map<std::pair<juce::String,int>,juce::String> patterns;
    struct Paste{int row;double start;const PlaylistClipboardEntry* entry;juce::String pattern;};std::vector<Paste> pending;
    for(const auto& entry:data.entries)
    {
        const int row=entry.row+rowDelta;const double start=project.normalisePlaylistBar(entry.start+delta);
        if(!juce::isPositiveAndBelow(row,project.getTrackCount()) || row>=tracks.size() || start<0 || start+entry.length>PlaylistGrid::barCount){fail("The clips would leave the Playlist.");return;}
        const bool audio=entry.audio.isValid();if(audio!=isPlaylistAudioTrack(row)){fail("Audio and pattern clips need matching destination lanes.");return;}
        const auto dest=project.getTrackId(row);juce::String pattern=entry.pattern;
        if(audio)
        {
            for(auto* clip:tracks[row]->getClips())if(dynamic_cast<te::WaveAudioClip*>(clip))
            {const auto range=clip->getEditTimeRange();if(range.getStart().inSeconds()<(start+entry.length)*seconds-1.e-8 && range.getEnd().inSeconds()>start*seconds+1.e-8){fail("An audio destination is occupied.");return;}}
        }
        else
        {
            for(const auto& clip:staged.getPlaylistClips(dest))if(clip.startBar<start+entry.length-1.e-9 && clip.startBar+clip.lengthBars>start+1.e-9){fail("A pattern destination is occupied.");return;}
            const auto src=data.source.getTrackId(entry.row);const int sourceSlot=data.source.getPatternIndexForId(src,entry.pattern);
            if(src!=dest || project.getPatternIndexForId(dest,pattern)<0)
            {
                const auto key=std::make_pair(src+entry.pattern,row);auto found=patterns.find(key);
                if(found!=patterns.end())pattern=found->second;
                else
                {
                    int slot=-1;for(int i=0;i<ProjectDocument::patternSlotCount;++i)if(!staged.isPatternCreated(dest,i)){slot=i;break;}
                    if(slot<0 || !staged.copyPatternFrom(data.source,src,sourceSlot,dest,slot)){fail("No empty destination pattern slot is available.");return;}
                    pattern=staged.getPatternId(dest,slot);patterns[key]=pattern;
                }
            }
            if(entry.exact)staged.setPlaylistClipExactBars(dest,pattern,start,entry.length);
            else staged.setPlaylistClip(dest,pattern,start,entry.follows?0:entry.length,true);
        }
        pending.push_back({row,start,&entry,pattern});
    }
    // Reuse the existing associated-event transaction with the clipboard's
    // captured Song lanes, so later source edits don't rewrite copied events.
    const auto prefs=playlistWindow->playlistViewPreferences();std::map<int,std::vector<orbit::playlist::EventRange>> ranges;
    const auto meter=project.getInitialTimeSignature();const double steps=meter.numerator*16./juce::jmax(1,meter.denominator);
    for(const auto& entry:data.entries)ranges[entry.row].push_back({entry.start*steps,(entry.start+entry.length)*steps});
    juce::String error;if(!stagePlaylistSongEvents(staged,ranges,rowDelta,delta,true,bool(prefs.getProperty("generatorEvents",true)),bool(prefs.getProperty("mixerEvents",true)),error,&data.source)){fail(error);return;}
    auto before=captureProjectSnapshot();orbit::playlist::Selection selection;
    for(const auto& item:pending)
    {
        if(item.entry->audio.isValid())
        {
            auto state=item.entry->audio.createCopy();te::EditItemID::remapIDs(state,nullptr,*edit);
            auto* clip=dynamic_cast<te::WaveAudioClip*>(tracks[item.row]->insertClipWithState(state));
            if(!clip){restoreProjectSnapshot(before,"Audio paste failed; original arrangement restored.");return;}
            clip->setStart(tracktion::TimePosition::fromSeconds(item.start*seconds),false,true);selection.insert({item.row,item.start,clip->itemID.toString()});
        }
        else selection.insert({item.row,item.start,item.pattern});
    }
    rememberUndoSnapshot(std::move(before),"Paste Playlist clips");project=std::move(staged);isDirty=true;refreshEnginePlaylist();
    playlistWindow->setArrangementSelection(selection);double end=0;for(const auto& item:pending)end=juce::jmax(end,item.start+item.entry->length);playlistWindow->advanceArrangementInsertion(end);refreshProjectView("Pasted "+juce::String(pending.size())+" Playlist clips. One Undo restores the previous arrangement.");
}

// Pattern-mode snapshot: each source keeps its own remembered slot. This does
// not read the selected Song clips or change the running transport's mode.
std::vector<int> patternSnapshotRows(bool soloOnly=false)const
{
    std::vector<int> rows;if(playbackMode!=PlaybackMode::pattern || !edit)return rows;
    for(int row=0;row<project.getTrackCount();++row)
    {
        const auto id=project.getTrackId(row);if(isPlaylistAudioTrack(row))continue;
        auto* track=engineTrackForProjectId(id);if(!track || track->isMuted(true) || (soloOnly && !track->isSolo(true)))continue;
        const int slot=project.getTrackSelectedPatternIndex(id);
        if(soloOnly || patternHasNotes(id,slot) || project.getPatternAutomationLaneCount(id,slot)>0)rows.push_back(row);
    }
    return rows;
}
double patternSnapshotLength(const std::vector<int>& rows)const
{
    const auto m=project.getInitialTimeSignature();const double stepsPerBar=m.numerator*16./juce::jmax(1,m.denominator);
    const auto cap=juce::int64(std::round(24*stepsPerBar));juce::int64 common=1,longest=1;bool exceeds=false;
    for(int row:rows)
    {
        const auto id=project.getTrackId(row);const juce::int64 steps=juce::jmax(1,project.getPatternLengthSteps(id,project.getTrackSelectedPatternIndex(id)));
        longest=juce::jmax(longest,steps);
        if(!exceeds){const auto factor=common/std::gcd(common,steps);if(steps>cap || factor>cap/steps)exceeds=true;else common=factor*steps;}
    }
    return juce::jmin(24.,juce::jmax(1.,(exceeds?longest:common)/stepsPerBar));
}
void addPatternsToPlaylist(bool soloOnly=false)
{
    if(audioRecordingActive || midiTake.active || automationTakeActive || patternClipTake.active || freezeJob)return;
    const auto rows=patternSnapshotRows(soloOnly);if(rows.empty()){status.setText("Choose active generator patterns in Pattern mode first.",juce::dontSendNotification);return;}
    const double bar=playlistWindow?project.normalisePlaylistBar(playlistWindow->arrangementPastePosition().second):0.;
    const auto m=project.getInitialTimeSignature();const double stepsPerBar=m.numerator*16./juce::jmax(1,m.denominator);
    const double length=patternSnapshotLength(rows);
    if(bar+length>PlaylistGrid::barCount){status.setText("Pattern snapshot would extend past the Playlist. Choose an earlier Song position.",juce::dontSendNotification);return;}
    auto staged=project;orbit::playlist::Selection selected;
    for(const auto row:rows)
    {
        const auto id=project.getTrackId(row);const int slot=project.getTrackSelectedPatternIndex(id);const auto pattern=project.getPatternId(id,slot);
        for(const auto& clip:project.getPlaylistClips(id))if(clip.startBar<bar+length-1.e-9 && clip.startBar+clip.lengthBars>bar+1.e-9)
        {status.setText("Pattern snapshot canceled: a destination is occupied. Choose an empty Song position.",juce::dontSendNotification);return;}
        staged.setPatternCreated(id,slot,true); // Soloed empty selectors are valid placeholders.
        const double cycle=juce::jmax(1,project.getPatternLengthSteps(id,slot))/stepsPerBar;
        for(double at=0;at<length-1.e-9;at+=cycle)
        {
            const double span=juce::jmin(cycle,length-at),start=project.normalisePlaylistBar(bar+at);
            if(span<cycle-1.e-9 && cycle<=24.)break; // Keep the final incomplete cycle as a gap.
            if(m.numerator==4 && m.denominator==4 && std::abs(span-cycle)<1.e-9 && std::abs(cycle-std::ceil(cycle))<1.e-9)staged.setPlaylistClip(id,pattern,start,0,true);
            else staged.setPlaylistClipExactBars(id,pattern,start,span);
            selected.insert({row,start,pattern});
        }
    }
    rememberUndo(soloOnly?"Add Soloed Patterns to Playlist":"Add Patterns to Playlist");project=std::move(staged);isDirty=true;refreshEnginePlaylist();
    if(playlistWindow){playlistWindow->setArrangementSelection(selected);playlistWindow->advanceArrangementInsertion(bar+length);}
    refreshProjectView("Added "+juce::String(rows.size())+" generator patterns over "+juce::String(length,2)+" bars. Next capture starts at bar "+juce::String(bar+length+1,2)+" (24-bar maximum).");
}
