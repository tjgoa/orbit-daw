// Included inside ProjectView. A freeze is a durable generator-only Song print,
// not Tracktion's temporary whole-track freeze. The original stays in the Edit.
struct FreezeJob
{
    std::unique_ptr<te::Edit> renderEdit;
    juce::ValueTree metadata;
    juce::String projectID, trackID, sourceSignature;
    te::Edit* sourceEdit = nullptr;
    juce::File file;
    int renderTrack = -1;
    double duration = 0, sampleRate = 48000;
    bool permanent = false;
};
std::unique_ptr<FreezeJob> freezeJob;
// Run bounded native render batches between message-loop turns. This keeps
// Cancel responsive and destroys the render graph on its required message thread;
// joining EditRenderer's worker here can deadlock its callBlocking teardown.
struct FreezePump final:juce::Timer
{
    std::function<void()> action;
    void timerCallback() override {const auto callback=action;if(callback)callback();}
};
std::unique_ptr<FreezePump> freezePump;
std::unique_ptr<te::Edit::ScopedRenderStatus> freezeRenderStatus;
std::unique_ptr<te::Renderer::RenderTask> freezeTask;
std::atomic<float> freezeRenderProgress{0};
std::unique_ptr<orbit::workspace::OwnedAlert> freezeProgress;
double freezeProgressValue = 0;
bool freezeCancelled = false;
juce::String freezeRecoveryMessage;

juce::ValueTree freezeState(const juce::String& id) const
{
    if(auto* track=engineTrackForProjectId(id))return track->state.getChildWithName("ORBITFREEZE");
    return {};
}
bool isGeneratorFrozen(const juce::String& id) const { return freezeState(id).isValid(); }
bool hasFrozenGenerators() const
{
    for(int i=0;i<project.getTrackCount();++i)if(isGeneratorFrozen(project.getTrackId(i)))return true;
    return false;
}
bool isFrozenSourceParameter(const juce::String& id,const juce::String& pluginID,const juce::String& name) const
{
    const auto state=freezeState(id);if(!state.isValid())return false;
    if(name=="MIDI")return true;
    for(const auto& source:state)
    {
        if(pluginID.isNotEmpty()?source.getProperty("sourceID").toString()==pluginID:source.getProperty("sourceName").toString()==name)return true;
        for(const auto& child:source.getChildWithName("BOUNDPLUGINS"))
            if(pluginID.isNotEmpty()?child.getProperty("id").toString()==pluginID:child.getProperty("name").toString()==name)return true;
    }
    return false;
}
bool requireUnfrozenGenerator(const juce::String& id)
{
    if(!isGeneratorFrozen(id))return true;
    status.setText("This generator is frozen. Choose Unfreeze in its menu before editing its sound, patterns or arrangement.",juce::dontSendNotification);
    return false;
}
bool frozenPatternAuditionAllowed(const juce::String& id) const
{
    const auto state=freezeState(id);return !state.isValid() || bool(state.getProperty("keepLivePattern",true));
}
std::vector<te::Plugin*> freezeSourcePlugins(const juce::String& id) const
{
    std::vector<te::Plugin*> result;
    auto* instrument=generatorControlPlugin(id);
    if(auto* track=engineTrackForProjectId(id))for(auto* plugin:track->pluginList)
        if(plugin==instrument || dynamic_cast<orbit::generator::MidiPlugin*>(plugin)!=nullptr)result.push_back(plugin);
    return result;
}
// Hash source musical content only. Selection, names, colours and live mixer
// automation are intentionally excluded. This also protects less common import
// paths: changed source data restores the live source rather than stale audio.
juce::String freezeMusicalSignature(const juce::String& id,const juce::ValueTree& sources={}) const
{
    juce::MemoryOutputStream out;juce::StringArray names,instances;
    if(const auto frozen=sources.isValid()?sources:freezeState(id);frozen.isValid())
    {for(const auto& source:frozen){names.add(source.getProperty("sourceName").toString());instances.add(source.getProperty("sourceID").toString());for(const auto& child:source.getChildWithName("BOUNDPLUGINS")){names.add(child.getProperty("name").toString());instances.add(child.getProperty("id").toString());}}}
    else for(auto* plugin:freezeSourcePlugins(id)){names.add(plugin->getName());instances.add(plugin->itemID.toString());}
    out.writeDouble(project.getInitialTempoBpm());const auto meter=project.getInitialTimeSignature();out.writeInt(meter.numerator);out.writeInt(meter.denominator);
    const auto lane=[&](const ProjectDocument::AutomationLane& value)
    {
        if(value.pluginName!="MIDI" && !(value.pluginId.isNotEmpty()?instances.contains(value.pluginId):names.contains(value.pluginName)))return;
        out.writeString(value.pluginName);out.writeString(value.pluginId);out.writeString(value.parameterId);out.writeInt(value.points.size());
        for(const auto& point:value.points){out.writeDouble(point.position);out.writeFloat(point.value);}
    };
    for(int slot=0;slot<ProjectDocument::patternSlotCount;++slot)
    {
        out.writeBool(project.isPatternCreated(id,slot));out.writeInt(project.getPatternLengthSteps(id,slot));
        const auto notes=project.getPatternNotes(id,slot);out.writeInt(int(notes.size()));
        for(const auto& n:notes){out.writeInt(n.pitch);out.writeInt(n.startTick);out.writeInt(n.lengthTicks);out.writeInt(n.velocity);}
        const auto switches=project.getPatternSwitches(id,slot);out.writeInt(int(switches.size()));
        for(const auto& s:switches){out.writeInt(s.startTick);out.writeInt(s.lengthTicks);out.writeBool(s.slide);}
        for(int i=0;i<project.getPatternAutomationLaneCount(id,slot);++i)lane(project.getPatternAutomationLane(id,slot,i));
    }
    for(const auto& clip:project.getPlaylistClips(id)){out.writeString(clip.patternId);out.writeDouble(clip.startBar);out.writeDouble(clip.lengthBars);}
    for(int i=0;i<project.getSongAutomationLaneCount(id);++i)lane(project.getSongAutomationLane(id,i));
    juce::uint64 hash=14695981039346656037ULL;const auto* bytes=static_cast<const unsigned char*>(out.getData());
    for(size_t i=0;i<out.getDataSize();++i){hash^=bytes[i];hash*=1099511628211ULL;}
    return juce::String::toHexString(juce::int64(hash));
}
bool requireLiveMixer(const juce::String& owner)
{
    // Mixer callbacks use native EditItemIDs; generator menus use document IDs.
    auto id=owner;
    for(int row=0;row<project.getTrackCount();++row)
        if(auto* track=engineTrackForProjectId(project.getTrackId(row));track && track->itemID.toString()==owner){id=project.getTrackId(row);break;}
    if(!bool(freezeState(id).getProperty("bakeMixer",false)))return true;
    return requireUnfrozenGenerator(id);
}
void configureFreezeAudio(te::WaveAudioClip& clip)
{
    // The legacy proxy reader introduces a two-source-sample interpolation
    // delay. Use the real-time reader for phase-aligned cached audio instead.
    clip.setUsesProxy(false);clip.setAutoTempo(false);clip.setTimeStretchMode(te::TimeStretcher::disabled);
    clip.setResamplingQuality(te::ResamplingQuality::lagrange);
    clip.state.setProperty("orbitRenderedAudio",true,nullptr);
}
void configureFreezeAudioInEdit(te::Edit& session)
{
    for(auto* track:te::getAudioTracks(session))for(auto* clip:track->getClips())
        if(auto* wave=dynamic_cast<te::WaveAudioClip*>(clip);wave && bool(wave->state.getProperty("orbitRenderedAudio",false)))configureFreezeAudio(*wave);
}
std::vector<te::Plugin*> freezePrintPlugins(const juce::String& id,bool bakeMixer,juce::String& error)
{
    auto sources=freezeSourcePlugins(id);if(!bakeMixer)return sources;
    auto* track=engineTrackForProjectId(id);if(!track)return {};
    const int index=getMixerTracks().indexOf(track);const auto inserts=getInsertPlugins(index);
    int last=-1,i=0;
    for(auto* plugin:track->pluginList)
    {
        if(std::find(sources.begin(),sources.end(),plugin)!=sources.end() || inserts.contains(plugin)
            || dynamic_cast<te::EqualiserPlugin*>(plugin) || dynamic_cast<orbit::mixing::EqSaturationPlugin*>(plugin))last=i;
        ++i;
    }
    std::vector<te::Plugin*> printed;i=0;
    for(auto* plugin:track->pluginList)
    {
        if(i++>last)break;
        if(dynamic_cast<te::VolumeAndPanPlugin*>(plugin) || dynamic_cast<te::AuxSendPlugin*>(plugin) || dynamic_cast<te::AuxReturnPlugin*>(plugin))
        {error="Mixer baking requires inserts and EQ before the fader and sends. Move pre-fader sends after the effects, or freeze with mixer baking off.";return {};}
        if(dynamic_cast<te::LevelMeterPlugin*>(plugin))continue;
        if(plugin->getSidechainSourceID().isValid())
        {error="Baking sidechained mixer effects is not supported yet. Keep mixer processing live for this track.";return {};}
        // Shared rack definitions can drive several tracks. A generator-only
        // print is safe; baking such a rack requires isolated rack stems later.
        if(auto* rack=dynamic_cast<te::RackInstance*>(plugin))
        {
            int instances=0;for(auto* other:te::getAllPlugins(*edit,true))if(auto* instance=dynamic_cast<te::RackInstance*>(other);instance && instance->rackTypeID==rack->rackTypeID)++instances;
            if(instances>1){error="This FX rack is shared by several tracks. Keep mixer processing live when freezing this generator.";return {};}
            for(auto* child:rack->type->getPlugins())if(child->getSidechainSourceID().isValid() || dynamic_cast<te::RackInstance*>(child))
            {error="Nested or sidechained FX racks cannot be baked yet. Keep mixer processing live.";return {};}
        }
        printed.push_back(plugin);
    }
    return printed;
}
void removeFreezeProjection(te::AudioTrack& track)
{
    std::vector<te::Clip*> clips;
    for(auto* clip:track.getClips())if(bool(clip->state.getProperty("orbitFreezeProjection",false)))clips.push_back(clip);
    for(auto* clip:clips)clip->removeFromParent();
}
void restoreFreezeSource(const juce::String& id)
{
    auto state=freezeState(id);if(!state.isValid())return;
    if(auto* track=engineTrackForProjectId(id))
    {
        for(auto* plugin:track->pluginList)
            if(auto saved=state.getChildWithProperty("sourceID",plugin->itemID.toString());saved.isValid() && saved.getNumChildren()>0)
            {
                plugin->setFrozen(false);
                const auto original=saved.getChild(0).createCopy();
                plugin->state.copyPropertiesAndChildrenFrom(original,nullptr);
                plugin->restorePluginStateFromValueTree(original);
                plugin->setEnabled(bool(saved.getProperty("enabled",true)));
                if(auto* rack=dynamic_cast<te::RackInstance*>(plugin);rack && saved.getChildWithName("RACKSTATE").getNumChildren()>0)
                    rack->type->restoreStateFromValueTree(saved.getChildWithName("RACKSTATE").getChild(0));
            }
        removeFreezeProjection(*track);track->state.removeChild(state,nullptr);
    }
}
bool unfreezeGenerator(const juce::String& id)
{
    if(!isGeneratorFrozen(id) || freezeJob)return false;
    if(!finishMidiRecording())return false;
    edit->getTransport().stop(false,false);eraseGeneratorControlWindow(id);
    rememberUndo("Unfreeze generator");restoreFreezeSource(id);isDirty=true;
    refreshEnginePlaylist();readPatternSteps();updateActiveInstrumentEditor();
    refreshProjectView("Generator unfrozen. Original sound, patterns and automation restored; mixer changes retained. Undo can restore the freeze.");
    return true;
}
// Called for every projection and project restore, never from the audio thread.
// A missing/corrupt cache must not leave the preserved source silently disabled.
bool validateFreeze(const juce::String& id)
{
    const auto state=freezeState(id);if(!state.isValid())return false;
    const juce::File file(state.getProperty("file").toString());
    bool valid=file.existsAsFile() && file.getSize()==juce::int64(state.getProperty("bytes",juce::int64(-1)));
    if(valid)
    {
        juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(file),true));
        valid=reader && reader->numChannels==2 && reader->sampleRate>0 && std::abs(reader->lengthInSamples/reader->sampleRate-double(state.getProperty("duration")))<.001;
    }
    valid=valid && state.getProperty("signature").toString()==freezeMusicalSignature(id);
    if(!valid)
    {
        restoreFreezeSource(id);isDirty=true;
        freezeRecoveryMessage="Frozen audio is missing, damaged, or its source timing changed. The original generator was restored. Save to keep the recovery.";status.setText(freezeRecoveryMessage,juce::dontSendNotification);
    }
    return valid;
}
void setFrozenSourcePlayback(const juce::String& id,bool livePattern)
{
    const auto state=freezeState(id);if(!state.isValid())return;
    if(auto* track=engineTrackForProjectId(id))for(auto* plugin:track->pluginList)
        if(const auto saved=state.getChildWithProperty("sourceID",plugin->itemID.toString());saved.isValid())
        {
            plugin->setFrozen(!livePattern);plugin->setEnabled(livePattern && bool(saved.getProperty("enabled",true)));
        }
}
void updateFrozenAudioMode(const juce::String& id,bool pattern)
{
    if(auto* track=engineTrackForProjectId(id))for(auto* item:track->getClips())if(auto* clip=dynamic_cast<te::WaveAudioClip*>(item))
    {
        clip->state.setProperty("orbitFreezeProjection",true,nullptr);configureFreezeAudio(*clip);
        if(pattern)
        {if(!clip->state.hasProperty("orbitFrozenWasMuted"))clip->state.setProperty("orbitFrozenWasMuted",clip->isMuted(),nullptr);clip->setMuted(true);}
        else if(clip->state.hasProperty("orbitFrozenWasMuted"))
        {clip->setMuted(bool(clip->state.getProperty("orbitFrozenWasMuted")));clip->state.removeProperty("orbitFrozenWasMuted",nullptr);}
    }
}
bool projectFrozenSong(const juce::String& id,te::AudioTrack& track)
{
    if(!validateFreeze(id))return false;
    auto state=freezeState(id);setFrozenSourcePlayback(id,false);
    // Native Wave clips are the editable Song arrangement. Keep them in place
    // across refresh/save/Pattern audition, including an intentionally empty lane.
    bool existing=false;for(auto* item:track.getClips())if(auto* wave=dynamic_cast<te::WaveAudioClip*>(item))
    {
        existing=true;
        if(!bool(state.getProperty("audioReady",false)) && wave->getName().startsWith("Frozen"))wave->setName("Frozen - "+track.getName());
    }
    if(!bool(state.getProperty("audioReady",false)) && !existing)
    {
        const auto length=double(state.getProperty("duration"));
        auto clip=track.insertWaveClip("Frozen - "+track.getName(),juce::File(state.getProperty("file").toString()),
            {{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(length)},tracktion::TimeDuration::fromSeconds(0)},false);
        if(!clip){restoreFreezeSource(id);isDirty=true;status.setText("Could not attach frozen audio. Original generator restored.",juce::dontSendNotification);return false;}
    }
    state.setProperty("audioReady",true,nullptr);updateFrozenAudioMode(id,false);return true;
}
void toggleFrozenGhostNotes(const juce::String& id)
{
    auto state=freezeState(id);if(!state.isValid())return;rememberUndo("Toggle frozen ghost notes");
    state.setProperty("showGhostNotes",!bool(state.getProperty("showGhostNotes",false)),nullptr);isDirty=true;if(playlistWindow)playlistWindow->refresh();
}
std::vector<juce::Rectangle<float>> frozenGhostNotes(int row,double startBar)
{
    std::vector<juce::Rectangle<float>> result;const auto id=project.getTrackId(row);const auto state=freezeState(id);
    if(!state.isValid() || !bool(state.getProperty("showGhostNotes",false)))return result;
    auto* clip=findAudioClipAtPosition(row,startBar);if(!clip || clip->getOriginalFile()!=juce::File(state.getProperty("file").toString()))return result;
    const double duration=clip->getEditTimeRange().getLength().inSeconds(),offset=clip->getPosition().getOffset().inSeconds()*clip->getSpeedRatio(),speed=clip->getSpeedRatio();
    if(duration<=0 || speed<=0)return result;
    const auto meter=project.getInitialTimeSignature();const double stepsPerBar=meter.numerator*16./juce::jmax(1,meter.denominator),ticksPerBar=stepsPerBar*64.;
    const double sourceLength=double(state.getProperty("duration")),barSeconds=playlistSecondsPerBar();int low=127,high=0;
    for(const auto& placement:project.getPlaylistClips(id))for(const auto& note:project.getPatternNotes(id,project.getPatternIndexForId(id,placement.patternId))){low=juce::jmin(low,note.pitch);high=juce::jmax(high,note.pitch);}
    const int pitches=juce::jmax(12,high-low+1);
    for(const auto& placement:project.getPlaylistClips(id))
    {
        const int slot=project.getPatternIndexForId(id,placement.patternId);if(slot<0)continue;
        const double cycle=project.getPatternLengthSteps(id,slot)/stepsPerBar;if(cycle<=0)continue;
        for(double repeat=0;repeat<placement.lengthBars;repeat+=cycle)for(const auto& note:project.getPatternNotes(id,slot))
        {
            const double local=repeat+note.startTick/ticksPerBar;if(local>=placement.lengthBars)continue;
            const double noteStart=(placement.startBar+local)*barSeconds,noteEnd=(placement.startBar+juce::jmin(placement.lengthBars,local+note.lengthTicks/ticksPerBar))*barSeconds;
            const double a=clip->getIsReversed()?sourceLength-noteEnd:noteStart,b=clip->getIsReversed()?sourceLength-noteStart:noteEnd;
            const auto x=juce::jlimit(0.,1.,(a-offset)/speed/duration),end=juce::jlimit(0.,1.,(b-offset)/speed/duration);
            if(end>x)result.emplace_back(float(x),float(high-note.pitch+(pitches-(high-low+1))/2)/pitches,float(end-x),1.f/pitches);
            if(result.size()>=20000)return result;
        }
    }
    return result;
}

void applyFreezeSongAutomation(te::Edit& target,const juce::String& id,te::Plugin& plugin)
{
    for(auto* param:plugin.getAutomatableParameters())if(param!=nullptr)
    {
        const ProjectDocument::AutomationLane* active=nullptr;std::optional<ProjectDocument::AutomationLane> exact,legacy;
        for(int i=0;i<project.getSongAutomationLaneCount(id);++i)
        {
            auto lane=project.getSongAutomationLane(id,i);if(lane.parameterId!=param->paramID)continue;
            if(lane.pluginId==plugin.itemID.toString())exact=std::move(lane);
            else if(lane.pluginId.isEmpty() && lane.pluginName==plugin.getName())legacy=std::move(lane);
        }
        int matches=0;bool patternOwned=false;
        if(auto* owner=engineTrackForProjectId(id))for(auto* candidate:orbit::mixing::flattenedPlugins(owner->pluginList))
            if(candidate->getName()==plugin.getName())for(auto* parameter:candidate->getAutomatableParameters())if(parameter->paramID==param->paramID)++matches;
        for(int slot=0;slot<ProjectDocument::patternSlotCount;++slot)for(int i=0;i<project.getPatternAutomationLaneCount(id,slot);++i)
        {const auto lane=project.getPatternAutomationLane(id,slot,i);if(lane.pluginName==plugin.getName() && lane.parameterId==param->paramID)patternOwned=true;}
        if(!exact && !legacy && !patternOwned)continue;
        const bool unique=matches==1;
        if(exact)active=&*exact;else if(unique && legacy)active=&*legacy;
        const auto base=plugin.state.getChildWithName("ORBITAUTOMATIONBASES").getChildWithProperty("parameterId",param->paramID);
        const auto initial=base.isValid()?float(base.getProperty("value")):param->getCurrentNormalisedValue();
        std::map<double,float> points;
        if(active && !active->points.isEmpty())for(const auto& p:active->points)points[p.position]=p.value;
        else if(unique)points=orbit::automation::arrangedPatternCurve(project,id,plugin.getName(),param->paramID,initial);
        auto& curve=param->getCurve();curve.clear();param->setNormalisedParameter(initial,juce::dontSendNotification);
        for(const auto& [step,value]:points)curve.addPoint(target.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(step*.25)),param->valueRange.convertFrom0to1(juce::jlimit(0.f,1.f,value)),0.f);
    }
}
std::unique_ptr<FreezeJob> prepareFreeze(const juce::String& id,bool keepLive,double extraTail,juce::String& error,bool bakeMixer=false,bool permanent=false)
{
    const auto fail=[&](const juce::String& message)->std::unique_ptr<FreezeJob>{error=message;return {};};
    if(edit==nullptr || freezeJob || renderHandle || isGeneratorFrozen(id))return fail("This generator cannot be frozen right now.");
    auto* track=engineTrackForProjectId(id);auto* instrument=generatorControlPlugin(id);
    if(track!=nullptr && track->isFrozen(te::Track::anyFreeze))return fail("Unfreeze this track before using Orbit Song Freeze.");
    if(track==nullptr || instrument==nullptr)return fail("Freeze requires an instrument generator with a Song arrangement.");
    if(track->state.getProperty("orbitMidiSource").toString().isNotEmpty())return fail("Freeze of MIDI received from another generator is not supported yet. Put its notes on this generator first.");
    for(int row=0;row<project.getTrackCount();++row)
        if(auto* other=engineTrackForProjectId(project.getTrackId(row));other && other->state.getProperty("orbitMidiSource").toString()==id)
            return fail("Another generator receives MIDI from this track. Put its notes on that generator before freezing this source.");
    if(permanent){bakeMixer=true;keepLive=false;}
    error.clear();const auto sources=freezePrintPlugins(id,bakeMixer,error);if(error.isNotEmpty())return {};
    double tail=juce::jlimit(0.,600.,extraTail),body=0;
    double reportedTail=0;
    for(auto* source:sources)
    {
        if(source->getSidechainSourceID().isValid())return fail("Freeze of a sidechained generator is not supported yet.");
        if(!source->isEnabled())continue;
        const auto reported=source->getTailLength();if(std::isfinite(reported))reportedTail+=juce::jmax(0.,reported);
        if(auto* rack=dynamic_cast<te::RackInstance*>(source))for(auto* child:rack->type->getPlugins())if(child->isEnabled())
        {const auto childTail=child->getTailLength();if(std::isfinite(childTail))reportedTail+=juce::jmax(0.,childTail);}

    }
    tail=juce::jmax(tail,reportedTail);
    for(const auto& clip:project.getPlaylistClips(id))body=juce::jmax(body,(clip.startBar+clip.lengthBars)*playlistSecondsPerBar());
    if(body<=0)return fail("Arrange at least one pattern for this generator in the Song Playlist before freezing.");
    if(!std::isfinite(tail) || tail>600)return fail("This generator reports a tail longer than ten minutes. Shorten its release or internal effects before freezing.");
    auto job=std::make_unique<FreezeJob>();job->projectID=project.getProjectId();job->trackID=id;job->sourceEdit=edit.get();job->sourceSignature=freezeMusicalSignature(id);job->permanent=permanent;
    const auto root=project.getFile()!=juce::File{}?project.getFile().getSiblingFile(project.getFile().getFileNameWithoutExtension()+" Audio").getChildFile("Freeze")
        :juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Orbit/Freeze").getChildFile(project.getProjectId());
    job->file=root.getChildFile(id+"-"+juce::Uuid().toString()).getChildFile("generator.wav");
    if(auto result=job->file.getParentDirectory().createDirectory();result.failed())return fail("Could not create the Freeze folder: "+result.getErrorMessage());
    edit->flushState();job->metadata=juce::ValueTree("ORBITFREEZE");
    for(auto* source:sources)
    {
        source->flushPluginStateToValueTree();juce::ValueTree saved("SOURCE");saved.setProperty("sourceID",source->itemID.toString(),nullptr);saved.setProperty("enabled",source->isEnabled(),nullptr);saved.setProperty("sourceName",source->getName(),nullptr);saved.addChild(source->state.createCopy(),-1,nullptr);
        if(auto* rack=dynamic_cast<te::RackInstance*>(source))
        {
            juce::ValueTree rackState("RACKSTATE"),bindings("BOUNDPLUGINS");rackState.addChild(rack->type->createStateCopy(true),-1,nullptr);saved.addChild(rackState,-1,nullptr);
            for(auto* child:rack->type->getPlugins()){juce::ValueTree binding("PLUGIN");binding.setProperty("id",child->itemID.toString(),nullptr);binding.setProperty("name",child->getName(),nullptr);bindings.addChild(binding,-1,nullptr);}
            saved.addChild(bindings,-1,nullptr);
        }
        job->metadata.addChild(saved,-1,nullptr);
    }
    job->metadata.setProperty("bakeMixer",bakeMixer,nullptr);
    job->sourceSignature=freezeMusicalSignature(id,job->metadata);
    job->metadata.setProperty("version",1,nullptr);job->metadata.setProperty("keepLivePattern",keepLive,nullptr);job->metadata.setProperty("signature",job->sourceSignature,nullptr);
    // Do not instantiate every other VST/editor just to print one generator.
    // Retain only selected processing (and its isolated rack members), tempo and
    // routing definitions in this independent render snapshot.
    std::set<juce::String> needed;
    for(auto* source:sources){needed.insert(source->itemID.toString());if(auto* rack=dynamic_cast<te::RackInstance*>(source))for(auto* child:rack->type->getPlugins())needed.insert(child->itemID.toString());}
    auto renderState=edit->state.createCopy();
    std::function<void(juce::ValueTree)> prune=[&](juce::ValueTree tree)
    {
        for(int i=tree.getNumChildren();--i>=0;)
        {
            auto child=tree.getChild(i);
            if(child.hasType("ORBITFREEZE") || (child.hasType(te::IDs::PLUGIN) && !needed.contains(child.getProperty(te::IDs::id).toString())))tree.removeChild(i,nullptr);
            else prune(child);
        }
    };prune(renderState);
    job->renderEdit=te::loadEditFromState(engine,renderState,te::Edit::forRendering);
    if(!job->renderEdit)return fail("Could not create the isolated Freeze session.");
    te::AudioTrack* target=nullptr;const auto all=te::getAllTracks(*job->renderEdit);
    for(int i=0;i<all.size();++i)if(all[i]->itemID==track->itemID){target=dynamic_cast<te::AudioTrack*>(all[i]);job->renderTrack=i;break;}
    if(target==nullptr)return fail("The generator was not found in the render session.");
    // Clear all clones' clips and disable their mixer/input processing. Only this
    // generator and its MIDI processor feed the print, independent of routing.
    for(auto* other:te::getAudioTracks(*job->renderEdit))
    {
        auto clips=other->getClips();for(auto* clip:clips)clip->removeFromParent();other->setMute(false);other->setSolo(false);
        for(auto* plugin:other->pluginList)
        {
            const auto saved=job->metadata.getChildWithProperty("sourceID",plugin->itemID.toString());
            plugin->setEnabled(other==target && saved.isValid() && bool(saved.getProperty("enabled",true)));
            if(other==target && saved.isValid())
            {
                applyFreezeSongAutomation(*job->renderEdit,id,*plugin);
                if(auto* rack=dynamic_cast<te::RackInstance*>(plugin))for(auto* child:rack->type->getPlugins())applyFreezeSongAutomation(*job->renderEdit,id,*child);
            }
        }
    }
    target->getOutput().setOutputToDefaultDevice(false);
    const double secondsPerBar=playlistSecondsPerBar();
    for(const auto& placement:project.getPlaylistClips(id))
    {
        const int slot=project.getPatternIndexForId(id,placement.patternId);if(slot<0 || !project.isPatternCreated(id,slot))continue;
        const auto start=placement.startBar*secondsPerBar;const auto length=project.getPatternLengthSteps(id,slot);
        auto clip=target->insertMIDIClip(project.getPatternName(id,slot),{tracktion::TimePosition::fromSeconds(start),tracktion::TimePosition::fromSeconds(start+placement.lengthBars*secondsPerBar)},nullptr);
        if(clip==nullptr)return fail("Could not create a MIDI clip for Freeze.");
        clip->setLoopRangeBeats({tracktion::BeatPosition::fromBeats(0),tracktion::BeatPosition::fromBeats(length*.25)});
        for(const auto& n:project.getPatternNotes(id,slot))clip->getSequence().addNote(n.pitch,tracktion::BeatPosition::fromBeats(n.startTick/256.),tracktion::BeatDuration::fromBeats(n.lengthTicks/256.),n.velocity,0,nullptr);
        addMidiAutomationToClip(*clip,id,slot,placement.startBar,length);
    }
    job->duration=body+tail;job->sampleRate=engine.getDeviceManager().getSampleRate();if(job->sampleRate<=0)job->sampleRate=48000;
    job->metadata.setProperty("bodySeconds",body,nullptr);job->metadata.setProperty("tailSeconds",tail,nullptr);job->metadata.setProperty("sampleRate",job->sampleRate,nullptr);
    job->renderEdit->dispatchPendingUpdatesSynchronously();return job;
}
te::Renderer::Parameters freezeRenderParameters(FreezeJob& job)
{
    te::Renderer::Parameters p(*job.renderEdit);p.destFile=job.file;p.audioFormat=engine.getAudioFileFormatManager().getWavFormat();
    p.sampleRateForAudio=job.sampleRate;p.bitDepth=32;p.canRenderInMono=false;p.checkNodesForAudio=false;p.usePlugins=true;p.useMasterPlugins=false;
    p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(job.duration)};p.tracksToDo.setBit(job.renderTrack);return p;
}
bool commitFreeze(FreezeJob& job,juce::String& error)
{
    if(edit.get()!=job.sourceEdit || project.getProjectId()!=job.projectID || engineTrackForProjectId(job.trackID)==nullptr || freezeMusicalSignature(job.trackID,job.metadata)!=job.sourceSignature)
    {error="The source session changed while rendering. Freeze was not applied.";return false;}
    juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(new juce::FileInputStream(job.file),true));
    if(!reader || reader->numChannels!=2 || reader->lengthInSamples<juce::int64(std::floor(job.duration*job.sampleRate))-512)
    {error="The rendered Freeze file is incomplete or unreadable. The original generator is unchanged.";return false;}
    job.metadata.setProperty("file",job.file.getFullPathName(),nullptr);job.metadata.setProperty("bytes",job.file.getSize(),nullptr);job.metadata.setProperty("duration",reader->lengthInSamples/reader->sampleRate,nullptr);
    // Validate attachment before any irreversible conversion or Undo boundary.
    auto* track=engineTrackForProjectId(job.trackID);
    if(job.permanent)
    {
        auto wave=track->insertWaveClip("Rendered - "+track->getName(),job.file,
            {{tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(double(job.metadata.getProperty("duration")))},{}},false);
        if(wave==nullptr){error="Could not attach the rendered audio. Original source unchanged.";return false;}
        configureFreezeAudio(*wave);eraseGeneratorControlWindow(job.trackID);
        instrumentEditorClip.setViewedComponent(nullptr,false);activeInstrumentEditor.reset();
        if(drumRackWindow)drumRackWindow->setVisible(false);if(equaliserWindow)equaliserWindow->setVisible(false);
        juce::StringArray ids,names;std::vector<te::Plugin::Ptr> removed;
        for(const auto& source:job.metadata)
        {
            ids.add(source.getProperty("sourceID").toString());names.add(source.getProperty("sourceName").toString());
            for(const auto& child:source.getChildWithName("BOUNDPLUGINS")){ids.add(child.getProperty("id").toString());names.add(child.getProperty("name").toString());}
        }
        for(auto* plugin:track->pluginList)if(ids.contains(plugin->itemID.toString()))removed.emplace_back(plugin);
        for(auto& plugin:removed)
        {
            closeEffectEditor(*plugin);auto* rack=dynamic_cast<te::RackInstance*>(plugin.get());auto type=rack?rack->type:te::RackType::Ptr{};plugin->deleteFromParent();
            if(type)edit->getRackList().removeRackType(type);
        }
        std::vector<te::MidiClip*> midi;for(auto* clip:track->getClips())if(auto* m=dynamic_cast<te::MidiClip*>(clip))midi.push_back(m);
        patternClip=nullptr;for(auto* clip:midi)clip->removeFromParent();
        const int row=getGeneratorTracks().indexOf(track);project.convertTrackToRenderedAudio(row,ids,names);
        undoHistory.clear();redoHistory.clear();edit->getUndoManager().clearUndoHistory();
        edit->flushState();project.setTracktionEditState(edit->state.createCopy());
        isDirty=true;refreshEnginePlaylist();readPatternSteps();refreshTrackSelector();updateActiveInstrumentEditor();
        refreshProjectView("Converted to audio permanently. Generator and baked mixer effects unloaded; Undo history cleared. Save to keep this conversion.");return true;
    }
    rememberUndo("Freeze generator");eraseGeneratorControlWindow(job.trackID);
    for(auto* plugin:track->pluginList)if(job.metadata.getChildWithProperty("sourceID",plugin->itemID.toString()).isValid())closeEffectEditor(*plugin);
    if(bool(job.metadata.getProperty("bakeMixer")) && equaliserWindow)equaliserWindow->setVisible(false);
    track->state.addChild(job.metadata.createCopy(),-1,nullptr);isDirty=true;
    refreshEnginePlaylist();
    if(!isGeneratorFrozen(job.trackID)){undoHistory.pop_back();error="Frozen audio could not be attached. Original generator restored.";return false;}
    readPatternSteps();updateActiveInstrumentEditor();
    refreshProjectView("Generator frozen for Song. "+juce::String(bool(job.metadata.getProperty("bakeMixer"))?"Mixer inserts and EQ are baked. ":"Mixer effects remain live. ")+"Pattern audition "+juce::String(bool(job.metadata.getProperty("keepLivePattern"))?"is available.":"is off. Unfreeze to audition."));return true;
}
void cancelFreeze() { freezeCancelled=true; }
void finishFreeze(const juce::String& renderError)
{
    freezePump.reset();freezeTask.reset();freezeRenderStatus.reset();if(!freezeJob)return;
    if(freezeProgress){freezeProgress->exitModalState(1);freezeProgress.reset();}
    juce::String error=renderError;
    if(freezeCancelled)error="Freeze cancelled. Original generator unchanged.";
    if(error.isEmpty())commitFreeze(*freezeJob,error);
    if(error.isNotEmpty())status.setText(error,juce::dontSendNotification);
    freezeJob.reset();freezeCancelled=false;
}
void processFreezeBatch()
{
    if(!freezeTask || !freezeJob)return;
    if(freezeCancelled){finishFreeze({});return;}
    const auto deadline=juce::Time::getMillisecondCounterHiRes()+15.;
    do
    {
        if(freezeTask->runJob()!=juce::ThreadPoolJob::jobNeedsRunningAgain)
        {const auto error=freezeTask->errorMessage;finishFreeze(error);return;}
    }while(juce::Time::getMillisecondCounterHiRes()<deadline && !freezeCancelled);
    freezeProgressValue=freezeRenderProgress.load();
}
void beginFreezeRender()
{
    if(!freezeJob || freezeTask)return;
    freezeCancelled=false;freezeRenderProgress=0;freezeProgressValue=0;
    freezeRenderStatus=std::make_unique<te::Edit::ScopedRenderStatus>(*freezeJob->renderEdit,false);
    freezeTask=std::make_unique<te::Renderer::RenderTask>("Freeze Song",freezeRenderParameters(*freezeJob),&freezeRenderProgress,nullptr);
    freezePump=std::make_unique<FreezePump>();const juce::Component::SafePointer<ProjectView> owner(this);
    freezePump->action=[owner]{if(owner)owner->processFreezeBatch();};freezePump->startTimer(1);
}
void startFreeze(const juce::String& id,bool keepLive,double tail,bool bakeMixer=false,bool permanent=false)
{
    if(freezeJob || edit==nullptr)return;
    if(!finishMidiRecording())return;
    edit->getTransport().stop(false,false);releaseLiveMidi();recordArmed=false;
    juce::String error;auto job=prepareFreeze(id,keepLive,tail,error,bakeMixer,permanent);
    if(!job){status.setText(error,juce::dontSendNotification);return;}
    freezeJob=std::move(job);freezeCancelled=false;freezeProgressValue=0;
    freezeProgress=std::make_unique<orbit::workspace::OwnedAlert>("Freezing generator",permanent?"Rendering before permanent audio conversion. Source is kept intact until the WAV is complete.":"Rendering the Song and effects tail. Original source is preserved until the WAV is complete.",juce::MessageBoxIconType::NoIcon,this);
    freezeProgress->addProgressBarComponent(freezeProgressValue);freezeProgress->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));freezeProgress->centreAroundComponent(this,560,180);
    const juce::Component::SafePointer<ProjectView> owner(this);
    freezeProgress->enterModalState(true,juce::ModalCallbackFunction::create([owner](int result){if(owner && result==0)owner->cancelFreeze();}),false);
    beginFreezeRender();
}

void showFreezeOptions(const juce::String& id)
{
    if(freezeJob || isGeneratorFrozen(id))return;
    auto* track=engineTrackForProjectId(id);if(!track)return;
    auto panel=std::make_unique<OptionsPanel>("Print this generator's Song and internal FX. Optionally bake its mixer inserts and EQ. Fader, pan, sends, buses and Master remain live. Permanent conversion discards the source and clears Undo history after a successful render.");
    auto& action=panel->choice("action","After rendering",{"Reversible Freeze","Permanent Audio - discard source / Undo"},0);
    panel->check("mixer","Bake mixer inserts and EQ",false);
    panel->check("live","Keep live Pattern audition (reversible only)",true);
    panel->number("tail","Release / FX tail (seconds)",10,0,600,.5);
    panel->apply.setButtonText("Freeze");
    const juce::Component::SafePointer<OptionsPanel> form(panel.get());
    action.onChange=[form]
    {
        if(!form)return;const bool permanent=form->index("action")==1;
        form->checks.at("mixer")->setEnabled(!permanent);form->checks.at("live")->setEnabled(!permanent);
        if(permanent){form->checks.at("mixer")->setToggleState(true,juce::dontSendNotification);form->checks.at("live")->setToggleState(false,juce::dontSendNotification);}
        form->apply.setButtonText(permanent?"Convert Permanently":"Freeze");
    };
    openOptionsTool("Freeze Song - "+track->getName(),std::move(panel),[this,id](const auto& p)
    {
        const bool live=p.checked("live"),bake=p.checked("mixer"),permanent=p.index("action")==1;const double tail=p.value("tail");const juce::Component::SafePointer<ProjectView> owner(this);
        juce::MessageManager::callAsync([owner,id,live,tail,bake,permanent]{if(owner)owner->startFreeze(id,live,tail,bake,permanent);});return true;
    },true);
}
