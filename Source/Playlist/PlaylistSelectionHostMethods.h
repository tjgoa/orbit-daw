// Included inside ProjectView. Validate all destinations against a staged
// document before changing the engine; one snapshot commits the whole gesture.
double playlistSecondsPerBar() const
{
    const auto meter=project.getInitialTimeSignature();
    return 60.0/juce::jmax(20.0,project.getInitialTempoBpm())*meter.numerator*4.0/juce::jmax(1,meter.denominator);
}

std::vector<orbit::playlist::SelectionItem> selectablePlaylistItems() const
{
    std::vector<orbit::playlist::SelectionItem> items;if(edit==nullptr)return items;
    const auto tracks=getGeneratorTracks();const double barSeconds=playlistSecondsPerBar();
    for(int row=0;row<project.getTrackCount();++row)
    {
        if(!isPlaylistAudioTrack(row))
        {
            const auto id=project.getTrackId(row);
            for(const auto& clip:project.getPlaylistClips(id))items.push_back({{row,clip.startBar,clip.patternId},clip.lengthBars});
            for(const auto& clip:patternClipVisible)if(clip.id==id.toStdString())items.push_back({{row,double(clip.startBar),project.getPatternId(id,clip.slot)},double(clip.lengthBars)});
        }
        else if(row<tracks.size())
            for(auto* clip:tracks[row]->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip))
            {
                const auto range=wave->getEditTimeRange();const double start=range.getStart().inSeconds()/barSeconds;
                const double end=range.getEnd().inSeconds()/barSeconds;
                items.push_back({{row,start,wave->itemID.toString()},end-start});
            }
    }
    return items;
}

std::optional<orbit::playlist::Selection> transformPlaylistSelection(const orbit::playlist::Selection& selected,int rows,double bars,bool copy,bool erase=false)
{
    using Selection=orbit::playlist::Selection;using Key=orbit::playlist::ClipKey;
    auto fail=[this](const juce::String& why)->std::optional<Selection>{status.setText("Playlist edit canceled. "+why,juce::dontSendNotification);return {};};
    if(edit==nullptr || !hasProject || selected.empty() || !std::isfinite(bars))return {};
    const bool includesPatterns=std::any_of(selected.begin(),selected.end(),[this](const auto& key){return juce::isPositiveAndBelow(key.row,project.getTrackCount()) && !isPlaylistAudioTrack(key.row);});
    if(includesPatterns)bars=project.normalisePlaylistBar(bars);
    if(audioRecordingActive || midiTake.active || !midiTake.notes.empty() || automationTakeActive || !patternClipTake.launches.empty())return fail("Finish recording before editing the arrangement.");
    if(!erase && rows==0 && bars==0)return selected;
    struct Item
    {
        Key source;int row=0,slot=-1;double start=0,length=1;bool audio=false,exact=false,follows=false;
        te::WaveAudioClip* wave=nullptr;double audioStart=0,audioEnd=0,cutStart=0,cutEnd=0;bool sliced=false;
    };
    const auto tracks=getGeneratorTracks();const auto seconds=playlistSecondsPerBar();std::vector<Item> items;
    for(const auto& key:selected)
    {
        if(!std::isfinite(key.startBar) || !juce::isPositiveAndBelow(key.row,project.getTrackCount()))return fail("A selected track no longer exists.");
        if(key.row>=tracks.size() || tracks[key.row]->isFrozen(te::Track::anyFreeze))return fail("Unfreeze the selected track before editing its clips.");
        const auto row64=juce::int64(key.row)+rows;const double start64=project.normalisePlaylistBar((key.rangeStart>=0?key.rangeStart:key.startBar)+bars);
        if(!erase && (row64<0 || row64>=project.getTrackCount() || start64<0 || start64>=PlaylistGrid::barCount))return fail("The group would leave the Playlist.");
        Item item;item.source=key;item.row=erase?key.row:int(row64);item.start=erase?key.startBar:start64;
        item.audio=isPlaylistAudioTrack(key.row);
        if(item.audio)
        {
            item.start=erase?key.startBar:key.startBar+bars;
            if(key.row>=tracks.size())return fail("A selected audio track is unavailable.");
            for(auto* clip:tracks[key.row]->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip);wave!=nullptr && wave->itemID.toString()==key.id)item.wave=wave;
            if(item.wave==nullptr)return fail("A selected audio clip changed.");
            const auto range=item.wave->getEditTimeRange();const auto [a,b]=selectedAudioTimeRange(*item.wave,key);
            if(b-a<.001)return fail("The audio selection is empty.");
            item.cutStart=a;item.cutEnd=b;item.sliced=a>range.getStart().inSeconds()+1.e-8 || b<range.getEnd().inSeconds()-1.e-8;
            item.audioStart=a+bars*seconds;item.audioEnd=b+bars*seconds;item.start=erase?a/seconds:a/seconds+bars;
            if(std::abs(range.getStart().inSeconds()/seconds-key.startBar)>1.0e-8)return fail("A selected audio clip moved before the edit completed.");
        }
        else
        {
            const auto id=project.getTrackId(key.row);const auto clips=project.getPlaylistClips(id);
            const auto found=std::find_if(clips.begin(),clips.end(),[&](const auto& c){return c.startBar==key.startBar && c.patternId==key.id;});
            if(found==clips.end())return fail("A selected pattern placement changed.");
            item.length=found->lengthBars;item.exact=found->exactLength;item.follows=project.playlistClipFollowsPatternLength(id,key.startBar);
            item.slot=project.getPatternIndexForId(id,key.id);
        }
        if(!erase)
        {
            if(!juce::isPositiveAndBelow(item.row,project.getTrackCount()) || item.row>=tracks.size())return fail("The group would leave the available tracks.");
            if(tracks[item.row]->isFrozen(te::Track::anyFreeze))return fail("Unfreeze the destination track before editing its clips.");
            if(item.audio!=isPlaylistAudioTrack(item.row))return fail("Audio and pattern clips require matching destination track types.");
            if(item.start<0 || (item.audio?item.audioStart<0 || item.audioEnd>PlaylistGrid::barCount*seconds:item.start+item.length>PlaylistGrid::barCount))
                return fail("The group would extend outside the Playlist.");
        }
        items.push_back(item);
    }
    auto staged=project;Selection result;
    if(!copy || erase)for(const auto& item:items)if(!item.audio)staged.setPlaylistClip(project.getTrackId(item.source.row),{},item.source.startBar,1,false);
    std::map<std::tuple<int,int,int>,juce::String> copiedPatterns;
    if(!erase)for(const auto& item:items)
    {
        if(item.audio)
        {
            for(auto* clip:tracks[item.row]->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip))
            {
                const bool removed=!copy && std::any_of(items.begin(),items.end(),[&](const auto& i){return i.wave==wave;});
                const auto range=wave->getEditTimeRange();
                if(removed)
                {
                    for(const auto& source:items)if(source.wave==wave && source.sliced)
                    {
                        const auto overlaps=[&](double a,double b){return b>a+1.e-8 && a<item.audioEnd-1.e-8 && b>item.audioStart+1.e-8;};
                        if(overlaps(range.getStart().inSeconds(),source.cutStart) || overlaps(source.cutEnd,range.getEnd().inSeconds()))return fail("The destination overlaps an unselected part of the audio.");
                    }
                }
                else if(range.getStart().inSeconds()<item.audioEnd-1.0e-8 && range.getEnd().inSeconds()>item.audioStart+1.0e-8)
                    return fail("An audio destination is occupied.");
            }
            for(const auto& other:items)
                if(&other!=&item && other.audio && other.row==item.row && other.audioStart<item.audioEnd-1.0e-8 && other.audioEnd>item.audioStart+1.0e-8)
                    return fail("Selected audio destinations overlap.");
            continue;
        }
        const auto dest=project.getTrackId(item.row);
        for(const auto& occupied:staged.getPlaylistClips(dest))
            if(occupied.startBar<item.start+item.length-1.e-9 && occupied.startBar+occupied.lengthBars>item.start+1.e-9)return fail("A pattern destination is occupied.");
        auto patternId=item.source.id;
        if(item.row!=item.source.row)
        {
            const auto key=std::make_tuple(item.source.row,item.slot,item.row);const auto existing=copiedPatterns.find(key);
            if(existing!=copiedPatterns.end())patternId=existing->second;
            else
            {
                int slot=-1;for(int i=0;i<ProjectDocument::patternSlotCount;++i)
                    if(!staged.isPatternCreated(dest,i) && staged.getPatternNotes(dest,i).empty()
                        && staged.getPatternSwitches(dest,i).empty() && staged.getPatternAutomationLaneCount(dest,i)==0){slot=i;break;}
                if(slot<0)return fail("A destination generator has no empty pattern slots.");
                if(!staged.copyPatternFrom(project,project.getTrackId(item.source.row),item.slot,dest,slot))return fail("Pattern data could not be copied.");
                patternId=staged.getPatternId(dest,slot);copiedPatterns.emplace(key,patternId);
            }
        }
        if(item.exact)staged.setPlaylistClipExactBars(dest,patternId,item.start,item.length);
        else staged.setPlaylistClip(dest,patternId,item.start,item.follows?0:item.length,true);
        result.insert({item.row,item.start,patternId});
    }
    if(!erase && playlistWindow!=nullptr)
    {
        const auto prefs=playlistWindow->playlistViewPreferences();
        const bool generators=bool(prefs.getProperty("generatorEvents",true)),mixer=bool(prefs.getProperty("mixerEvents",true));
        if(generators || mixer)
        {
            std::map<int,std::vector<orbit::playlist::EventRange>> ranges;
            const auto meter=project.getInitialTimeSignature();const double stepsPerBar=meter.numerator*16.0/juce::jmax(1,meter.denominator);
            // Native pattern placement still assumes 16 steps per bar. Do not
            // move its Song curve by a different duration in another meter.
            if((meter.numerator!=4 || meter.denominator!=4) && std::any_of(items.begin(),items.end(),[](const auto& item){return !item.audio;}))
                return fail("Moving/copying pattern Song events currently requires 4/4. Disable the event options to edit clips alone.");
            for(const auto& item:items)
            {
                const double start=item.audio?(item.audioStart-bars*seconds)/seconds*stepsPerBar:item.source.startBar*stepsPerBar;
                const double end=item.audio?(item.audioEnd-bars*seconds)/seconds*stepsPerBar:(item.source.startBar+item.length)*stepsPerBar;
                ranges[item.source.row].push_back({start,end});
            }
            juce::String error;
            if(!stagePlaylistSongEvents(staged,ranges,rows,bars,copy,generators,mixer,error))return fail(error);
        }
    }
    auto before=captureProjectSnapshot();
    for(const auto& item:items)if(item.audio)
    {
        if(erase){if(!retainAudioOutsideSelection(*item.wave,item.cutStart,item.cutEnd)){restoreProjectSnapshot(before,"Audio slicing failed; original clips restored.");return {};}continue;}
        te::Clip::Ptr duplicate;if(copy || item.sliced)duplicate=te::duplicateClip(*item.wave);
        auto* wave=(copy || item.sliced)?dynamic_cast<te::WaveAudioClip*>(duplicate.get()):item.wave;
        if(wave && item.sliced)
        {
            const auto old=item.wave->getPosition();wave->setPosition({{tracktion::TimePosition::fromSeconds(item.cutStart),tracktion::TimePosition::fromSeconds(item.cutEnd)},old.getOffset()+tracktion::TimeDuration::fromSeconds(item.cutStart-old.time.getStart().inSeconds())});
            if(item.cutStart>old.time.getStart().inSeconds()+1.e-8)wave->setFadeIn({});if(item.cutEnd<old.time.getEnd().inSeconds()-1.e-8)wave->setFadeOut({});
            if(!copy && !retainAudioOutsideSelection(*item.wave,item.cutStart,item.cutEnd)){duplicate=nullptr;restoreProjectSnapshot(before,"Audio slicing failed; original clips restored.");return {};}
        }
        if(wave==nullptr || (item.row!=item.source.row && !wave->moveTo(*tracks[item.row])))
        {duplicate=nullptr;restoreProjectSnapshot(before,"Playlist edit rolled back because an audio clip could not be transferred.");return {};}
        wave->setStart(tracktion::TimePosition::fromSeconds(item.audioStart),false,true);
        result.insert({item.row,item.start,wave->itemID.toString()});
    }
    rememberUndoSnapshot(std::move(before),erase?"Delete selected Playlist clips":copy?"Copy selected Playlist clips":"Move selected Playlist clips");
    project=std::move(staged);refreshEnginePlaylist();isDirty=true;
    refreshProjectView(juce::String(erase?"Deleted ":copy?"Copied ":"Moved ")+juce::String(items.size())+" Playlist clip(s). One Undo restores the complete edit.");
    return result;
}

bool deletePlaylistSelection(const orbit::playlist::Selection& selected)
{return transformPlaylistSelection(selected,0,0,false,true).has_value();}

void renamePlaylistClip(const orbit::playlist::ClipKey& key)
{
    if(edit==nullptr || !juce::isPositiveAndBelow(key.row,project.getTrackCount()))return;
    const auto trackId=project.getTrackId(key.row);
    if(!isPlaylistAudioTrack(key.row))
    {
        const auto slot=project.getPatternIndexForId(trackId,key.id);
        if(slot>=0)renameGeneratorOrPattern(trackId,slot,true);
        return;
    }
    auto find=[this,trackId,id=key.id]()->te::WaveAudioClip*
    {
        if(auto* track=engineTrackForProjectId(trackId))for(auto* clip:track->getClips())
            if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip);wave!=nullptr && wave->itemID.toString()==id)return wave;
        return nullptr;
    };
    auto* clip=find();if(clip==nullptr)return;
    auto panel=std::make_unique<OptionsPanel>("Rename this audio placement. Its source file remains the same.");
    panel->text("name","Name",clip->getName());
    openOptionsTool("Rename audio clip",std::move(panel),[this,find](const auto& p)
    {
        auto* wave=find();if(wave==nullptr){status.setText("This audio clip is no longer available.",juce::dontSendNotification);return false;}
        const auto name=p.string("name").trim();
        if(name.isEmpty() || name.length()>128){status.setText("Enter a name of 1-128 characters.",juce::dontSendNotification);return false;}
        if(name==wave->getName())return true;
        rememberUndo("Rename audio clip");wave->setName(name);isDirty=true;
        if(playlistWindow!=nullptr)playlistWindow->refresh();return true;
    },true);
}
