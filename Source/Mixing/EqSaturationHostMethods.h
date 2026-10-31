te::PluginList* mixerPluginList(int index)
{
    if(edit==nullptr)return nullptr;const auto tracks=getMixerTracks();
    if(index==tracks.size())return &edit->getMasterPluginList();
    return juce::isPositiveAndBelow(index,tracks.size())?&tracks[index]->pluginList:nullptr;
}
orbit::mixing::EqSaturationPlugin* getTrackEqSaturationPlugin(int index)
{
    if(auto* list=mixerPluginList(index))for(auto* plugin:*list)
        if(auto* saturation=dynamic_cast<orbit::mixing::EqSaturationPlugin*>(plugin))return saturation;
    return nullptr;
}
bool getTrackEqSaturation(int index)
{
    auto* plugin=getTrackEqSaturationPlugin(index);return plugin!=nullptr && bool(plugin->state.getProperty("orbitEqSaturationOn",false));
}
void syncTrackEqSaturation(int index)
{
    if(auto* saturation=getTrackEqSaturationPlugin(index))
    {auto* eq=getTrackEqualiser(index);saturation->setEnabled(eq!=nullptr && eq->isEnabled() && getTrackEqSaturation(index));}
}
void setTrackEqSaturation(int index,bool enabled,bool undo=true)
{
    auto* list=mixerPluginList(index);if(list==nullptr || enabled==getTrackEqSaturation(index))return;
    if(undo)rememberUndo("Change EQ soft saturation");
    auto* eq=enabled?ensureTrackEqualiser(index):getTrackEqualiser(index);if(eq==nullptr)return;
    auto* saturation=getTrackEqSaturationPlugin(index);
    if(saturation==nullptr && enabled)
    {
        auto plugin=edit->getPluginCache().createNewPlugin(orbit::mixing::EqSaturationPlugin::xmlTypeName,{});
        saturation=dynamic_cast<orbit::mixing::EqSaturationPlugin*>(plugin.get());if(saturation==nullptr)return;
        list->insertPlugin(plugin,list->indexOf(eq)+1,nullptr);
    }
    if(saturation!=nullptr)saturation->state.setProperty("orbitEqSaturationOn",enabled,&edit->getUndoManager());
    syncTrackEqSaturation(index);isDirty=true;
    status.setText(enabled?"EQ soft saturation enabled.":"EQ soft saturation disabled.",juce::dontSendNotification);
}
