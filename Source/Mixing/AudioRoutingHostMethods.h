// Included inside ProjectView. Native graph edges are used for feedback checks.
bool audioRouteWouldCycle(te::AudioTrack* source,te::AudioTrack* destination,te::Plugin* ignored=nullptr)
{
    if(source==nullptr || destination==nullptr)return false;
    std::vector<te::AudioTrack*> pending{destination};std::set<te::AudioTrack*> visited;
    const auto tracks=te::getAudioTracks(*edit);
    while(!pending.empty())
    {
        auto* track=pending.back();pending.pop_back();if(track==source)return true;
        if(!visited.insert(track).second)continue;
        if(auto* next=track->getOutput().getDestinationTrack())pending.push_back(next);
        for(auto* plugin:track->pluginList)if(plugin!=ignored && plugin->isEnabled())
            if(auto* send=dynamic_cast<te::AuxSendPlugin*>(plugin))
                for(auto* receiver:tracks)for(auto* p:receiver->pluginList)
                    if(auto* ret=dynamic_cast<te::AuxReturnPlugin*>(p);ret!=nullptr && ret->isEnabled() && ret->busNumber.get()==send->busNumber.get())pending.push_back(receiver);
        for(auto* receiver:tracks)for(auto* plugin:receiver->pluginList)
            if(plugin!=ignored && plugin->isEnabled() && plugin->getSidechainSourceID()==track->itemID)pending.push_back(receiver);
    }
    return false;
}
bool isOutputBusUsed(int bus)
{
    auto* target=getOutputBusTrack(bus);if(target==nullptr)return false;
    for(auto* source:te::getAudioTracks(*edit))if(source!=target)
    {
        if(source->getOutput().getDestinationTrack()==target)return true;
        for(auto* p:source->pluginList)if(auto* send=dynamic_cast<te::AuxSendPlugin*>(p);send!=nullptr && send->isEnabled() && send->busNumber.get()==32+bus)return true;
    }
    return false;
}
int getTrackOutputRoutes(int index)
{
    const auto tracks=getMixerTracks();if(!juce::isPositiveAndBelow(index,tracks.size()))return 0;
    auto* track=tracks[index];
    if(track->state.hasProperty("orbitOutputRouteMask"))return int(track->state.getProperty("orbitOutputRouteMask"))&511;
    if(auto* destination=track->getOutput().getDestinationTrack())
    {
        const int bus=int(destination->state.getProperty("orbitMixerBus",-1));
        return juce::isPositiveAndBelow(bus,8)?1<<(bus+1):0;
    }
    return track->getOutput().usesDefaultAudioOut()?1:0;
}
void ensureOutputBusReturn(int bus)
{
    auto* track=ensureOutputBusTrack(bus);if(track==nullptr)return;
    for(auto* p:track->pluginList)if(auto* ret=dynamic_cast<te::AuxReturnPlugin*>(p);ret!=nullptr && ret->busNumber.get()==32+bus)return;
    auto plugin=edit->getPluginCache().createNewPlugin(te::AuxReturnPlugin::xmlTypeName,{});
    if(auto* ret=dynamic_cast<te::AuxReturnPlugin*>(plugin.get()))
    {ret->busNumber=32+bus;ret->state.setProperty("orbitOutputBusReturn",true,nullptr);track->pluginList.insertPlugin(plugin,0,nullptr);}
}
bool setTrackOutputRoutes(int index,int routes,bool undo=true)
{
    const auto tracks=getMixerTracks();if(edit==nullptr || !juce::isPositiveAndBelow(index,tracks.size()) || routes<0 || routes>511)return false;
    auto* source=tracks[index];if(getTrackOutputRoutes(index)==routes)return true;
    for(int bus=0;bus<8;++bus)if(routes&(1<<(bus+1)))
        if(auto* target=getOutputBusTrack(bus);target!=nullptr && audioRouteWouldCycle(source,target))
        {status.setText("Routing unchanged: that destination would create audio feedback.",juce::dontSendNotification);return false;}
    // Check self-routing even when a bus has not previously received a signal.
    const auto selfBus=int(source->state.getProperty("orbitMixerBus",-1));
    if(juce::isPositiveAndBelow(selfBus,8) && (routes&(1<<(selfBus+1))))return false;
    if(undo)rememberUndo("Change channel output routes");
    int primary=-1;if(!(routes&1))for(int bus=0;bus<8;++bus)if(routes&(1<<(bus+1))){primary=bus;break;}
    for(int bus=0;bus<8;++bus)if(routes&(1<<(bus+1)))ensureOutputBusReturn(bus);
    std::vector<juce::ValueTree> remove;std::set<int> existing;
    for(auto* p:source->pluginList)if(auto* send=dynamic_cast<te::AuxSendPlugin*>(p);send!=nullptr && bool(send->state.getProperty("orbitOutputRouteSend",false)))
    {
        const auto bus=send->busNumber.get()-32;
        if(juce::isPositiveAndBelow(bus,8) && (routes&(1<<(bus+1))) && bus!=primary)existing.insert(bus);
        else remove.push_back(send->state);
    }
    for(auto state:remove)state.getParent().removeChild(state,&edit->getUndoManager());
    for(int bus=0;bus<8;++bus)if((routes&(1<<(bus+1))) && bus!=primary && !existing.contains(bus))
    {
        auto plugin=edit->getPluginCache().createNewPlugin(te::AuxSendPlugin::xmlTypeName,{});
        if(auto* send=dynamic_cast<te::AuxSendPlugin*>(plugin.get()))
        {
            send->busNumber=32+bus;send->setGainDb(0);send->state.setProperty("orbitOutputRouteSend",true,nullptr);
            auto* meter=source->getLevelMeterPlugin();const auto at=meter!=nullptr?source->pluginList.indexOf(meter):source->pluginList.size();
            source->pluginList.insertPlugin(plugin,at<0?source->pluginList.size():at,nullptr);
        }
    }
    if(routes&1)source->getOutput().setOutputToDefaultDevice(false);
    else if(primary>=0)source->getOutput().setOutputToTrack(getOutputBusTrack(primary));
    else source->getOutput().setOutputToDefaultDevice(false);
    // An empty native output name falls back to Master. Keep the track in the
    // graph for sends, and silence only its final dry branch after all sends.
    std::vector<juce::ValueTree> dryMutes;
    for(auto* p:source->pluginList)if(bool(p->state.getProperty("orbitRouteDryMute",false)))dryMutes.push_back(p->state);
    if(routes!=0)for(auto state:dryMutes)state.getParent().removeChild(state,&edit->getUndoManager());
    else if(dryMutes.empty())
    {
        auto plugin=edit->getPluginCache().createNewPlugin(orbit::mixing::OutputGatePlugin::xmlTypeName,{});
        if(plugin!=nullptr){plugin->state.setProperty("orbitRouteDryMute",true,nullptr);source->pluginList.insertPlugin(plugin,source->pluginList.size(),nullptr);}
    }
    source->state.setProperty("orbitOutputRouteMask",routes,&edit->getUndoManager());isDirty=true;
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
    refreshSongEventEditor();status.setText(routes==0?"Channel direct output disconnected; FX sends remain available.":"Channel output routes updated. Master and buses can be combined.",juce::dontSendNotification);
    return true;
}
void toggleTrackOutputRoute(int index,int bus)
{
    if(bus < -1 || bus>=8)return;
    setTrackOutputRoutes(index,getTrackOutputRoutes(index)^(1<<(bus+1)));
}

te::VolumeAndPanPlugin* masterVolumePlugin() const
{
    if(edit==nullptr)return nullptr;
    for(auto* plugin:edit->getMasterPluginList())
        if(auto* volume=dynamic_cast<te::VolumeAndPanPlugin*>(plugin);volume!=nullptr && bool(volume->state.getProperty("orbitMasterPreInsert",false)))return volume;
    return edit->getMasterVolumePlugin().get();
}
bool isMasterVolumeBeforeInserts() const
{
    return edit!=nullptr && masterVolumePlugin()!=edit->getMasterVolumePlugin().get();
}
void transferMasterVolumeState(te::VolumeAndPanPlugin& from,te::VolumeAndPanPlugin& to)
{
    to.restorePluginStateFromValueTree(from.state);
    const std::array<te::AutomatableParameter*,2> src{from.volParam.get(),from.panParam.get()},dst{to.volParam.get(),to.panParam.get()};
    for(size_t i=0;i<src.size();++i)
    {
        dst[i]->setParameter(src[i]->getCurrentValue(),juce::dontSendNotification);dst[i]->getCurve().clear();
        for(int point=0;point<src[i]->getCurve().getNumPoints();++point)
            dst[i]->getCurve().addPoint(src[i]->getCurve().getPointTime(point),src[i]->getCurve().getPointValue(point),src[i]->getCurve().getPointCurve(point));
        src[i]->getCurve().clear();
    }
    auto old=to.state.getChildWithName("ORBITAUTOMATIONBASES");if(old.isValid())to.state.removeChild(old,nullptr);
    auto bases=from.state.getChildWithName("ORBITAUTOMATIONBASES");if(bases.isValid())
    {
        auto copy=bases.createCopy();for(auto parameter:copy)for(size_t i=0;i<src.size();++i)
            if(parameter.getProperty("parameterId").toString()==src[i]->paramID)parameter.setProperty("parameterId",dst[i]->paramID,nullptr);
        to.state.addChild(copy,-1,nullptr);
    }
}
bool setMasterVolumeBeforeInserts(bool enabled,bool undo=true)
{
    if(edit==nullptr)return false;if(isMasterVolumeBeforeInserts()==enabled)return true;
    if(!finishMidiRecording())return false;
    if(undo)rememberUndo("Change Master volume order");
    auto* from=masterVolumePlugin();te::VolumeAndPanPlugin::Ptr to;
    const bool legacyVolume=songLegacyTargetIsUnique(targetForParameter(*from->volParam));
    const bool legacyPan=songLegacyTargetIsUnique(targetForParameter(*from->panParam));
    if(enabled)
    {
        auto plugin=edit->getPluginCache().createNewPlugin(te::VolumeAndPanPlugin::xmlTypeName,{});
        to=dynamic_cast<te::VolumeAndPanPlugin*>(plugin.get());if(to==nullptr)return false;
        to->state.setProperty("orbitMasterPreInsert",true,nullptr);edit->getMasterPluginList().insertPlugin(plugin,0,nullptr);
    }
    else to=edit->getMasterVolumePlugin();
    transferMasterVolumeState(*from,*to);
    project.remapSongAutomationParameter("mixer:master",from->getName(),from->itemID.toString(),from->volParam->paramID,to->itemID.toString(),to->volParam->paramID,to->volParam->getParameterName(),legacyVolume);
    project.remapSongAutomationParameter("mixer:master",from->getName(),from->itemID.toString(),from->panParam->paramID,to->itemID.toString(),to->panParam->paramID,to->panParam->getParameterName(),legacyPan);
    if(enabled){from->setVolumeDb(0);from->setPan(0);}
    else{auto state=from->state;state.getParent().removeChild(state,&edit->getUndoManager());}
    ensureMasterMuteGate();syncMasterMixerState();isDirty=true;updateEngineAutomation();refreshSongEventEditor();
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
    status.setText(enabled?"Master volume is before the EQ and insert effects.":"Master volume is after the insert effects.",juce::dontSendNotification);
    return true;
}
