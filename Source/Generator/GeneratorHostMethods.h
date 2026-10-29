// Included inside ProjectView after its pattern and OptionsPanel declarations.
// UI handlers pin the generator ID and pattern slot while a dialog is open.
orbit::generator::MidiPlugin* generatorMidiPlugin(const juce::String& id,bool create=false)
{
    auto* track=engineTrackForProjectId(id);if(track==nullptr || edit==nullptr)return nullptr;
    for(auto* plugin:track->pluginList)if(auto* midi=dynamic_cast<orbit::generator::MidiPlugin*>(plugin))return midi;
    if(!create)return nullptr;
    auto plugin=edit->getPluginCache().createNewPlugin(orbit::generator::MidiPlugin::xmlTypeName,{});
    auto* midi=dynamic_cast<orbit::generator::MidiPlugin*>(plugin.get());
    if(midi!=nullptr)track->pluginList.insertPlugin(*midi,0,nullptr);return midi;
}
orbit::generator::Settings generatorSettings(const juce::String& id)
{if(auto* plugin=generatorMidiPlugin(id))return plugin->getSettings();return {};}
void refreshGeneratorSettings(const juce::String& id)
{
    isDirty=true;releaseLiveMidi();
    if(id==selectedProjectTrackId()){readPatternSteps();loadSelectedPitchSteps();}
    refreshEnginePlaylist();refreshTrackSelector();refreshMixerControls();
    if(playlistWindow!=nullptr)playlistWindow->refresh();
}
bool applyGeneratorArpeggio(const juce::String& id,int slot,orbit::generator::Settings settings,bool render)
{
    auto* track=engineTrackForProjectId(id);if(track==nullptr || edit==nullptr)return false;
    std::vector<orbit::generator::Note> rendered;
    if(render)
    {
        if(!settings.arp && settings.shuffle==0){status.setText("Enable the arpeggiator or shuffle before rendering.",juce::dontSendNotification);return false;}
        const auto source=project.getPatternNotes(id,slot);
        if(source.empty()){status.setText("This pattern has no notes to render.",juce::dontSendNotification);return false;}
        std::vector<orbit::generator::Note> notes;for(const auto& n:source)notes.push_back({n.pitch,n.startTick,n.lengthTicks,n.velocity});
        try{rendered=orbit::generator::render(notes,project.getPatternLengthSteps(id,slot)*64,settings);}
        catch(const std::exception& error){status.setText(juce::String::fromUTF8(error.what()),juce::dontSendNotification);return false;}
    }
    if(!render && generatorSettings(id)==settings)return true;
    rememberUndo(render?"Render arpeggiator and shuffle":"Change arpeggiator and shuffle");
    auto* midi=generatorMidiPlugin(id,true);if(midi==nullptr)return false;
    if(render)
    {
        PatternData data;data.created=true;data.lengthSteps=project.getPatternLengthSteps(id,slot);
        for(const auto& n:rendered)data.notes.push_back({n.pitch,n.startTick,n.lengthTicks,n.velocity});
        writePatternData(id,slot,data);settings.arp=false;settings.shuffle=0;clearNoteSelection();
    }
    midi->setSettings(settings);refreshGeneratorSettings(id);
    status.setText(render?"Arpeggio rendered into the pattern. Arpeggiator and shuffle disabled; Undo restores both notes and settings.":"Generator arpeggiator and shuffle applied. Save to keep the settings.",juce::dontSendNotification);return true;
}
static bool loadGeneratorNoteOrder(const juce::File& file,orbit::generator::Settings& settings,juce::String& error)
{
    if(file.getSize()>1024*1024){error="Choose a MIDI file smaller than 1 MiB.";return false;}
    juce::FileInputStream input(file);juce::MidiFile midi;
    if(!input.openedOk() || !midi.readFrom(input)){error="Could not read a standard MIDI file.";return false;}
    juce::MidiMessageSequence sequence;
    for(int t=0;t<midi.getNumTracks();++t)sequence.addSequence(*midi.getTrack(t),0);
    sequence.sort();std::vector<int> pitches;
    for(int i=0;i<sequence.getNumEvents();++i)if(sequence.getEventPointer(i)->message.isNoteOn())pitches.push_back(sequence.getEventPointer(i)->message.getNoteNumber());
    if(pitches.empty() || pitches.size()>32){error="User note orders need 1-32 MIDI notes.";return false;}
    const int origin=pitches.front();for(int pitch:pitches)if(std::abs(pitch-origin)>48){error="Keep the user note order within 48 semitones of its first note.";return false;}
    settings.customSize=int(pitches.size());for(size_t i=0;i<pitches.size();++i)settings.custom[i]=pitches[i]-origin;
    settings.order=5;return true;
}
void presentGeneratorArpeggiator(std::unique_ptr<orbit::generator::ArpeggiatorPanel> panel,
                                std::function<bool(orbit::generator::Settings,bool)> commit)
{
    auto* form=panel.get();juce::DialogWindow::LaunchOptions options;
    options.dialogTitle="Arpeggiator and Shuffle";options.dialogBackgroundColour=orbit::options::colour(27,43,55);
    options.componentToCentreAround=this;options.useNativeTitleBar=true;options.resizable=false;options.content.setOwned(panel.release());
    auto* window=options.launchAsync();const juce::Component::SafePointer<juce::DialogWindow> safeWindow(window);
    const juce::Component::SafePointer<orbit::generator::ArpeggiatorPanel> safePanel(form);
    const juce::Component::SafePointer<ProjectView> owner(this);
    form->apply.onClick=[owner,safeWindow,safePanel,commit]
    {if(owner==nullptr || safeWindow==nullptr || safePanel==nullptr)return;if(commit(safePanel->getSettings(),false))safeWindow->exitModalState(1);else safePanel->setMessage(owner->status.getText());};
    form->render.onClick=[owner,safeWindow,safePanel,commit]
    {if(owner==nullptr || safeWindow==nullptr || safePanel==nullptr)return;if(commit(safePanel->getSettings(),true))safeWindow->exitModalState(1);else safePanel->setMessage(owner->status.getText());};
    form->cancel.onClick=[safeWindow]{if(safeWindow!=nullptr)safeWindow->exitModalState(0);};
    form->load.onClick=[owner,safePanel]
    {
        if(owner==nullptr || safePanel==nullptr)return;
        owner->fileChooser=std::make_unique<juce::FileChooser>("Load user arpeggio note order from MIDI",juce::File{},"*.mid;*.midi");
        owner->fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[owner,safePanel](const auto& chooser)
        {
            if(owner==nullptr || safePanel==nullptr || chooser.getResult()==juce::File{})return;
            auto settings=safePanel->getSettings();juce::String error;
            if(loadGeneratorNoteOrder(chooser.getResult(),settings,error)){safePanel->setCustom(settings);safePanel->order.setSelectedItemIndex(5);safePanel->setMessage("MIDI note order loaded. Apply or Render to use it.");}
            else safePanel->setMessage(error);
        });
    };
}
void showGeneratorArpeggiator(const juce::String& id,int slot)
{
    if(engineTrackForProjectId(id)==nullptr)return;
    const auto projectId=project.getProjectId();
    const juce::Component::SafePointer<ProjectView> owner(this);
    presentGeneratorArpeggiator(std::make_unique<orbit::generator::ArpeggiatorPanel>(generatorSettings(id)),[owner,id,slot,projectId](auto settings,bool render)
    {if(owner==nullptr)return false;if(owner->project.getProjectId()!=projectId){owner->status.setText("The project changed. Close this panel and reopen it.",juce::dontSendNotification);return false;}return owner->applyGeneratorArpeggio(id,slot,settings,render);});
}
void showGeneratorProperties(const juce::String& id)
{
    auto* track=engineTrackForProjectId(id);if(track==nullptr)return;
    const auto settings=generatorSettings(id);
    auto panel=std::make_unique<OptionsPanel>("MIDI settings for this generator. Transpose and velocity affect playback and live input without changing pattern notes. Input channel follows Options > MIDI In Settings > Advanced routing.");
    juce::StringArray channels{"Omni"},outputs{"Preserve input channel"};for(int i=1;i<=16;++i){channels.add(juce::String(i));outputs.add(juce::String(i));}
    panel->choice("input","MIDI input channel",channels,int(track->state.getProperty("orbitMidiChannel",0)));
    panel->choice("output","MIDI output channel",outputs,settings.outputChannel);
    panel->number("transpose","Transpose (semitones)",settings.transpose,-48,48);
    panel->number("velocity","Velocity scale (%)",settings.velocity,1,200);
    openOptionsTool("Generator Settings",std::move(panel),[this,id](const auto& p)
    {
        auto* target=engineTrackForProjectId(id);if(target==nullptr)return false;
        auto settings=generatorSettings(id);settings.transpose=int(p.value("transpose"));settings.velocity=int(p.value("velocity"));settings.outputChannel=p.index("output");
        if(settings==generatorSettings(id) && int(target->state.getProperty("orbitMidiChannel",0))==p.index("input"))return true;
        rememberUndo("Change generator settings");auto* plugin=generatorMidiPlugin(id,true);if(plugin==nullptr)return false;
        target->state.setProperty("orbitMidiChannel",p.index("input"),nullptr);plugin->setSettings(settings);refreshGeneratorSettings(id);
        status.setText("Generator MIDI settings applied. Save to keep the settings.",juce::dontSendNotification);return true;
    },true);
}
void renameGeneratorOrPattern(const juce::String& id,int slot,bool pattern)
{
    if(engineTrackForProjectId(id)==nullptr)return;
    const auto index=getGeneratorTracks().indexOf(engineTrackForProjectId(id));
    auto panel=std::make_unique<OptionsPanel>(pattern?"Name the current pattern. Its bank and slot stay the same.":"Name this generator in its header, Playlist and Mixer.");
    panel->text("name","Name",pattern?project.getPatternName(id,slot):project.getTrackName(index));
    openOptionsTool(pattern?"Rename Current Pattern":"Rename Generator",std::move(panel),[this,id,slot,pattern](const auto& p)
    {
        auto* track=engineTrackForProjectId(id);if(track==nullptr)return false;
        if(p.string("name").isEmpty() || p.string("name").length()>128){status.setText("Enter a name of 1-128 characters.",juce::dontSendNotification);return false;}
        rememberUndo(pattern?"Rename pattern":"Rename generator");
        if(pattern)project.setPatternName(id,slot,p.string("name"));
        else{project.setTrackName(getGeneratorTracks().indexOf(track),p.string("name"));track->setName(p.string("name"));}
        refreshGeneratorSettings(id);status.setText(pattern?"Pattern renamed.":"Generator renamed.",juce::dontSendNotification);return true;
    },true);
}
te::Plugin* generatorInstrument(te::AudioTrack* track)const
{
    if(track!=nullptr)for(auto* p:track->pluginList)
        if(dynamic_cast<te::FourOscPlugin*>(p)!=nullptr || dynamic_cast<te::SamplerPlugin*>(p)!=nullptr || (dynamic_cast<te::ExternalPlugin*>(p)!=nullptr && p->isSynth()))return p;
    return nullptr;
}
std::vector<std::pair<juce::String,juce::String>> generatorControllerTargets(const juce::String& id)
{
    const auto prefix="generator\n"+id+"\n";
    std::vector<std::pair<juce::String,juce::String>> result{{"Volume",prefix+"volume"},{"Pan",prefix+"pan"}};
    if(auto* plugin=generatorInstrument(engineTrackForProjectId(id)))for(auto* parameter:plugin->getAutomatableParameters())
        result.push_back({parameter->getParameterName(),prefix+"parameter\n"+parameter->paramID});return result;
}
void applyGeneratorController(const juce::String& target,float value)
{
    const auto parts=juce::StringArray::fromLines(target);if(parts.size()<3)return;
    auto* track=engineTrackForProjectId(parts[1]);if(track==nullptr)return;
    auto* plugin=generatorInstrument(track);te::AutomatableParameter* parameter=nullptr;
    if(parts[2]=="parameter" && parts.size()==4 && plugin!=nullptr)for(auto* p:plugin->getAutomatableParameters())if(p->paramID==parts[3])parameter=p;
    if(parameter==nullptr && parts[2]!="volume" && parts[2]!="pan")return;
    const auto now=juce::Time::getMillisecondCounter();
    if(target!=lastControllerUndoTarget || now-lastControllerUndoMs>500){rememberUndo("Generator MIDI controller");lastControllerUndoTarget=target;}lastControllerUndoMs=now;
    if(parameter!=nullptr)parameter->setNormalisedParameter(value,juce::sendNotificationSync);
    else if(parts[2]=="volume")setTrackGainDb(getMixerTracks().indexOf(track),-60+66*value);
    else setTrackPan(getMixerTracks().indexOf(track),value*2-1);
    isDirty=true;
}
bool setGeneratorMidiSource(const juce::String& destination,const juce::String& source)
{
    auto* target=engineTrackForProjectId(destination);auto* origin=engineTrackForProjectId(source);
    if(target==nullptr || (source.isNotEmpty() && origin==nullptr) || destination==source)return false;
    std::set<juce::String> visited{destination};auto next=source;
    while(next.isNotEmpty())
    {
        if(!visited.insert(next).second){status.setText("MIDI routing would create a feedback loop.",juce::dontSendNotification);return false;}
        auto* track=engineTrackForProjectId(next);next=track!=nullptr?track->state.getProperty("orbitMidiSource").toString():juce::String{};
    }
    if(target->state.getProperty("orbitMidiSource").toString()==source)return true;
    auto before=captureProjectSnapshot();releaseLiveMidi();
    const auto fail=[&](const juce::String& message)
    {restoreProjectSnapshot(before,message);return false;};
    if(origin!=nullptr)
    {
        auto& input=origin->getMidiInputDevice();input.setEnabled(true);input.setMonitorMode(te::InputDevice::MonitorMode::on);
        edit->getEditInputDevices().getInstanceStateForInputDevice(input);
        edit->getTransport().ensureContextAllocated();
        auto* context=edit->getCurrentPlaybackContext();
        auto* instance=edit->getCurrentInstanceForInputDevice(&input);
        if(instance==nullptr && context!=nullptr){context->addMidiInputDeviceInstance(input);instance=edit->getCurrentInstanceForInputDevice(&input);}
        if(instance==nullptr)return fail("Could not prepare this generator's MIDI route.");
        for(auto destinationID:instance->getTargets())if(destinationID!=target->itemID)
            return fail("This source already feeds another generator. Disconnect that route first.");
        const auto result=instance->setTarget(target->itemID,false,nullptr);
        if(!result)return fail(result.error());
    }
    const auto previous=target->state.getProperty("orbitMidiSource").toString();
    if(auto* old=engineTrackForProjectId(previous))if(auto* instance=edit->getCurrentInstanceForInputDevice(&old->getMidiInputDevice()))
    {
        const auto result=instance->removeTarget(target->itemID,nullptr);
        if(result.failed())return fail(result.getErrorMessage());
    }
    rememberUndoSnapshot(std::move(before),"Change generator MIDI source");
    target->state.setProperty("orbitMidiSource",source,nullptr);isDirty=true;edit->restartPlayback();
    status.setText(source.isEmpty()?"Generator MIDI receive disconnected.":"Generator MIDI receive connected. Save to keep the route.",juce::dontSendNotification);return true;
}
juce::PopupMenu makeGeneratorMenu(const juce::String& id)
{
    juce::PopupMenu menu,controllers,sources;const bool valid=engineTrackForProjectId(id)!=nullptr;
    menu.addItem(1,"Rename Generator...",valid);menu.addItem(2,"Generator Settings...",valid);menu.addSeparator();
    menu.addItem(3,"Rename Current Pattern...",valid);menu.addItem(4,"Arpeggiator/Shuffle...",valid);menu.addSeparator();
    const auto targets=generatorControllerTargets(id);for(size_t i=0;i<targets.size();++i)controllers.addItem(1000+int(i),targets[i].first,true,generatorLearnTarget==targets[i].second);
    controllers.addSeparator();controllers.addItem(1999,"Cancel controller link",generatorLearnTarget.isNotEmpty());
    menu.addSubMenu("Link To Next MIDI Controller",controllers,valid);
    const auto current=valid?engineTrackForProjectId(id)->state.getProperty("orbitMidiSource").toString():juce::String{};
    sources.addItem(2000,"None",true,current.isEmpty());
    for(int i=0;i<project.getTrackCount();++i)if(project.getTrackId(i)!=id && project.getTrackKind(i)!=ProjectDocument::TrackKind::audio)
        sources.addItem(2001+i,project.getTrackName(i),true,current==project.getTrackId(i));
    menu.addSubMenu("Receive MIDI From Generator",sources,valid);return menu;
}
void handleGeneratorMenuResult(const juce::String& id,int slot,int result,const std::vector<std::pair<juce::String,juce::String>>& targets)
{
    if(engineTrackForProjectId(id)==nullptr || result==0)return;
    if(result==1)renameGeneratorOrPattern(id,slot,false);else if(result==2)showGeneratorProperties(id);
    else if(result==3)renameGeneratorOrPattern(id,slot,true);else if(result==4)showGeneratorArpeggiator(id,slot);
    else if(result==1999)generatorLearnTarget.clear();
    else if(result>=1000 && result<1000+int(targets.size()))
    {generatorLearnTarget=targets[size_t(result-1000)].second;learnControllerPanel=nullptr;status.setText("Move a controller on an enabled MIDI input to link "+targets[size_t(result-1000)].first+" on this generator.",juce::dontSendNotification);}
    else if(result==2000)setGeneratorMidiSource(id,{});
    else if(result>2000 && result<=2000+project.getTrackCount())setGeneratorMidiSource(id,project.getTrackId(result-2001));
}
void presentGeneratorMenu(juce::PopupMenu menu,juce::Component* anchor,std::function<void(int)> result)
{
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),std::move(result));
}
void showGeneratorMenuFor(const juce::String& id,juce::Component* anchor)
{
    if(engineTrackForProjectId(id)==nullptr)return;
    const auto slot=id==selectedProjectTrackId()?selectedPatternIndex():project.getTrackSelectedPatternIndex(id);
    const auto targets=generatorControllerTargets(id);
    const juce::Component::SafePointer<ProjectView> owner(this);
    presentGeneratorMenu(makeGeneratorMenu(id),anchor,[owner,id,slot,targets](int result)
    {if(owner!=nullptr)owner->handleGeneratorMenuResult(id,slot,result,targets);});
}
void showGeneratorMenu()
{
    showGeneratorMenuFor(selectedProjectTrackId(),&generatorSettingsButton);
}
