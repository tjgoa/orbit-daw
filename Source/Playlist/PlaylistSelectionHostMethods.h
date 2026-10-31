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
        if(project.getTrackKind(row)!=ProjectDocument::TrackKind::audio)
        {
            for(const auto& clip:project.getPlaylistClips(project.getTrackId(row)))
                items.push_back({{row,clip.startBar,clip.patternId},clip.lengthBars});
        }
        else if(row<tracks.size())
            for(auto* clip:tracks[row]->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip))
            {
                const auto range=wave->getEditTimeRange();const int start=int(std::floor(range.getStart().inSeconds()/barSeconds+1.0e-8));
                const int end=int(std::ceil(range.getEnd().inSeconds()/barSeconds-1.0e-8));
                items.push_back({{row,start,wave->itemID.toString()},juce::jmax(1,end-start)});
            }
    }
    return items;
}

std::optional<orbit::playlist::Selection> transformPlaylistSelection(const orbit::playlist::Selection& selected,int rows,int bars,bool copy,bool erase=false)
{
    using Selection=orbit::playlist::Selection;using Key=orbit::playlist::ClipKey;
    auto fail=[this](const juce::String& why)->std::optional<Selection>{status.setText("Playlist edit canceled. "+why,juce::dontSendNotification);return {};};
    if(edit==nullptr || !hasProject || selected.empty())return {};
    if(midiTake.active || !midiTake.notes.empty())return fail("Finish recording before editing the arrangement.");
    if(!erase && rows==0 && bars==0)return selected;
    struct Item
    {
        Key source;int row=0,start=0,length=1,slot=-1;bool audio=false,exact=false,follows=false;
        te::WaveAudioClip* wave=nullptr;double audioStart=0,audioEnd=0;
    };
    const auto tracks=getGeneratorTracks();const auto seconds=playlistSecondsPerBar();std::vector<Item> items;
    for(const auto& key:selected)
    {
        if(!juce::isPositiveAndBelow(key.row,project.getTrackCount()))return fail("A selected track no longer exists.");
        if(key.row>=tracks.size() || tracks[key.row]->isFrozen(te::Track::anyFreeze))return fail("Unfreeze the selected track before editing its clips.");
        const auto row64=juce::int64(key.row)+rows,start64=juce::int64(key.startBar)+bars;
        if(!erase && (row64<0 || row64>=project.getTrackCount() || start64<0 || start64>=PlaylistGrid::barCount))return fail("The group would leave the Playlist.");
        Item item;item.source=key;item.row=erase?key.row:int(row64);item.start=erase?key.startBar:int(start64);
        item.audio=project.getTrackKind(key.row)==ProjectDocument::TrackKind::audio;
        if(item.audio)
        {
            if(key.row>=tracks.size())return fail("A selected audio track is unavailable.");
            for(auto* clip:tracks[key.row]->getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip);wave!=nullptr && wave->itemID.toString()==key.id)item.wave=wave;
            if(item.wave==nullptr)return fail("A selected audio clip changed.");
            const auto range=item.wave->getEditTimeRange();item.audioStart=range.getStart().inSeconds()+bars*seconds;item.audioEnd=range.getEnd().inSeconds()+bars*seconds;
            if(int(std::floor(range.getStart().inSeconds()/seconds+1.0e-8))!=key.startBar)return fail("A selected audio clip moved before the edit completed.");
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
            if(item.audio!=(project.getTrackKind(item.row)==ProjectDocument::TrackKind::audio))return fail("Audio and pattern clips require matching destination track types.");
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
                if(!removed && range.getStart().inSeconds()<item.audioEnd-1.0e-8 && range.getEnd().inSeconds()>item.audioStart+1.0e-8)
                    return fail("An audio destination is occupied.");
            }
            for(const auto& other:items)
                if(&other!=&item && other.audio && other.row==item.row && other.audioStart<item.audioEnd-1.0e-8 && other.audioEnd>item.audioStart+1.0e-8)
                    return fail("Selected audio destinations overlap.");
            continue;
        }
        const auto dest=project.getTrackId(item.row);
        for(const auto& occupied:staged.getPlaylistClips(dest))
            if(occupied.startBar<item.start+item.length && occupied.startBar+occupied.lengthBars>item.start)return fail("A pattern destination is occupied.");
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
    auto before=captureProjectSnapshot();
    for(const auto& item:items)if(item.audio)
    {
        if(erase){item.wave->removeFromParent();continue;}
        te::Clip::Ptr duplicate;if(copy)duplicate=te::duplicateClip(*item.wave);
        auto* wave=copy?dynamic_cast<te::WaveAudioClip*>(duplicate.get()):item.wave;
        if(wave==nullptr || (item.row!=item.source.row && !wave->moveTo(*tracks[item.row])))
        {duplicate=nullptr;restoreProjectSnapshot(before,"Playlist edit rolled back because an audio clip could not be transferred.");return {};}
        // A placement move keeps the same source excerpt. preserveSync=true
        // would slip the source offset by the timeline movement instead.
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
    if(project.getTrackKind(key.row)!=ProjectDocument::TrackKind::audio)
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
