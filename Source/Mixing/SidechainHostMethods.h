// Included inside ProjectView. Resolve IDs again after every asynchronous menu.
te::Plugin* sidechainPluginById(const juce::String& id)
{
    for(const auto& module:songEventModules())for(auto* plugin:songModulePlugins(module))if(plugin->itemID.toString()==id)return plugin;
    return nullptr;
}
bool setEffectSidechain(const juce::String& pluginId,const juce::String& sourceId)
{
    auto* plugin=sidechainPluginById(pluginId);if(plugin==nullptr || !plugin->canSidechain())return false;
    te::AudioTrack* source=nullptr;
    if(sourceId.isNotEmpty())for(auto* track:te::getAudioTracks(*edit))if(track->itemID.toString()==sourceId){source=track;break;}
    if(sourceId.isNotEmpty() && source==nullptr)return false;
    auto* destination=dynamic_cast<te::AudioTrack*>(plugin->getOwnerTrack());
    if(source!=nullptr && audioRouteWouldCycle(source,destination,plugin))
    {status.setText("Sidechain unchanged: this source would create audio feedback.",juce::dontSendNotification);return false;}
    auto* compressor=dynamic_cast<te::CompressorPlugin*>(plugin);
    const auto id=source!=nullptr?source->itemID:te::EditItemID{};
    if(plugin->getSidechainSourceID()==id && (compressor==nullptr || compressor->useSidechainTrigger.get()==(source!=nullptr)))return true;
    rememberUndo("Change effect sidechain");plugin->setSidechainSourceID(id);
    if(source!=nullptr && plugin->getNumWires()==0)plugin->guessSidechainRouting();
    if(compressor!=nullptr)compressor->useSidechainTrigger=source!=nullptr;
    isDirty=true;edit->restartPlayback();
    if(mixerWindow!=nullptr)mixerWindow->refreshEffectSidechain();
    status.setText(source!=nullptr?"Sidechain source: "+source->getName():"External sidechain disconnected.",juce::dontSendNotification);
    return true;
}
void showEffectSidechainMenu(te::Plugin& requested,juce::Component* anchor)
{
    const auto pluginId=requested.itemID.toString(),projectId=project.getProjectId();
    auto* plugin=sidechainPluginById(pluginId);if(plugin==nullptr || !plugin->canSidechain())return;
    auto* destination=dynamic_cast<te::AudioTrack*>(plugin->getOwnerTrack());
    juce::PopupMenu menu;menu.addItem(1,"None",true,!plugin->getSidechainSourceID().isValid());
    std::vector<juce::String> ids;
    for(auto* track:getMixerTracks())
    {
        ids.push_back(track->itemID.toString());const bool safe=!audioRouteWouldCycle(track,destination,plugin);
        menu.addItem(int(ids.size())+1,juce::String(ids.size())+". "+track->getName(),safe,plugin->getSidechainSourceID()==track->itemID);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),[safe=juce::Component::SafePointer<ProjectView>(this),projectId,pluginId,ids=std::move(ids)](int choice)
    {
        if(safe==nullptr || safe->project.getProjectId()!=projectId)return;
        if(choice==1)safe->setEffectSidechain(pluginId,{});
        else if(juce::isPositiveAndBelow(choice-2,int(ids.size())))safe->setEffectSidechain(pluginId,ids[size_t(choice-2)]);
    });
}
