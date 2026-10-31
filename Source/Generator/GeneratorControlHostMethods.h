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
    // Component teardown sends hierarchy notifications. Silence callbacks
    // before the map starts destroying entries, so no callback walks dead rows.
    for(auto& [id,entry]:generatorControlWindows)entry.window->onWorkspaceChanged={};
    generatorControlWindows.clear();
}
void eraseGeneratorControlWindow(const juce::String& id)
{
    if(auto entry=generatorControlWindows.find(id);entry!=generatorControlWindows.end())
    {entry->second.window->onWorkspaceChanged={};generatorControlWindows.erase(entry);}
}

te::Plugin* generatorControlPlugin(const juce::String& id)
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
    GeneratorControlViews result;for(const auto& [id,entry]:generatorControlWindows)if(entry.window->isVisible())result.emplace_back(id,entry.window->floatingBounds());return result;
}
void pruneGeneratorControlWindows()
{
    for(auto it=generatorControlWindows.begin();it!=generatorControlWindows.end();)
    {
        auto* plugin=generatorControlPlugin(it->first);
        if(plugin==nullptr || plugin->itemID.toString()!=it->second.pluginId){it->second.window->onWorkspaceChanged={};it=generatorControlWindows.erase(it);}
        else {it->second.window->setName(engineTrackForProjectId(it->first)->getName());++it;}
    }
}
void refreshControlPattern(const juce::String& id)
{
    const auto slot=project.getTrackSelectedPatternIndex(id);isDirty=true;
    if(id==selectedProjectTrackId()){selectedBankIndex=slot/8;selectedSlotIndex=slot%8;readPatternSteps();loadSelectedPitchSteps();}
    invalidatePlaylistPatternPreview(id,slot);refreshEnginePlaylist();
}
void selectGeneratorControlPattern(const juce::String& id,int slot)
{
    if(!canOpenGeneratorControls(id) || !juce::isPositiveAndBelow(slot,64))return;
    if(project.getTrackSelectedPatternIndex(id)==slot)return;
    if(!finishMidiRecording())return;project.setTrackSelectedPatternIndex(id,slot);refreshControlPattern(id);
    if(auto window=generatorControlWindows.find(id);window!=generatorControlWindows.end())
    {window->second.window->refreshPattern();if(auto* drum=dynamic_cast<DrumRackWindow*>(window->second.window->instrument.get()))refreshDetachedDrumPanel(id,*drum);}
}
void refreshDetachedDrumPanel(const juce::String& id,DrumRackWindow& panel)
{
    auto* sampler=findSamplerPlugin(id);if(sampler==nullptr)return;const auto slot=project.getTrackSelectedPatternIndex(id);
    panel.setStepCount(project.getPatternLengthSteps(id,slot));
    for(int pad=0;pad<16;++pad){panel.setPadInfo(pad,getDrumRackPadInfo(sampler,pad));panel.setSteps(pad,getDrumRackSteps(id,slot,pad));panel.setVelocities(pad,getDrumRackVelocities(id,slot,pad));}
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
            refreshControlPattern(id);if(*weak!=nullptr)refreshDetachedDrumPanel(id,**weak);
        },
        [this,id,weak,valid](int pad,int step,int velocity)
        {
            if(!valid())return;const int slot=project.getTrackSelectedPatternIndex(id),pitch=48+pad;
            if(project.getPatternNote(id,slot,pitch,step))project.setPatternNoteVelocity(id,slot,pitch,step,velocity);
            refreshControlPattern(id);if(*weak!=nullptr)refreshDetachedDrumPanel(id,**weak);
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
void openGeneratorControls(const juce::String& id,bool focus=true,std::optional<juce::Rectangle<int>> restore={})
{
    auto* plugin=generatorControlPlugin(id);if(plugin==nullptr)return;pruneGeneratorControlWindows();
    if(auto found=generatorControlWindows.find(id);found!=generatorControlWindows.end()){found->second.window->showControls(focus);return;}
    const auto projectId=project.getProjectId();std::unique_ptr<juce::Component> body;
    const bool drum=dynamic_cast<te::SamplerPlugin*>(plugin)!=nullptr;
    if(drum)body=createDetachedDrumControls(id);
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
        [this,id]{selectGeneratorControlTrack(id);showPianoRoll();},
        [this,prefix](juce::Rectangle<int> b)
        {if(auto* settings=applicationProperties.getUserSettings()){settings->setValue(prefix+"set",true);settings->setValue(prefix+"x",b.getX());settings->setValue(prefix+"y",b.getY());settings->setValue(prefix+"w",b.getWidth());settings->setValue(prefix+"h",b.getHeight());settings->saveIfNeeded();}},bounds);
    window->onFocus=[this,id]{selectGeneratorControlTrack(id);};
    window->onWorkspaceChanged=[this]{syncGeneratorNativeViews();};
    addChildComponent(*window);window->constrainToWorkspace();
    if (auto* panel=dynamic_cast<DrumRackWindow*>(window->instrument.get()))
    {
        window->onRefresh=[this,id,panel]{refreshDetachedDrumPanel(id,*panel);};
        auto priorHeight=std::make_shared<int>(0);
        panel->onSampleEditorExpanded=[weak=juce::Component::SafePointer<orbit::generator::ControlWindow>(window.get()),priorHeight](bool expanded)
        {
            if (weak==nullptr) return;
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
        panel->onEffectsExpanded=[weak=juce::Component::SafePointer<orbit::generator::ControlWindow>(window.get()),priorHeight](bool expanded)
        {
            if(weak==nullptr)return;
            if(expanded){*priorHeight=weak->getHeight();weak->setSize(weak->getWidth(),juce::jmax(weak->getHeight(),404));}
            else if(*priorHeight>0){weak->setSize(weak->getWidth(),*priorHeight);*priorHeight=0;}
            weak->constrainToWorkspace();
        };
    }
    window->onShortcut=[root=juce::Component::SafePointer<ProjectView>(this),id,projectId](const juce::KeyPress& key)
    {juce::MessageManager::callAsync([root,id,projectId,key]{if(root!=nullptr && root->project.getProjectId()==projectId && root->canOpenGeneratorControls(id)){root->selectGeneratorControlTrack(id);root->handleEditShortcut(key);}});};
    if(auto* native=dynamic_cast<OrbitPluginEditorContent*>(window->instrument.get()))
        native->preferredSizeChanged=[weak=juce::Component::SafePointer<orbit::generator::ControlWindow>(window.get()),native]
        {if(weak!=nullptr){weak->setSize(native->getPreferredWidth(),native->getPreferredHeight()+82);weak->constrainToWorkspace();}};
    auto* show=window.get();generatorControlWindows.emplace(id,GeneratorControlEntry{plugin,plugin->itemID.toString(),std::move(window)});show->showControls(focus);
    if(auto* native=dynamic_cast<OrbitPluginEditorContent*>(show->instrument.get())){native->ensureEditorCreated();syncGeneratorNativeViews();}
}
