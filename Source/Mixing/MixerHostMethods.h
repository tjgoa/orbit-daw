// Included inside ProjectView. Clipboard contains values, never plugin pointers.
struct MixerSettings
{
    double gain=0,pan=0;
    bool mute=false,solo=false,eqPresent=false,eqEnabled=false,eqSaturation=false,generator=false;
    std::array<float,12> eq{};
    std::array<double,8> sends{};
    std::array<bool,8> linked{};
};
std::optional<MixerSettings> mixerSettingsClipboard;
bool mixerClipboardEqOnly=false;

juce::String mixerOwnerId(int index)
{
    if(edit==nullptr)return {};
    const auto tracks=getMixerTracks();
    if(index==tracks.size())return "mixer:master";
    if(!juce::isPositiveAndBelow(index,tracks.size()))return {};
    return tracks[index]->itemID.toString();
}
int mixerIndexForOwner(const juce::String& id)
{
    if(edit==nullptr || id.isEmpty())return -1;
    const auto tracks=getMixerTracks();
    if(id=="mixer:master")return tracks.size();
    for(int i=0;i<tracks.size();++i)if(tracks[i]->itemID.toString()==id)return i;
    return -1;
}
bool getTrackSendLinked(int index,int bus)
{
    const auto tracks=getGeneratorTracks();
    if(!juce::isPositiveAndBelow(index,tracks.size()) || !juce::isPositiveAndBelow(bus,8))return true;
    return !bool(tracks[index]->state.getProperty("orbitSendPreFader"+juce::String(bus),false));
}
void applyTrackSendLink(int index,int bus,bool linked)
{
    const auto tracks=getGeneratorTracks();
    if(!juce::isPositiveAndBelow(index,tracks.size()) || !juce::isPositiveAndBelow(bus,8))return;
    auto* track=tracks[index];
    track->state.setProperty("orbitSendPreFader"+juce::String(bus),!linked,&edit->getUndoManager());
    if(auto* send=track->getAuxSendPlugin(bus))if(auto* volume=track->getVolumePlugin())
    {
        auto parent=send->state.getParent();
        if(parent==volume->state.getParent())
        {
            const auto from=parent.indexOf(send->state),at=parent.indexOf(volume->state);
            if((linked && from<at) || (!linked && from>at))parent.moveChild(from,at,&edit->getUndoManager());
        }
    }
    isDirty=true;
}
void setTrackSendLinked(int index,int bus,bool linked)
{
    if(edit==nullptr || !juce::isPositiveAndBelow(index,getGeneratorTracks().size()) || !juce::isPositiveAndBelow(bus,8) || getTrackSendLinked(index,bus)==linked)return;
    rememberUndo("Change send routing");applyTrackSendLink(index,bus,linked);
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
    status.setText(linked?"Send follows channel volume and pan.":"Send is independent of channel volume and pan.",juce::dontSendNotification);
}
void copyMixerSettings(int index,bool eqOnly)
{
    if(mixerOwnerId(index).isEmpty())return;
    const bool master=index==getMixerTracks().size();MixerSettings c;
    c.gain=master?getMasterGainDb():getTrackGainDb(index);
    c.pan=master?(masterVolumePlugin()!=nullptr?masterVolumePlugin()->getPan():0):getTrackPan(index);
    c.mute=master?isMasterMuted():getTrackMute(index);c.solo=!master && getTrackSolo(index);
    auto* eq=getTrackEqualiser(index);c.eqPresent=eq!=nullptr;c.eqEnabled=eq!=nullptr && eq->isEnabled();c.eqSaturation=getTrackEqSaturation(index);
    for(int p=0;p<12;++p)c.eq[size_t(p)]=getTrackEqParameter(index,p);
    c.generator=index<getGeneratorTracks().size();
    for(int bus=0;bus<8;++bus){c.sends[size_t(bus)]=c.generator?getTrackSendGainDb(index,bus):-60;c.linked[size_t(bus)]=getTrackSendLinked(index,bus);}
    mixerSettingsClipboard=c;mixerClipboardEqOnly=eqOnly;
    status.setText(eqOnly?"EQ settings copied.":"Channel settings copied (level, pan, mute/solo, EQ and sends).",juce::dontSendNotification);
}
void pasteMixerSettings(int index,bool eqOnly)
{
    if(!mixerSettingsClipboard || mixerOwnerId(index).isEmpty() || (!eqOnly && mixerClipboardEqOnly))return;
    const auto c=*mixerSettingsClipboard;rememberUndo(eqOnly?"Paste EQ settings":"Paste channel settings");
    if(c.eqPresent)
    {
        for(int p=0;p<12;++p)setTrackEqParameter(index,p,c.eq[size_t(p)]);
        setTrackEqSaturation(index,c.eqSaturation,false);setTrackEqEnabled(index,c.eqEnabled);
    }
    else if(getTrackEqualiser(index)!=nullptr){setTrackEqSaturation(index,false,false);setTrackEqEnabled(index,false);}
    if(!eqOnly)
    {
        if(index==getMixerTracks().size()){setMasterGainDb(c.gain);setMasterPan(c.pan);setMasterMuted(c.mute);}
        else
        {
            setTrackGainDb(index,c.gain);setTrackPan(index,c.pan);setTrackMute(index,c.mute);setTrackSolo(index,c.solo);
            if(c.generator && index<getGeneratorTracks().size())for(int bus=0;bus<8;++bus)
            {applyTrackSendLink(index,bus,c.linked[size_t(bus)]);setTrackSendGainDb(index,bus,c.sends[size_t(bus)]);}
        }
    }
    isDirty=true;refreshMixerControls();
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
    if(equaliserWindow!=nullptr)equaliserWindow->refreshParameterValues(index);
    refreshSongEventEditor();
    status.setText(eqOnly?"EQ settings pasted. Undo restores the previous settings.":"Channel settings pasted. Instruments, inserts, routing destinations and event lanes are retained.",juce::dontSendNotification);
}
void renameMixerChannel(int index,const juce::String& name)
{
    const auto tracks=getMixerTracks();const auto cleaned=name.trim();
    if(!juce::isPositiveAndBelow(index,tracks.size()) || cleaned.isEmpty() || tracks[index]->getName()==cleaned)return;
    rememberUndo("Rename mixer channel");tracks[index]->setName(cleaned);
    if(index<project.getTrackCount())project.setTrackName(index,cleaned);
    syncTrackMixerState(tracks[index]);isDirty=true;refreshTrackSelector();refreshMixerControls();
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
    refreshSongEventEditor();
}
void promptRenameMixerChannel(const juce::String& owner,const juce::String& projectId)
{
    const auto index=mixerIndexForOwner(owner);const auto tracks=getMixerTracks();
    if(!juce::isPositiveAndBelow(index,tracks.size()))return;
    juce::AlertWindow dialog("Rename Channel","Enter the channel name.",juce::MessageBoxIconType::NoIcon);
    dialog.addTextEditor("name",tracks[index]->getName());dialog.addButton("Rename",1,juce::KeyPress(juce::KeyPress::returnKey));dialog.addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    if(dialog.runModalLoop()==1 && project.getProjectId()==projectId)renameMixerChannel(mixerIndexForOwner(owner),dialog.getTextEditorContents("name"));
}
void showMixerSettingsMenu(int index,juce::Component* anchor)
{
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();if(owner.isEmpty())return;
    juce::PopupMenu menu;menu.addItem(1,"Copy Channel Settings");menu.addItem(2,"Paste Channel Settings",mixerSettingsClipboard.has_value()&&!mixerClipboardEqOnly);
    if(owner=="mixer:master")menu.addItem(6,"Volume Before Insert Effects",true,isMasterVolumeBeforeInserts());
    menu.addSeparator();menu.addItem(7,"Copy Insert Effects");menu.addItem(8,"Paste Insert Effects (Replace)",effectClipboard.has_value());
    menu.addSeparator();menu.addItem(5,"Rename Channel...",owner!="mixer:master");menu.addSeparator();menu.addItem(3,"Copy EQ Settings");menu.addItem(4,"Paste EQ Settings",mixerSettingsClipboard.has_value());
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),[safe=juce::Component::SafePointer<ProjectView>(this),owner,projectId](int choice)
    {
        if(safe==nullptr || safe->project.getProjectId()!=projectId)return;
        const auto index=safe->mixerIndexForOwner(owner);if(index<0)return;
        if(choice==7)safe->copyInsertEffects(index);
        if(choice==8)safe->pasteInsertEffects(index,true);
        if(choice==5)safe->promptRenameMixerChannel(owner,projectId);
        if(choice==6 && owner=="mixer:master")safe->setMasterVolumeBeforeInserts(!safe->isMasterVolumeBeforeInserts());
        if(choice==1 || choice==3)safe->copyMixerSettings(index,choice==3);
        if(choice==2 || choice==4)safe->pasteMixerSettings(index,choice==4);
    });
}
te::AutomatableParameter* mixerEqParameter(int index,int parameter,bool create)
{
    if(!juce::isPositiveAndBelow(parameter,12))return nullptr;
    auto* eq=create?ensureTrackEqualiser(index):getTrackEqualiser(index);if(eq==nullptr)return nullptr;
    const std::array<te::AutomatableParameter*,12> parameters{eq->loFreq.get(),eq->loGain.get(),eq->loQ.get(),
        eq->midFreq1.get(),eq->midGain1.get(),eq->midQ1.get(),eq->midFreq2.get(),eq->midGain2.get(),eq->midQ2.get(),eq->hiFreq.get(),eq->hiGain.get(),eq->hiQ.get()};
    return parameters[size_t(parameter)];
}
void showMixerEqEventMenu(int index,int parameter,juce::Component* anchor)
{
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();if(owner.isEmpty())return;
    juce::PopupMenu menu;menu.addItem(1,"Edit Song Events");menu.addItem(2,"Edit Pattern Events",index<getGeneratorTracks().size());
    menu.addSeparator();menu.addItem(3,"Enable Soft Saturation",true,getTrackEqSaturation(index));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),[safe=juce::Component::SafePointer<ProjectView>(this),owner,projectId,parameter](int choice)
    {
        if(safe==nullptr || safe->project.getProjectId()!=projectId || (choice!=1 && choice!=2 && choice!=3))return;
        const auto index=safe->mixerIndexForOwner(owner);if(index<0)return;
        if(choice==3){safe->setTrackEqSaturation(index,!safe->getTrackEqSaturation(index));return;}
        if(safe->getTrackEqualiser(index)==nullptr)safe->rememberUndo("Create EQ event target");
        if(auto* p=safe->mixerEqParameter(index,parameter,true))safe->openParameterEvents(safe->targetForParameter(*p),choice==2);
    });
}
juce::PopupMenu makeMixerSendMenu(int index,int bus,const juce::Array<juce::PluginDescription>& external)
{
    juce::PopupMenu menu;auto* track=getFxReturnTrack(bus);
    const int returnIndex=track!=nullptr?getMixerTracks().indexOf(track):-1;
    addDirectEffectChoices(menu,external,returnIndex<0 || getInsertPlugins(returnIndex).size()<maxInsertSlots(returnIndex));
    menu.addSeparator();menu.addItem(4,"Open FX "+juce::String(bus+1)+" Return Effects",returnIndex>=0 && !getInsertPlugins(returnIndex).isEmpty());
    menu.addItem(1,"Linked to Channel Volume",true,getTrackSendLinked(index,bus));
    auto* send=getGeneratorTracks()[index]->getAuxSendPlugin(bus);
    menu.addSeparator();menu.addItem(2,"Edit Song Events",send!=nullptr);menu.addItem(3,"Edit Pattern Events",send!=nullptr);
    return menu;
}
void handleMixerSendChoice(const juce::String& owner,const juce::String& projectId,int bus,
                           const juce::Array<juce::PluginDescription>& external,int choice)
{
    if(project.getProjectId()!=projectId || !juce::isPositiveAndBelow(bus,8))return;
    const int index=mixerIndexForOwner(owner);if(!juce::isPositiveAndBelow(index,getGeneratorTracks().size()))return;
    if(choice==1)setTrackSendLinked(index,bus,!getTrackSendLinked(index,bus));
    if(choice==2 || choice==3)if(auto* send=getGeneratorTracks()[index]->getAuxSendPlugin(bus))openParameterEvents(targetForParameter(*send->gain),choice==3);
    if(choice==4){if(auto* track=getFxReturnTrack(bus)){const int row=getMixerTracks().indexOf(track);const auto plugins=getInsertPlugins(row);for(int slot=0;slot<plugins.size();++slot)openInsertEditor(row,slot);}return;}
    if((choice>=101 && choice<=110) || choice==99 || juce::isPositiveAndBelow(choice-1000,external.size()))
    {
        auto* track=ensureFxReturnTrack(bus);if(track==nullptr)return;const int row=getMixerTracks().indexOf(track);
        if(choice>=101 && choice<=110)addInsert(row,choice-100);
        else if(choice==99)showVst3EffectBrowser(mixerOwnerId(row));
        else addVst3Effect(row,external[choice-1000]);
    }
}
void showMixerSendMenu(int index,int bus,juce::Component* anchor)
{
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();
    if(owner.isEmpty() || !juce::isPositiveAndBelow(index,getGeneratorTracks().size()) || !juce::isPositiveAndBelow(bus,8))return;
    const auto external=knownVst3Effects();auto menu=makeMixerSendMenu(index,bus,external);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),[safe=juce::Component::SafePointer<ProjectView>(this),owner,projectId,bus,external](int choice)
    {if(safe!=nullptr)safe->handleMixerSendChoice(owner,projectId,bus,external,choice);});
}
