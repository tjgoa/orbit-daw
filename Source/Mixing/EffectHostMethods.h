// Included inside ProjectView. Effect state clipboard contains no live plugin refs.
struct EffectClipboard
{
    juce::String projectId;
    std::vector<juce::ValueTree> states;
    std::vector<juce::String> sidechainSources;
};
std::optional<EffectClipboard> effectClipboard;
juce::Array<juce::PluginDescription> vst3EffectMenuDescriptions;
std::unique_ptr<Vst3InstrumentBrowser> vst3EffectBrowser;
juce::String effectBrowserOwner,effectBrowserProject;

int maxInsertSlots(int index) const
{
    return index<getGeneratorTracks().size()?3:4;
}
juce::Array<juce::PluginDescription> knownVst3Effects()
{
    juce::Array<juce::PluginDescription> result;
    for(const auto& d:engine.getPluginManager().knownPluginList.getTypes())if(d.pluginFormatName=="VST3" && !d.isInstrument)result.add(d);
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.name.compareIgnoreCase(b.name)<0;});return result;
}
int selectedEffectTarget()
{
    const auto tracks=getMixerTracks();const int selected=tracks.indexOf(instrumentTrack);return selected>=0?selected:tracks.size();
}
bool attachInsertEffect(int index,te::Plugin::Ptr plugin,const juce::String& action,const juce::String& replaceId={})
{
    if(plugin==nullptr || mixerOwnerId(index).isEmpty())return false;
    te::Plugin::Ptr replaced;
    for(auto* candidate:getInsertPlugins(index))if(candidate->itemID.toString()==replaceId)replaced=candidate;
    if(replaceId.isNotEmpty() && replaced==nullptr)return false;
    if(replaced==nullptr && getInsertPlugins(index).size()>=maxInsertSlots(index))return false;
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();rememberUndo(action);
    if(project.getProjectId()!=projectId)return false;index=mixerIndexForOwner(owner);auto* list=mixerPluginList(index);if(list==nullptr)return false;
    int at=list->size();if(index==getMixerTracks().size())at=masterInsertEnd();
    else if(auto* volume=getMixerTracks()[index]->getVolumePlugin())at=list->indexOf(volume);
    if(!list->canInsertPlugin())return false;
    if(replaced!=nullptr)at=list->indexOf(replaced.get());
    list->insertPlugin(plugin,at<0?list->size():at,nullptr);if(!list->contains(plugin.get()))return false;
    if(replaced!=nullptr)
    {
        closeEffectEditor(*replaced);auto state=replaced->state;
        if(state.getParent().isValid())state.getParent().removeChild(state,&edit->getUndoManager());
        updateEngineAutomation();
    }
    isDirty=true;
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));refreshSongEventEditor();
    const auto inserts=getInsertPlugins(index);const int slot=inserts.indexOf(plugin.get());
    if(slot>=0)openInsertEditor(index,slot);
    status.setText("Inserted "+plugin->getName()+". Undo removes this effect.",juce::dontSendNotification);return true;
}
bool addVst3Effect(int index,const juce::PluginDescription& description,const juce::String& replaceId={})
{
    if(edit==nullptr || description.pluginFormatName!="VST3" || description.isInstrument || mixerOwnerId(index).isEmpty())return false;
    if(replaceId.isEmpty() && getInsertPlugins(index).size()>=maxInsertSlots(index)){status.setText("All insert slots are occupied. Remove an effect first.",juce::dontSendNotification);return false;}
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();
    engine.getPluginManager().knownPluginList.addType(description);
    auto plugin=edit->getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName,description);
    if(project.getProjectId()!=projectId)return false;
    auto* external=dynamic_cast<te::ExternalPlugin*>(plugin.get());
    if(external==nullptr || external->getAudioPluginInstance()==nullptr || external->isSynth())
    {status.setText("The VST3 effect could not be loaded. No insert was changed.",juce::dontSendNotification);return false;}
    // Fast moduleinfo scans can omit channel counts. Validate the live instance.
    auto* instance=external->getAudioPluginInstance();
    if(instance->getTotalNumInputChannels()<=0 || instance->getTotalNumOutputChannels()<=0)
    {status.setText("This plugin has no active audio input/output. No insert was changed.",juce::dontSendNotification);return false;}
    return attachInsertEffect(mixerIndexForOwner(owner),plugin,replaceId.isEmpty()?"Insert VST3 effect":"Replace VST3 effect",replaceId);
}
void showVst3EffectBrowser(const juce::String& owner)
{
    if(mixerIndexForOwner(owner)<0)return;effectBrowserOwner=owner;effectBrowserProject=project.getProjectId();
    if(vst3EffectBrowser==nullptr)
    {
        auto& plugins=engine.getPluginManager();vst3EffectBrowser=std::make_unique<Vst3InstrumentBrowser>(plugins.pluginFormatManager,plugins.knownPluginList,
            [this]{return getVstSearchPaths();},[this]{showVstPathsDialog();},[this](const juce::PluginDescription& d)
            {if(project.getProjectId()==effectBrowserProject)addVst3Effect(mixerIndexForOwner(effectBrowserOwner),d);},false);
    }
    vst3EffectBrowser->setName("Insert VST3 Effect - "+(owner=="mixer:master"?juce::String("Master"):getMixerTracks()[mixerIndexForOwner(owner)]->getName()));
    vst3EffectBrowser->refreshList();vst3EffectBrowser->setVisible(true);vst3EffectBrowser->toFront(true);
    if(vst3EffectBrowser->needsInitialScan())vst3EffectBrowser->scanNow();
}
void addKnownVst3EffectsToMenu(juce::PopupMenu& menu)
{
    vst3EffectMenuDescriptions=knownVst3Effects();
    if(vst3EffectMenuDescriptions.isEmpty())menu.addItem(409,"No VST3 effects scanned yet",false);
    for(int i=0;i<vst3EffectMenuDescriptions.size();++i)menu.addItem(20000+i,vst3EffectMenuDescriptions[i].name,hasProject);
}
void showEffectChooser(int index,juce::Component* anchor)
{
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();if(owner.isEmpty())return;
    juce::PopupMenu menu;juce::PopupMenu native;const std::array<const char*,10> names{"Compressor","Reverb","Delay","Chorus","Phaser","Low-pass Filter","Look-ahead Brickwall Limiter","Peak Compressor","Tube Distortion","Two-Band Distortion"};
    for(int i=0;i<10;++i)native.addItem(i+1,names[size_t(i)]);menu.addSubMenu("Built-in Effects",native);
    const auto external=knownVst3Effects();juce::PopupMenu vst;for(int i=0;i<external.size();++i)vst.addItem(1000+i,external[i].name);
    if(external.isEmpty())vst.addItem(900,"No scanned effects",false);menu.addSubMenu("VST3 Effects",vst);
    menu.addSeparator();menu.addItem(99,"Scan / Manage VST3 Effects...");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),[safe=juce::Component::SafePointer<ProjectView>(this),owner,projectId,external](int choice)
    {
        if(safe==nullptr || safe->project.getProjectId()!=projectId)return;const auto index=safe->mixerIndexForOwner(owner);if(index<0)return;
        if(choice>=1 && choice<=10)safe->addInsert(index,choice);
        else if(choice==99)safe->showVst3EffectBrowser(owner);
        else if(juce::isPositiveAndBelow(choice-1000,external.size()))safe->addVst3Effect(index,external[choice-1000]);
    });
}
void closeEffectEditor(te::Plugin& plugin)
{
    plugin.hideWindowForShutdown();if(mixerWindow!=nullptr)mixerWindow->closeEffectEditorFor(plugin.itemID.toString());
}
void copyInsertEffects(int index,int onlySlot=-1)
{
    const auto plugins=getInsertPlugins(index);EffectClipboard clipboard;clipboard.projectId=project.getProjectId();
    if(onlySlot>=0 && !juce::isPositiveAndBelow(onlySlot,plugins.size()))return;
    for(int i=0;i<plugins.size();++i)if(onlySlot<0 || i==onlySlot)
    {
        auto* plugin=plugins[i];
        if(dynamic_cast<te::RackInstance*>(plugin)!=nullptr)
        {status.setText("Rack-linked effects need a rack-aware copy and remain unchanged.",juce::dontSendNotification);return;}
        plugin->flushPluginStateToValueTree();auto state=plugin->state.createCopy();te::AutomationCurve::removeAllAutomationCurvesRecursively(state);
        auto bases=state.getChildWithName("ORBITAUTOMATIONBASES");if(bases.isValid())state.removeChild(bases,nullptr);
        clipboard.states.push_back(state);clipboard.sidechainSources.push_back(plugin->getSidechainSourceID().toString());
    }
    effectClipboard=std::move(clipboard);status.setText("Effect settings copied. Automation lanes stay with their tracks.",juce::dontSendNotification);
}
bool pasteInsertEffects(int index,bool replace)
{
    if(!effectClipboard || mixerOwnerId(index).isEmpty())return false;
    const auto clipboard=*effectClipboard;const auto old=getInsertPlugins(index);
    if(int(clipboard.states.size())+(replace?0:old.size())>maxInsertSlots(index))
    {status.setText("The copied effects exceed this channel's insert capacity.",juce::dontSendNotification);return false;}
    auto* list=mixerPluginList(index);if(list==nullptr)return false;const auto owner=mixerOwnerId(index),projectId=project.getProjectId();
    te::Plugin::Array staged;
    for(size_t i=0;i<clipboard.states.size();++i)
    {
        auto state=clipboard.states[i].createCopy();state.removeProperty(te::IDs::masterPluginID,nullptr);state.removeProperty(te::IDs::sidechainSourceID,nullptr);te::EditItemID::remapIDs(state,nullptr,*edit);
        auto plugin=edit->getPluginCache().createNewPlugin(state);
        if(plugin==nullptr){status.setText("An effect could not be restored. No inserts were changed.",juce::dontSendNotification);return false;}
        te::AudioTrack* source=nullptr;
        if(clipboard.projectId==projectId && clipboard.sidechainSources[i].isNotEmpty())
            for(auto* track:te::getAudioTracks(*edit))if(track->itemID.toString()==clipboard.sidechainSources[i])source=track;
        auto* destination=index==getMixerTracks().size()?nullptr:getMixerTracks()[index];
        if(source!=nullptr && audioRouteWouldCycle(source,destination))source=nullptr;
        plugin->setSidechainSourceID(source!=nullptr?source->itemID:te::EditItemID{});
        if(auto* compressor=dynamic_cast<te::CompressorPlugin*>(plugin.get()))compressor->useSidechainTrigger=source!=nullptr;
        if(plugin->isMissing())plugin->setEnabled(false);staged.add(plugin);
    }
    if(project.getProjectId()!=projectId)return false;rememberUndo(replace?"Paste insert effects (replace)":"Paste insert effects (append)");
    index=mixerIndexForOwner(owner);list=mixerPluginList(index);if(list==nullptr)return false;
    if(replace)for(auto* plugin:old){closeEffectEditor(*plugin);auto state=plugin->state;state.getParent().removeChild(state,&edit->getUndoManager());}
    int at=list->size();if(index==getMixerTracks().size())at=masterInsertEnd();else if(auto* volume=getMixerTracks()[index]->getVolumePlugin())at=list->indexOf(volume);
    for(auto plugin:staged)list->insertPlugin(plugin,at<0?list->size():at++,nullptr);
    isDirty=true;updateEngineAutomation();if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));refreshSongEventEditor();
    status.setText("Effect settings pasted as one Undo operation; copied automation is excluded.",juce::dontSendNotification);return true;
}
void moveInsertEffect(int index,int slot,int direction)
{
    const auto plugins=getInsertPlugins(index);const auto other=slot+direction;
    if(!juce::isPositiveAndBelow(slot,plugins.size()) || !juce::isPositiveAndBelow(other,plugins.size()))return;
    auto state=plugins[slot]->state,neighbor=plugins[other]->state,parent=state.getParent();if(parent!=neighbor.getParent())return;
    rememberUndo("Reorder insert effects");parent.moveChild(parent.indexOf(state),parent.indexOf(neighbor),&edit->getUndoManager());isDirty=true;
    if(mixerWindow!=nullptr)mixerWindow->refresh(getMixerTracks().indexOf(instrumentTrack));
}
void addDirectEffectChoices(juce::PopupMenu& menu,const juce::Array<juce::PluginDescription>& external,bool available=true)
{
    juce::PopupMenu native,vst;
    const std::array<const char*,10> names{"Compressor","Reverb","Delay","Chorus","Phaser","Low-pass Filter","Look-ahead Brickwall Limiter","Peak Compressor","Tube Distortion","Two-Band Distortion"};
    for(int i=0;i<10;++i)native.addItem(101+i,names[size_t(i)],available);
    menu.addSubMenu("Built-in Effects",native,available);
    for(int i=0;i<external.size();++i)vst.addItem(1000+i,external[i].name,available);
    if(external.isEmpty())vst.addItem(900,"No scanned effects",false);
    menu.addSubMenu("VST3 Effects",vst,available);menu.addItem(99,"Scan / Manage VST3 Effects...");
}
juce::PopupMenu makeInsertContextMenu(int index,int slot,const juce::Array<juce::PluginDescription>& external)
{
    const auto plugins=getInsertPlugins(index);const bool populated=juce::isPositiveAndBelow(slot,plugins.size());
    juce::PopupMenu menu;addDirectEffectChoices(menu,external,populated || plugins.size()<maxInsertSlots(index));
    menu.addSeparator();
    if(populated)
    {
        menu.addItem(1,"Open Effect");menu.addItem(2,"Host Parameters / Events");menu.addItem(3,"Enabled",true,plugins[slot]->isEnabled());
        menu.addItem(4,"Remove Effect");menu.addSeparator();menu.addItem(5,"Copy Effect Settings");
        menu.addItem(9,"Move Earlier",slot>0);menu.addItem(10,"Move Later",slot+1<plugins.size());menu.addSeparator();
    }
    menu.addItem(6,"Paste Effects (Append)",effectClipboard.has_value());
    menu.addItem(11,"Copy Insert Chain");menu.addItem(7,"Paste Insert Chain (Replace)",effectClipboard.has_value());
    return menu;
}
void handleInsertContextChoice(const juce::String& owner,const juce::String& projectId,const juce::String& pluginId,
                               const juce::Array<juce::PluginDescription>& external,int choice)
{
    if(project.getProjectId()!=projectId)return;const int index=mixerIndexForOwner(owner);if(index<0)return;
    const auto plugins=getInsertPlugins(index);int slot=-1;for(int i=0;i<plugins.size();++i)if(plugins[i]->itemID.toString()==pluginId)slot=i;
    if(choice==99){showVst3EffectBrowser(owner);return;}
    if(pluginId.isNotEmpty() && slot<0)return;
    if(choice>=101 && choice<=110){addInsert(index,choice-100,pluginId);return;}
    if(juce::isPositiveAndBelow(choice-1000,external.size())){addVst3Effect(index,external[choice-1000],pluginId);return;}
    if(choice==6 || choice==7){pasteInsertEffects(index,choice==7);return;}
    if(choice==11){copyInsertEffects(index);return;}
    if(slot<0)return;if(choice==1)openInsertEditor(index,slot);if(choice==2)openInsertHostParameters(index,slot);
    if(choice==3)toggleInsert(index,slot);if(choice==4)removeInsert(index,slot);if(choice==5)copyInsertEffects(index,slot);
    if(choice==9 || choice==10)moveInsertEffect(index,slot,choice==9?-1:1);
}
void showInsertContextMenu(int index,int slot,juce::Component* anchor)
{
    const auto owner=mixerOwnerId(index),projectId=project.getProjectId();const auto plugins=getInsertPlugins(index);if(owner.isEmpty())return;
    const auto pluginId=juce::isPositiveAndBelow(slot,plugins.size())?plugins[slot]->itemID.toString():juce::String{};
    const auto external=knownVst3Effects();auto menu=makeInsertContextMenu(index,slot,external);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),[safe=juce::Component::SafePointer<ProjectView>(this),owner,projectId,pluginId,external](int choice)
    {if(safe!=nullptr)safe->handleInsertContextChoice(owner,projectId,pluginId,external,choice);});
}
