int masterInsertEnd() const
{
    if(edit==nullptr)return 0;const auto& list=edit->getMasterPluginList();
    for(auto* plugin:list)if(bool(plugin->state.getProperty("orbitMasterMuteGate",false)))return list.indexOf(plugin);
    return list.size();
}
void ensureMasterMuteGate(bool restoreLegacyGain=true)
{
    if(edit==nullptr)return;auto& list=edit->getMasterPluginList();
    std::vector<juce::ValueTree> gates;for(auto* plugin:list)if(bool(plugin->state.getProperty("orbitMasterMuteGate",false)))gates.push_back(plugin->state);
    if(!isMasterMuted())
    {for(auto state:gates)state.getParent().removeChild(state,&edit->getUndoManager());return;}
    if(gates.empty())
    {
        if(restoreLegacyGain && masterVolumePlugin()!=nullptr && masterVolumePlugin()->getVolumeDb()<=-90
            && edit->state.hasProperty(masterPreMuteGainProperty()))
            masterVolumePlugin()->setVolumeDb(float(edit->state.getProperty(masterPreMuteGainProperty())));
        auto plugin=edit->getPluginCache().createNewPlugin(orbit::mixing::OutputGatePlugin::xmlTypeName,{});
        if(plugin!=nullptr){plugin->state.setProperty("orbitMasterMuteGate",true,nullptr);list.insertPlugin(plugin,list.size(),nullptr);}
    }
    else
    {
        auto state=gates.front(),parent=state.getParent();const auto at=parent.indexOf(state);
        if(at!=parent.getNumChildren()-1)parent.moveChild(at,parent.getNumChildren()-1,&edit->getUndoManager());
        for(size_t i=1;i<gates.size();++i)gates[i].getParent().removeChild(gates[i],&edit->getUndoManager());
    }
}
