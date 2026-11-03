// Included inside ProjectView. Resolve instance IDs before touching either the
// document or native clips. Shared bus/Master lanes have no selected row and
// deliberately stay in Song time.
bool stagePlaylistSongEvents(ProjectDocument& staged,
    const std::map<int,std::vector<orbit::playlist::EventRange>>& selectedRanges,
    int rowDelta,double barDelta,bool copy,bool generators,bool mixer,juce::String& error,const ProjectDocument* sourceDocument=nullptr)
{
    const auto& sourceDoc=sourceDocument?*sourceDocument:project;
    using Range=orbit::playlist::EventRange;
    using Paste=orbit::playlist::EventPaste;
    using Lane=ProjectDocument::AutomationLane;
    using Key=std::tuple<juce::String,juce::String,juce::String>;
    struct Edit { juce::String owner;Lane original;float baseline=0.5f;std::vector<Range> cuts;std::vector<Paste> pastes; };
    std::map<Key,Edit> edits;
    const auto modules=songEventModules();
    const auto meter=project.getInitialTimeSignature();
    const double delta=barDelta*meter.numerator*16.0/juce::jmax(1,meter.denominator);
    auto plugins=[&](const juce::String& owner)
    {for(const auto& module:modules)if(module.id==owner)return songModulePlugins(module);return juce::Array<te::Plugin*>{};};
    auto parameter=[](te::Plugin* plugin,const juce::String& id)->te::AutomatableParameter*
    {if(plugin)for(auto* p:plugin->getAutomatableParameters())if(p!=nullptr && p->paramID==id)return p;return nullptr;};
    auto equivalent=[](te::Plugin* a,te::Plugin* b)
    {
        if(a->getIdentifierString()!=b->getIdentifierString() || a->getPluginType()!=b->getPluginType())return false;
        if(auto* send=dynamic_cast<te::AuxSendPlugin*>(a))
        {auto* other=dynamic_cast<te::AuxSendPlugin*>(b);return other!=nullptr && send->busNumber.get()==other->busNumber.get();}
        return true;
    };
    auto editFor=[&](const juce::String& owner,const Lane& lane,te::Plugin* plugin)->Edit&
    {
        const Key key{owner,lane.pluginId.isEmpty()?lane.pluginName:lane.pluginId,lane.parameterId};
        if(auto i=edits.find(key);i!=edits.end())return i->second;
        Edit edit;edit.owner=owner;edit.original=lane;
        if(lane.pluginName=="MIDI")edit.baseline=lane.parameterId=="Pitch"?0.5f:0.0f;
        if(auto* p=parameter(plugin,lane.parameterId))
        {
            const auto base=plugin->state.getChildWithName("ORBITAUTOMATIONBASES").getChildWithProperty("parameterId",p->paramID);
            edit.baseline=float(base.getProperty("value",p->getCurrentNormalisedValue()));
        }
        return edits.emplace(key,std::move(edit)).first->second;
    };
    for(const auto& [row,rawRanges]:selectedRanges)
    {
        const auto ranges=orbit::playlist::mergeEventRanges(rawRanges);
        const auto owner=sourceDoc.getTrackId(row),dest=project.getTrackId(row+rowDelta);
        const auto sourcePlugins=plugins(owner),destPlugins=plugins(dest);
        auto* generator=generatorControlPlugin(owner);
        for(int index=0;index<sourceDoc.getSongAutomationLaneCount(owner);++index)
        {
            auto source=sourceDoc.getSongAutomationLane(owner,index);
            if(source.points.isEmpty() || isFrozenSourceParameter(owner,source.pluginId,source.pluginName))continue;
            const bool midi=source.pluginName=="MIDI" && source.pluginId.isEmpty();
            te::Plugin* from=nullptr;
            if(!midi)
            {
                for(auto* p:sourcePlugins)
                    if((source.pluginId.isNotEmpty()?p->itemID.toString()==source.pluginId:p->getName()==source.pluginName) && parameter(p,source.parameterId))
                    {if(from!=nullptr){error="A name-only Song lane is ambiguous. Choose its plugin instance in the Event Editor first.";return false;}from=p;}
                if(from==nullptr){error="A Song automation target is missing. Turn off the event-copy options to move clips alone.";return false;}
                // An exact lane supersedes the historical name-only lane.
                if(source.pluginId.isEmpty())
                {
                    bool superseded=false;
                    for(int n=0;n<sourceDoc.getSongAutomationLaneCount(owner);++n)
                    {const auto lane=sourceDoc.getSongAutomationLane(owner,n);superseded|=lane.pluginId==from->itemID.toString() && lane.parameterId==source.parameterId;}
                    if(superseded)continue;
                }
            }
            const bool instrument=midi || from==generator;
            if(instrument?!generators:!mixer)continue;
            te::Plugin* to=from;
            if(owner!=dest && !midi)
            {
                if(instrument)
                {to=generatorControlPlugin(dest);if(to!=nullptr && !equivalent(from,to))to=nullptr;}
                else
                {
                    to=nullptr;int sources=0,destinations=0;
                    for(auto* p:sourcePlugins)if(p!=generator && equivalent(from,p))++sources;
                    for(auto* p:destPlugins)if(p!=generatorControlPlugin(dest) && equivalent(from,p)){to=p;++destinations;}
                    if(sources!=1 || destinations!=1)to=nullptr;
                }
                if(to==nullptr || parameter(to,source.parameterId)==nullptr)
                {error="Song events need a matching, unambiguous plugin and parameter on the destination track. Move within this track, or disable that event option.";return false;}
            }
            // Promote resolved legacy lanes to exact IDs. Existing instance
            // curves remain authoritative and duplicate inserts never collapse.
            if(from)source.pluginId=from->itemID.toString();
            Lane destination=source;
            if(to){destination.pluginId=to->itemID.toString();destination.pluginName=to->getName();}
            destination.points.clear();
            if(midi)
            {
                for(int n=0;n<project.getSongAutomationLaneCount(dest);++n)
                {auto lane=project.getSongAutomationLane(dest,n);if(lane.pluginName=="MIDI" && lane.parameterId==source.parameterId){destination.points=lane.points;break;}}
            }
            else destination.points=songEventPoints({dest,destination.pluginId,destination.pluginName,destination.parameterId,destination.parameterName,{}});
            // A new Song lane takes precedence over arranged Pattern curves.
            // Do not silently disable those curves outside the copied block.
            if(owner!=dest && !midi && destination.points.isEmpty())
                for(int slot=0;slot<ProjectDocument::patternSlotCount;++slot)
                    for(int n=0;n<project.getPatternAutomationLaneCount(dest,slot);++n)
                    {
                        const auto lane=project.getPatternAutomationLane(dest,slot,n);
                        if(lane.pluginName==destination.pluginName && lane.parameterId==destination.parameterId && !lane.points.isEmpty())
                        {error="The destination parameter already uses Pattern automation. Disable this event option or choose a track with a Song lane for that parameter.";return false;}
                    }
            auto& sourceEdit=editFor(owner,source,from);
            auto& destEdit=editFor(dest,destination,to);
            for(auto range:ranges)
            {
                if(!copy)sourceEdit.cuts.push_back(range);
                const Range target{range.start+delta,range.end+delta};
                for(const auto& paste:destEdit.pastes)
                    if(target.start<paste.destination.end && target.end>paste.destination.start)
                    {error="Selected Song event destinations overlap. Select non-overlapping clips and try again.";return false;}
                destEdit.pastes.push_back({target,delta,source});
            }
        }
    }
    for(auto& [key,edit]:edits)
    {
        if(edit.cuts.empty() && edit.pastes.empty())continue;
        const auto points=orbit::playlist::editSongEventBlocks(edit.original,edit.baseline,
            orbit::playlist::mergeEventRanges(edit.cuts),edit.pastes);
        staged.replaceSongAutomationPoints(edit.owner,edit.original.pluginName,edit.original.parameterId,
            edit.original.parameterName,points,edit.original.pluginId);
    }
    return true;
}
