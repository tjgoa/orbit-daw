// Independent built-in generator controls. Piano Roll remains the active editor.
struct GeneratorControlEntry
{
    te::Plugin::Ptr plugin;
    juce::String pluginId;
    std::unique_ptr<orbit::generator::ControlWindow> window;
};
std::map<juce::String,GeneratorControlEntry> generatorControlWindows;
using GeneratorControlViews=std::vector<std::pair<juce::String,juce::Rectangle<int>>>;
void clearGeneratorControlWindows()
{
    releaseHostedPianoRoll(false);
    // Component teardown sends hierarchy notifications. Silence callbacks
    // before the map starts destroying entries, so no callback walks dead rows.
    for(auto& [id,entry]:generatorControlWindows)entry.window->onWorkspaceChanged={};
    generatorControlWindows.clear();
}
void eraseGeneratorControlWindow(const juce::String& id)
{
    if(hostedPianoRollTrackId==id)releaseHostedPianoRoll(false);
    if(auto entry=generatorControlWindows.find(id);entry!=generatorControlWindows.end())
    {entry->second.window->onWorkspaceChanged={};generatorControlWindows.erase(entry);}
}

te::Plugin* generatorControlPlugin(const juce::String& id) const
{
    if(auto* track=engineTrackForProjectId(id))for(auto* plugin:track->pluginList)
        if(dynamic_cast<orbit::synth::NineKitPlugin*>(plugin)!=nullptr || dynamic_cast<orbit::synth::GeonkickPlugin*>(plugin)!=nullptr || dynamic_cast<te::SamplerPlugin*>(plugin)!=nullptr || dynamic_cast<te::FourOscPlugin*>(plugin)!=nullptr || dynamic_cast<orbit::synth::AcidBassPlugin*>(plugin)!=nullptr
            || (dynamic_cast<te::ExternalPlugin*>(plugin)!=nullptr && plugin->isSynth()))return plugin;
    return nullptr;
}
bool canOpenGeneratorControls(const juce::String& id){return generatorControlPlugin(id)!=nullptr;}
void selectGeneratorControlTrack(const juce::String& id)
{
    independentControlsMode=true;
    if(selectedProjectTrackId()==id)return;
    for(int row=0;row<project.getTrackCount();++row)if(project.getTrackId(row)==id){selectTrackByIndex(row);return;}
}
GeneratorControlViews generatorControlViews()
{
    GeneratorControlViews result;for(const auto& [id,entry]:generatorControlWindows)if(entry.window->isVisible() || entry.window->isMinimised())result.emplace_back(id,entry.window->instrumentViewBounds());return result;
}
void pruneGeneratorControlWindows()
{
    for(auto it=generatorControlWindows.begin();it!=generatorControlWindows.end();)
    {
        auto* plugin=generatorControlPlugin(it->first);
        if(plugin==nullptr || plugin->itemID.toString()!=it->second.pluginId){if(hostedPianoRollTrackId==it->first)releaseHostedPianoRoll(false);it->second.window->onWorkspaceChanged={};it=generatorControlWindows.erase(it);}
        else {it->second.window->setName(engineTrackForProjectId(it->first)->getName());++it;}
    }
}
void refreshControlPattern(const juce::String& id)
{
    const auto slot=project.getTrackSelectedPatternIndex(id);isDirty=true;
    if(id==selectedProjectTrackId()){selectedBankIndex=slot/8;selectedSlotIndex=slot%8;readPatternSteps();loadSelectedPitchSteps();}
    invalidatePlaylistPatternPreview(id,slot);refreshEnginePlaylist();if(id==hostedPianoRollTrackId && pianoRollWindow!=nullptr){pianoRollWindow->setPatternContext(id,slot);pianoRollWindow->refresh(pianoRollTitle());}
}
void selectGeneratorControlPattern(const juce::String& id,int slot)
{
    if(!canOpenGeneratorControls(id) || !juce::isPositiveAndBelow(slot,64))return;
    if(project.getTrackSelectedPatternIndex(id)==slot)return;
    if(patternClipTake.active)
    {
        if(patternClipTake.contains(id.toStdString())){if(!recordPatternClipSelection(id,slot))return;}
        else project.setTrackSelectedPatternIndex(id,slot);
        if(id==selectedProjectTrackId()){selectedBankIndex=slot/8;selectedSlotIndex=slot%8;readPatternSteps();}
    }
    else {if(!finishMidiRecording())return;project.setTrackSelectedPatternIndex(id,slot);refreshControlPattern(id);}
    if(auto window=generatorControlWindows.find(id);window!=generatorControlWindows.end())
    {window->second.window->refreshPattern();if(auto* drum=dynamic_cast<DrumRackWindow*>(window->second.window->instrument.get()))refreshDetachedDrumPanel(id,*drum);}
}
void refreshDetachedDrumPanel(const juce::String& id,DrumRackWindow& panel)
{
    auto* sampler=findSamplerPlugin(id);if(sampler==nullptr)return;const auto slot=project.getTrackSelectedPatternIndex(id);
    panel.setStepCount(project.getPatternLengthSteps(id,slot));
    for(int pad=0;pad<16;++pad){panel.setPadInfo(pad,getDrumRackPadInfo(sampler,pad));panel.setSteps(pad,getDrumRackSteps(id,slot,pad));panel.setVelocities(pad,getDrumRackVelocities(id,slot,pad));}
}
// Notes/velocity edits update existing MIDI clips in place. Other tracks,
// sample editors, saved views and controller curves retain their identities.
void refreshDrumPatternNotes(const juce::String& id,int slot)
{
    auto* track=engineTrackForProjectId(id);if(track==nullptr)return;
    const auto notes=project.getPatternNotes(id,slot);invalidatePlaylistPatternPreview(id,slot);isDirty=true;
    for(auto* item:track->getClips())if(auto* clip=dynamic_cast<te::MidiClip*>(item))
    {
        const auto bar=clip->getPosition().getStart().inSeconds()/playlistSecondsPerBar();
        const bool matches=auditionPatternPlayback?project.getTrackSelectedPatternIndex(id)==slot:
            project.getPatternIndexForId(id,visiblePlaylistPatternId(id,bar))==slot;
        if(!matches)continue;
        auto& sequence=clip->getSequence();sequence.removeAllNotes(nullptr);
        for(const auto& n:notes)sequence.addNote(n.pitch,tracktion::BeatPosition::fromBeats(n.startTick/256.),tracktion::BeatDuration::fromBeats(n.lengthTicks/256.),n.velocity,0,nullptr);
    }
    if(id==selectedProjectTrackId()){hasPattern=project.isPatternCreated(id,slot);for(auto& row:noteGrid)row.clear();for(const auto& n:notes)if(juce::isPositiveAndBelow(n.pitch,128))noteGrid[size_t(n.pitch)].insert(n.startTick/64);}
    if(playlistWindow!=nullptr)playlistWindow->repaintGrid();
    if(pianoRollWindow!=nullptr && pianoRollWindow->isVisible() && pianoEditingTrackId()==id && pianoEditingPatternIndex()==slot)pianoRollWindow->refresh(pianoRollTitle());
}
using DrumControlRef=std::shared_ptr<juce::Component::SafePointer<DrumRackWindow>>;
void chooseDetachedDrumSample(const juce::String& id,int pad,DrumControlRef panel)
{
    const auto projectId=project.getProjectId();fileChooser=std::make_unique<juce::FileChooser>("Load a drum sample",firstSampleFolder(),"*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [root=juce::Component::SafePointer<ProjectView>(this),id,pad,panel,projectId](const juce::FileChooser& chooser)
    {
        if(root==nullptr || root->project.getProjectId()!=projectId || !chooser.getResult().existsAsFile())return;
        auto* sampler=root->findSamplerPlugin(id);if(sampler==nullptr)return;const auto file=chooser.getResult();const int note=48+pad;
        juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        if(reader==nullptr || reader->lengthInSamples<=0){root->status.setText("The sample could not be read. The existing sound was kept.",juce::dontSendNotification);return;}
        root->rememberUndo("Load Drum Rack sample");for(int n=sampler->getNumSounds();--n>=0;)if(sampler->getKeyNote(n)==note)sampler->removeSound(n);
        const auto error=sampler->addSound(file.getFullPathName(),file.getFileNameWithoutExtension(),0,0,0);
        if(error.isNotEmpty()){root->status.setText(error,juce::dontSendNotification);return;}
        const int sound=sampler->getNumSounds()-1;sampler->setSoundParams(sound,note,note,note);sampler->setSoundOpenEnded(sound,true);sampler->setSoundLooping(sound,false);
        auto state=root->findSamplerSoundState(sampler,sound);state.setProperty("orbitOriginalSource",file.getFullPathName(),sampler->getUndoManager());state.setProperty("orbitReversed",false,sampler->getUndoManager());
        root->isDirty=true;if(*panel!=nullptr)root->refreshDetachedDrumPanel(id,**panel);
        if(root->drumRackWindow!=nullptr && root->drumRackTrackId==id)root->refreshDrumRackPadInfoLater(id,pad);
    });
}
std::unique_ptr<DrumRackWindow> createDetachedDrumControls(const juce::String& id)
{
    auto weak=std::make_shared<juce::Component::SafePointer<DrumRackWindow>>();const auto projectId=project.getProjectId();
    const auto valid=[this,id,projectId]{return project.getProjectId()==projectId && findSamplerPlugin(id)!=nullptr;};
    auto panel=std::make_unique<DrumRackWindow>(
        [this,id,weak,valid](int pad){if(valid())chooseDetachedDrumSample(id,pad,weak);},
        [this,id,weak,valid](int){if(valid() && *weak!=nullptr)refreshDetachedDrumPanel(id,**weak);},
        [this,id,weak,valid](int pad)
        {
            if(!valid())return;juce::BigInteger notes;notes.setBit(48+pad);findSamplerPlugin(id)->playNotes(notes);
            juce::Timer::callAfterDelay(90,[root=juce::Component::SafePointer<ProjectView>(this),id,weak,projectId=project.getProjectId()]
            {if(root!=nullptr && *weak!=nullptr && root->project.getProjectId()==projectId)if(auto* sampler=root->findSamplerPlugin(id))sampler->playNotes(juce::BigInteger{});});
        },
        [this,id,weak,valid](int pad,int step,bool enabled)
        {
            if(!valid())return;const int slot=project.getTrackSelectedPatternIndex(id),pitch=48+pad;
            project.setPatternCreated(id,slot,true);project.setPatternNote(id,slot,pitch,step,enabled);
            if(enabled){project.setPatternNoteVelocity(id,slot,pitch,step,100);project.setPatternNoteLength(id,slot,pitch,step,1);}
            refreshDrumPatternNotes(id,slot);if(*weak!=nullptr){(*weak)->setSteps(pad,getDrumRackSteps(id,slot,pad));(*weak)->setVelocities(pad,getDrumRackVelocities(id,slot,pad));}
        },
        [this,id,weak,valid](int pad,int step,int velocity)
        {
            if(!valid())return;const int slot=project.getTrackSelectedPatternIndex(id),pitch=48+pad;
            if(project.getPatternNote(id,slot,pitch,step))project.setPatternNoteVelocity(id,slot,pitch,step,velocity);
            refreshDrumPatternNotes(id,slot);if(*weak!=nullptr){(*weak)->setSteps(pad,getDrumRackSteps(id,slot,pad));(*weak)->setVelocities(pad,getDrumRackVelocities(id,slot,pad));}
        },
        [this,id,weak,valid](int pad,DrumRackWindow::PadControl control,double value)
        {if(valid()){setDrumRackPadControl(id,pad,control,value);if(*weak!=nullptr)(*weak)->setPadInfo(pad,getDrumRackPadInfo(findSamplerPlugin(id),pad));}},
        [this,id,weak,valid](int count)
        {
            if(!valid())return;const auto slot=project.getTrackSelectedPatternIndex(id);rememberUndo("Change Drum Rack pattern length");project.setPatternCreated(id,slot,true);project.setPatternLengthSteps(id,slot,count);
            refreshControlPattern(id);if(*weak!=nullptr)refreshDetachedDrumPanel(id,**weak);
        },juce::Rectangle<int>{0,0,1320,760},[](juce::Rectangle<int>){});
    *weak=panel.get();panel->setEmbedded(true);panel->undoGestureHandler=[this](const auto& label){rememberUndo(label);};
    panel->onGeneratorMenu=[this,id](juce::Component* anchor){showGeneratorMenuFor(id,anchor);};refreshDetachedDrumPanel(id,*panel);return panel;
}
juce::String generatorPresetName(const juce::String& id)
{
    if(auto* p=generatorControlPlugin(id))
    {
        if(p->state.hasProperty("orbitPresetName"))return p->state.getProperty("orbitPresetName").toString();
        if(auto* ext=dynamic_cast<te::ExternalPlugin*>(p);ext!=nullptr && ext->getNumPrograms()>0)return ext->getCurrentProgramName();
        return p->state.getProperty("orbitPresetName","Default").toString();
    }
    return {};
}
juce::File generatorPresetFolder(const juce::String& id,bool create=false)
{
    auto* plugin=generatorControlPlugin(id);if(plugin==nullptr)return {};
    const auto configured=optionString("presetLibraryRoot");
    const auto root=configured.isNotEmpty()?juce::File(configured):juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Orbit/Presets");
    const auto name=dynamic_cast<orbit::synth::AcidBassPlugin*>(plugin)!=nullptr?juce::String("Acid Bass"):juce::File::createLegalFileName(plugin->getName());
    const auto folder=root.getChildFile(name.isEmpty()?"Generator":name);if(create)folder.createDirectory();return folder;
}
juce::StringArray generatorPresetChoices(const juce::String& id)
{
    juce::StringArray names;
    if(auto* p=dynamic_cast<te::ExternalPlugin*>(generatorControlPlugin(id)))for(int i=0;i<p->getNumPrograms();++i)names.add(p->getProgramName(i).isEmpty()?juce::String(i+1):p->getProgramName(i));
    const auto folder=generatorPresetFolder(id);juce::Array<juce::File> files;
    if(folder.isDirectory())files=folder.findChildFiles(juce::File::findFiles,true,"*.orbitpreset");
    struct Order{int compareElements(const juce::File& a,const juce::File& b)const{return a.getFullPathName().compareNatural(b.getFullPathName());}}order;files.sort(order);
    generatorPresetFiles[id]=files;for(const auto& file:files)names.add(file.getRelativePathFrom(folder).dropLastCharacters(12));return names;
}
void refreshGeneratorPresetLists()
{
    for(auto& [id,entry]:generatorControlWindows)entry.window->setPresetChoices(generatorPresetChoices(id));
    if(instrumentTrack!=nullptr){patternPresetSelector.clear(juce::dontSendNotification);patternPresetSelector.addItemList(generatorPresetChoices(selectedProjectTrackId()),1);patternPresetSelector.setText(generatorPresetName(selectedProjectTrackId()),juce::dontSendNotification);}
}
void chooseGeneratorPreset(const juce::String& id,int index)
{
    if(!requireUnfrozenGenerator(id))return;
    if(auto* p=dynamic_cast<te::ExternalPlugin*>(generatorControlPlugin(id));p!=nullptr && index<p->getNumPrograms())
    {if(index<0)return;rememberUndo("Change generator preset");p->state.removeProperty("orbitPresetName",nullptr);p->setCurrentProgram(index,true);isDirty=true;return;}
    int offset=0;if(auto* p=dynamic_cast<te::ExternalPlugin*>(generatorControlPlugin(id)))offset=p->getNumPrograms();
    if(auto found=generatorPresetFiles.find(id);found!=generatorPresetFiles.end() && juce::isPositiveAndBelow(index-offset,found->second.size()))loadGeneratorPresetFile(id,found->second[index-offset]);
}
juce::File suggestedGeneratorPresetFile(const juce::String& id,bool save)
{
    const auto folder=generatorPresetFolder(id,true);if(!save)return folder;
    auto name=juce::File::createLegalFileName(generatorPresetName(id));if(name.isEmpty() || name=="Default")name="Untitled Preset";
    return folder.getChildFile(name+".orbitpreset");
}
void setGeneratorControlSteps(const juce::String& id,int count)
{
    if(!requireUnfrozenGenerator(id))return;
    if(!canOpenGeneratorControls(id) || midiTake.active || automationTakeActive || patternClipTake.active)return;
    const int slot=project.getTrackSelectedPatternIndex(id),next=juce::jlimit(1,ProjectDocument::maximumPatternLengthSteps,count);
    if(next==project.getPatternLengthSteps(id,slot))return;
    rememberUndo("Change pattern length");project.setPatternCreated(id,slot,true);project.setPatternLengthSteps(id,slot,next);refreshControlPattern(id);
    if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end())found->second.window->refreshPattern();
}
bool saveGeneratorPresetFile(const juce::String& id,const juce::File& file)
{
    auto* plugin=generatorControlPlugin(id);if(plugin==nullptr)return false;
    plugin->flushPluginStateToValueTree();juce::ValueTree preset("ORBITGENERATORPRESET");preset.setProperty("version",1,nullptr);
    preset.setProperty("identifier",plugin->getIdentifierString(),nullptr);preset.setProperty("name",file.getFileNameWithoutExtension(),nullptr);
    preset.addChild(plugin->state.createCopy(),-1,nullptr);
    auto xml=preset.createXml();const bool ok=xml!=nullptr && xml->writeTo(file);
    if(ok){plugin->state.setProperty("orbitPresetName",file.getFileNameWithoutExtension(),nullptr);isDirty=true;refreshGeneratorPresetLists();}return ok;
}
bool loadGeneratorPresetFile(const juce::String& id,const juce::File& file)
{
    if(!requireUnfrozenGenerator(id))return false;
    auto* plugin=generatorControlPlugin(id);if(plugin==nullptr || file.getSize()>64*1024*1024 || midiTake.active || automationTakeActive || patternClipTake.active)return false;
    auto xml=juce::parseXML(file);auto preset=xml!=nullptr?juce::ValueTree::fromXml(*xml):juce::ValueTree{};
    if(!preset.hasType("ORBITGENERATORPRESET") || int(preset.getProperty("version"))!=1 || preset.getProperty("identifier").toString()!=plugin->getIdentifierString() || preset.getNumChildren()!=1 || preset.getChild(0).getType()!=plugin->state.getType())return false;
    auto state=preset.getChild(0).createCopy();state.setProperty(te::IDs::id,plugin->state.getProperty(te::IDs::id),nullptr);
    rememberUndo("Load generator preset");plugin->restorePluginStateFromValueTree(state);
    plugin->state.setProperty("orbitKnobOverride",juce::jlimit(-1,2,int(state.getProperty("orbitKnobOverride",-1))),nullptr);refreshGeneratorKnobStyle(id);
    plugin->state.setProperty("orbitPresetName",preset.getProperty("name",file.getFileNameWithoutExtension()),nullptr);
    isDirty=true;updateEngineAutomation();refreshSongEventEditor();refreshGeneratorPresetLists();
    if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end())found->second.window->refreshPattern();
    return true;
}
void chooseGeneratorPresetFile(const juce::String& id,bool save)
{
    if(!requireUnfrozenGenerator(id))return;
    auto* plugin=generatorControlPlugin(id);if(plugin==nullptr)return;
    const auto projectId=project.getProjectId();const auto instance=plugin->itemID.toString();
    fileChooser=std::make_unique<juce::FileChooser>(save?"Save generator preset":"Open generator preset",suggestedGeneratorPresetFile(id,save),"*.orbitpreset");
    fileChooser->launchAsync((save?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode)|juce::FileBrowserComponent::canSelectFiles,
        [weak=juce::Component::SafePointer<ProjectView>(this),id,projectId,instance,save](const juce::FileChooser& chooser)
        {
            if(weak==nullptr || weak->project.getProjectId()!=projectId)return;auto* current=weak->generatorControlPlugin(id);
            if(current==nullptr || current->itemID.toString()!=instance)return;auto file=chooser.getResult();if(file==juce::File{})return;
            if(save)file=file.withFileExtension("orbitpreset");
            const bool ok=save?weak->saveGeneratorPresetFile(id,file):weak->loadGeneratorPresetFile(id,file);
            weak->status.setText(ok?(save?"Generator preset saved.":"Generator preset loaded."):"Could not use this generator preset; check its instrument type and file.",juce::dontSendNotification);
        });
}
void openGeneratorControls(const juce::String& id,bool focus=true,std::optional<juce::Rectangle<int>> restore={})
{
    auto* plugin=generatorControlPlugin(id);if(plugin==nullptr)return;pruneGeneratorControlWindows();
    if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end()){found->second.window->showControls(focus);return;}
    const auto projectId=project.getProjectId();std::unique_ptr<juce::Component> body;
    const bool drum=dynamic_cast<te::SamplerPlugin*>(plugin)!=nullptr;
    if(isGeneratorFrozen(id))
    {
        auto label=std::make_unique<juce::Label>();label->setText("Frozen for Song. Mixer processing stays live.\n"+juce::String(frozenPatternAuditionAllowed(id)?"Use the pattern buttons and Pattern Play to audition the original.":"Live Pattern audition is off.")+"\nChoose Unfreeze from the generator menu to edit the source.",juce::dontSendNotification);label->setJustificationType(juce::Justification::centred);body=std::move(label);
    }
    else if(drum)body=createDetachedDrumControls(id);
    else if(auto* geon=dynamic_cast<orbit::synth::GeonkickPlugin*>(plugin))body=createGeonkickEditor(id,*geon);
    else if(auto* nine=dynamic_cast<orbit::synth::NineKitPlugin*>(plugin))body=createNineKitEditor(id,*nine);
    else if(auto* acid=dynamic_cast<orbit::synth::AcidBassPlugin*>(plugin))
    {
        auto editor=std::make_unique<orbit::synth::AcidBassEditor>(acid,[this]{rememberUndo("Adjust Acid Bass");},[this]{isDirty=true;},
            [this,id]{selectGeneratorControlTrack(id);createAcidDemo();});
        editor->onEditEvents=[this,acid](int parameter,juce::Component* anchor){showParameterEventMenu(*acid->parameters[size_t(parameter)],anchor);};body=std::move(editor);
    }
    else if(auto* external=dynamic_cast<te::ExternalPlugin*>(plugin))body=std::make_unique<OrbitPluginEditorContent>(*external);
    else body=std::make_unique<OrbitNativeInstrumentEditor>(plugin,[this]{isDirty=true;},[this](auto& parameter,juce::Component* anchor){showParameterEventMenu(parameter,anchor);});
    const int offset=28*int(generatorControlWindows.size());juce::Rectangle<int> bounds{40+offset,90+offset,drum?1160:900,drum?760:440};
    if(dynamic_cast<orbit::synth::AcidBassPlugin*>(plugin)!=nullptr)bounds.setHeight(300);
    if(dynamic_cast<orbit::synth::GeonkickPlugin*>(plugin)!=nullptr)bounds.setHeight(310);
    if(dynamic_cast<orbit::synth::NineKitPlugin*>(plugin)!=nullptr)bounds.setHeight(520);
    const auto prefix="generatorWorkspace_"+id+"_";
    if(auto* settings=applicationProperties.getUserSettings();settings!=nullptr && settings->getBoolValue(prefix+"set",false))bounds={settings->getIntValue(prefix+"x",bounds.getX()),settings->getIntValue(prefix+"y",bounds.getY()),settings->getIntValue(prefix+"w",bounds.getWidth()),settings->getIntValue(prefix+"h",bounds.getHeight())};
    if(restore)bounds=*restore;
    auto window=std::make_unique<orbit::generator::ControlWindow>(engineTrackForProjectId(id)->getName(),std::move(body),
        [this,id,projectId]{return project.getProjectId()==projectId && canOpenGeneratorControls(id)?project.getTrackSelectedPatternIndex(id):-1;},
        [this,id,projectId](int slot){if(project.getProjectId()==projectId)selectGeneratorControlPattern(id,slot);},
        [this,id](juce::Component* anchor){showGeneratorMenuFor(id,anchor);},
        [this,id]{toggleHostedPianoRoll(id);},
        [this,prefix](juce::Rectangle<int> b)
        {if(auto* settings=applicationProperties.getUserSettings()){settings->setValue(prefix+"set",true);settings->setValue(prefix+"x",b.getX());settings->setValue(prefix+"y",b.getY());settings->setValue(prefix+"w",b.getWidth());settings->setValue(prefix+"h",b.getHeight());settings->saveIfNeeded();}},bounds);
    window->readSteps=[this,id]{return project.getPatternLengthSteps(id,project.getTrackSelectedPatternIndex(id));};
    window->writeSteps=[this,id](int count){setGeneratorControlSteps(id,count);};
    window->readSolo=[this,id]{auto* t=engineTrackForProjectId(id);return t!=nullptr && t->isSolo(false);};
    window->readMute=[this,id]{auto* t=engineTrackForProjectId(id);return t!=nullptr && t->isMuted(false);};
    window->writeSolo=[this,id](bool state){for(int row=0;row<project.getTrackCount();++row)if(project.getTrackId(row)==id)setTrackSolo(row,state);};
    window->writeMute=[this,id](bool state){for(int row=0;row<project.getTrackCount();++row)if(project.getTrackId(row)==id)setTrackMute(row,state);};
    window->readPreset=[this,id]{return generatorPresetName(id);};window->choosePreset=[this,id](int index){chooseGeneratorPreset(id,index);};
    window->openPreset=[this,id]{chooseGeneratorPresetFile(id,false);};window->savePreset=[this,id]{chooseGeneratorPresetFile(id,true);};window->setPresetChoices(generatorPresetChoices(id));
    window->onPatternClosed=[this]{closePianoRollView();};
    window->onDeleteRequested=[root=juce::Component::SafePointer<ProjectView>(this),id,projectId]
    {
        // Delete after the button's dispatch unwinds: the confirmation can remove its owning frame.
        juce::MessageManager::callAsync([root,id,projectId]
        {if(root!=nullptr && root->project.getProjectId()==projectId)for(int row=0;row<root->project.getTrackCount();++row)if(root->project.getTrackId(row)==id){root->deleteGeneratorAtProjectIndex(row);break;}});
    };
    window->onMinimizeStateChanged=[this]{scheduleGeneratorDockRefresh();};
    window->onFocus=[this,id]{playlistEditActive=false;selectGeneratorControlTrack(id);};
    window->onWorkspaceChanged=[this,id]{if(hostedPianoRollTrackId==id)if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end())pianoRollMaximized=found->second.window->isMaximized();syncGeneratorNativeViews();};
    addChildComponent(*window);window->constrainToWorkspace();
    if (auto* panel=dynamic_cast<DrumRackWindow*>(window->instrument.get()))
    {
        window->onRefresh=[this,id,panel]{refreshDetachedDrumPanel(id,*panel);};
        auto priorHeight=std::make_shared<int>(0);
        panel->onSampleEditorExpanded=[weak=juce::Component::SafePointer<orbit::generator::ControlWindow>(window.get()),priorHeight](bool expanded)
        {
            if (weak==nullptr || weak->isPatternView()) return;
            auto bounds=weak->getBounds();
            if (expanded)
            {
                *priorHeight=bounds.getHeight();
                int limit=1100;
                if (const auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(bounds))
                    limit=display->userArea.getHeight();
                bounds.setHeight(juce::jmax(bounds.getHeight(),juce::jmin(1020,limit)));
            }
            else if (*priorHeight>0) { bounds.setHeight(*priorHeight); *priorHeight=0; }
            if (juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()!=nullptr)
                weak->setBoundsConstrained(bounds);
            else weak->setBounds(bounds);
        };
    }
    if(auto* panel=dynamic_cast<orbit::synth::AcidBassEditor*>(window->instrument.get()))
    {
        auto priorHeight=std::make_shared<int>(0);
        panel->onEffectsExpanded=[weak=juce::Component::SafePointer<orbit::generator::ControlWindow>(window.get()),priorHeight,panel](bool expanded)
        {
            if(weak==nullptr || weak->isPatternView())return;
            if(expanded){if(*priorHeight==0)*priorHeight=weak->getHeight();weak->setSize(weak->getWidth(),juce::jmax(*priorHeight,300+panel->preferredHeight()-190));}
            else if(*priorHeight>0){weak->setSize(weak->getWidth(),*priorHeight);*priorHeight=0;}
            weak->constrainToWorkspace();
        };
    }
    window->onShortcut=[root=juce::Component::SafePointer<ProjectView>(this),id,projectId](const juce::KeyPress& key)
    {juce::MessageManager::callAsync([root,id,projectId,key]{if(root!=nullptr && root->project.getProjectId()==projectId && root->canOpenGeneratorControls(id)){root->selectGeneratorControlTrack(id);root->handleEditShortcut(key);}});};
    if(auto* native=dynamic_cast<OrbitPluginEditorContent*>(window->instrument.get()))
        native->preferredSizeChanged=[weak=juce::Component::SafePointer<orbit::generator::ControlWindow>(window.get()),native]
        {if(weak!=nullptr && !weak->isPatternView()){weak->setSize(juce::jmax(480,native->getPreferredWidth()),native->getPreferredHeight()+82);weak->constrainToWorkspace();}};
    if(window->instrument)window->instrument->getProperties().set("orbitKnobOverride",juce::jlimit(-1,2,int(plugin->state.getProperty("orbitKnobOverride",-1))));
    auto* show=window.get();generatorControlWindows.emplace(id,GeneratorControlEntry{plugin,plugin->itemID.toString(),std::move(window)});show->showControls(focus);
    if(auto* native=dynamic_cast<OrbitPluginEditorContent*>(show->instrument.get())){native->ensureEditorCreated();syncGeneratorNativeViews();}
}
