// Included inside ProjectView. Callbacks resolve IDs afresh after Undo/load.
using SongTarget = orbit::automation::SongTarget;
using SongPoints = orbit::automation::Points;
struct AutomationTakeLane { SongTarget target; int slot=-1; SongPoints points; float baseline=0.5f; };

struct SongModule
{
    juce::String id, name;
    te::PluginList* plugins = nullptr;
    int projectRow = -1;
};
std::vector<SongModule> songEventModules()
{
    std::vector<SongModule> result;
    if (edit == nullptr || !hasProject) return result;
    const auto tracks = getGeneratorTracks();
    for (int row = 0; row < project.getTrackCount() && row < tracks.size(); ++row)
        result.push_back({project.getTrackId(row), project.getTrackName(row), &tracks[row]->pluginList, row});
    for (int bus = 0; bus < 8; ++bus)
    {
        if (auto* track = getFxReturnTrack(bus)) result.push_back({"mixer:fx:" + juce::String(bus), track->getName(), &track->pluginList});
        if (auto* track = getOutputBusTrack(bus)) result.push_back({"mixer:bus:" + juce::String(bus), track->getName(), &track->pluginList});
    }
    result.push_back({"mixer:master", "Master", &edit->getMasterPluginList()});
    return result;
}

juce::Array<te::Plugin*> songModulePlugins(const SongModule& module)
{
    juce::Array<te::Plugin*> result;
    if (module.plugins != nullptr) for (auto* plugin : *module.plugins) if (plugin != nullptr && !bool(plugin->state.getProperty("orbitOutputRouteSend",false)) && !bool(plugin->state.getProperty("orbitRouteDryMute",false))) result.add(plugin);
    // Tracktion's final Master volume is separate from its insert list.
    if (module.id == "mixer:master" && edit != nullptr)
        if (auto* volume = masterVolumePlugin()) result.addIfNotAlreadyThere(volume);
    return result;
}

te::AudioTrack* songEventTrack(const juce::String& id)
{
    const auto tracks = getGeneratorTracks();
    for (int row = 0; row < project.getTrackCount() && row < tracks.size(); ++row)
        if (project.getTrackId(row) == id) return tracks[row];
    return nullptr;
}

te::AutomatableParameter* songEventParameter(const SongTarget& target)
{
    for (const auto& module : songEventModules()) if (module.id == target.trackId)
        for (auto* plugin : songModulePlugins(module))
            if (plugin != nullptr && plugin->itemID.toString() == target.pluginId)
                for (auto* p : plugin->getAutomatableParameters())
                    if (p != nullptr && p->paramID == target.parameterId) return p;
    return nullptr;
}

bool songLegacyTargetIsUnique(const SongTarget& target)
{
    int matches = 0;
    for (const auto& module : songEventModules()) if (module.id == target.trackId)
        for (auto* plugin : songModulePlugins(module))
            if (plugin != nullptr && plugin->getName() == target.pluginName)
                for (auto* p : plugin->getAutomatableParameters())
                    if (p != nullptr && p->paramID == target.parameterId) ++matches;
    return matches == 1;
}

SongPoints songEventPoints(const SongTarget& target)
{
    SongPoints legacy;
    const bool unique = songLegacyTargetIsUnique(target);
    for (int i = 0; i < project.getSongAutomationLaneCount(target.trackId); ++i)
    {
        const auto lane = project.getSongAutomationLane(target.trackId, i);
        if (lane.parameterId != target.parameterId) continue;
        if (lane.pluginId == target.pluginId) return lane.points;
        if (unique && lane.pluginId.isEmpty() && lane.pluginName == target.pluginName) legacy = lane.points;
    }
    return legacy;
}

void commitSongEvents(const juce::String& projectId, const SongTarget& target,
                      const SongPoints& before, const SongPoints& next)
{
    if (automationTakeActive) finishAutomationRecording();
    if (!hasProject || project.getProjectId() != projectId || songEventParameter(target) == nullptr
        || !orbit::automation::samePoints(songEventPoints(target), before))
    {
        refreshSongEventEditor();
        status.setText("Song events changed while editing. The pending gesture was canceled.", juce::dontSendNotification);
        return;
    }
    if (orbit::automation::samePoints(before, next)) return;
    rememberUndo("Edit Song events");
    // An instance-specific lane supersedes legacy name-only data for this target.
    project.replaceSongAutomationPoints(target.trackId, target.pluginName, target.parameterId,
        target.parameterName, next, target.pluginId);
    isDirty = true; updateEngineAutomation();
    if (playlistWindow != nullptr) playlistWindow->repaint();
    refreshSongEventEditor();
    status.setText("Song events updated. Use Song mode to hear the arrangement automation.", juce::dontSendNotification);
}

void refreshSongEventEditor()
{
    if (songEventWindow == nullptr) return;
    auto& panel = songEventWindow->panel;
    const auto old = juce::isPositiveAndBelow(panel.selectedTarget, int(panel.targets.size()))
        ? panel.targets[size_t(panel.selectedTarget)] : SongTarget{};
    juce::StringArray labels;
    const auto modules = songEventModules();
    int selected = -1;
    for (int i = 0; i < int(modules.size()); ++i)
    { labels.add(modules[size_t(i)].name); if (modules[size_t(i)].id == panel.trackId) selected = i; }
    if (selected < 0 && !modules.empty()) selected = 0;
    panel.projectId = hasProject ? project.getProjectId() : juce::String{};
    panel.trackId = selected >= 0 ? modules[size_t(selected)].id : juce::String{};
    panel.setTracks(labels, selected);
    std::vector<SongTarget> targets;
    if (selected >= 0)
    {
        int slot = 0;
        for (auto* plugin : songModulePlugins(modules[size_t(selected)]))
        {
            ++slot;
            if (plugin == nullptr) continue;
            for (auto* p : plugin->getAutomatableParameters()) if (p != nullptr)
            {
                SongTarget target{panel.trackId, plugin->itemID.toString(), plugin->getName(), p->paramID, p->getParameterName(), {}};
                target.label = juce::String(slot) + ": " + target.pluginName + " / " + target.parameterName;
                targets.push_back(std::move(target));
            }
        }
    }
    int targetIndex = targets.empty() ? -1 : 0;
    for (int i = 0; i < int(targets.size()); ++i)
        if (orbit::automation::sameTarget(targets[size_t(i)], old)) targetIndex = i;
    setSongEventTarget(std::move(targets), targetIndex);
}

void setSongEventTarget(std::vector<SongTarget> targets, int index)
{
    auto& panel = songEventWindow->panel;
    const SongTarget target = juce::isPositiveAndBelow(index, int(targets.size())) ? targets[size_t(index)] : SongTarget{};
    const auto projectId = panel.projectId;
    panel.onCommit = [this, projectId, target](const SongPoints& before, const SongPoints& next)
        { commitSongEvents(projectId, target, before, next); };
    panel.formatValue = [this, target](float value)
    {
        if (auto* p = songEventParameter(target))
        {
            const auto native = p->valueRange.convertFrom0to1(value);
            return p->valueToString(native) + (p->getLabel().isEmpty() ? juce::String{} : " " + p->getLabel());
        }
        return juce::String(value, 2);
    };
    const auto meter = project.getInitialTimeSignature();
    panel.setTargets(std::move(targets), index, songEventPoints(target), meter.numerator * 16.0 / juce::jmax(1, meter.denominator));
    juce::String name;
    for (const auto& module : songEventModules()) if (module.id == panel.trackId) name = module.name;
    songEventWindow->setName("Song Event Editor" + (name.isEmpty() ? juce::String{} : " - " + name));
}

void showSongEventEditor(juce::String id = {})
{
    if (!hasProject || edit == nullptr) return;
    if (songEventWindow == nullptr)
    {
        songEventWindow = std::make_unique<orbit::automation::SongEventWindow>();
        addChildComponent(*songEventWindow);songEventWindow->constrainToWorkspace();
        songEventWindow->onWorkspaceChanged=[this]{syncGeneratorNativeViews();};
        auto& panel = songEventWindow->panel;
        panel.setViewOptions(optionBool("songEventFollow", true), optionBool("songEventGradient", false));
        panel.setGradientStyle(optionInt("songEventGradientStyle"));
        panel.onLfo=[this]{showAutomationLfo(false);};
        panel.onSeek=[this](double step){if(edit!=nullptr){edit->getTransport().setPosition(edit->tempoSequence.toTime(tracktion::BeatPosition::fromBeats(step*.25)));syncSongEventTransport();}};
        panel.onLoop=[this](int start,int length){loopStartSlider.setValue(start,juce::dontSendNotification);loopLengthSlider.setValue(length,juce::dontSendNotification);loopButton.setToggleState(true,juce::dontSendNotification);updateLoopRange();};
        panel.onViewOptionsChanged = [this]
        {
            saveOption("songEventFollow", songEventWindow->panel.isFollowing());
            saveOption("songEventGradient", songEventWindow->panel.isGradientFillEnabled());
            saveOption("songEventGradientStyle",songEventWindow->panel.gradientStyle);
        };
        panel.onTrack = [this](int row)
        {
            const auto modules = songEventModules();
            if (!juce::isPositiveAndBelow(row, int(modules.size()))) return;
            songEventWindow->panel.trackId = modules[size_t(row)].id; refreshSongEventEditor();
        };
        panel.onTarget = [this](int index)
        {
            auto targets = songEventWindow->panel.targets;
            setSongEventTarget(std::move(targets), index);
        };
        panel.onRecord = [this]{if(!automationTakeActive)setPlaybackMode(PlaybackMode::song);toggleAutomationRecording();};
        panel.onUndo = [this](bool redo) { if (redo) redoProjectAction(); else undoProjectAction(); };
    }
    if (id.isEmpty()) id = selectedProjectTrackId();
    if (id.isEmpty()) id = "mixer:master";
    saveOption("recordContent",1);recordModeButton.setButtonText("AUTO");
    songEventWindow->panel.trackId = id;
    refreshSongEventEditor();
    syncSongEventTransport();
    songEventWindow->setMinimised(false);songEventWindow->setVisible(true); songEventWindow->toFront(true);syncGeneratorNativeViews();
    songEventWindow->panel.graph.grabKeyboardFocus();
}

void syncSongEventTransport()
{
    if (songEventWindow == nullptr) return;
    if (edit == nullptr || !hasProject)
    { songEventWindow->panel.setTransportState(0, false, false, 0, 0, false); return; }
    auto& transport = edit->getTransport();
    const auto range = transport.getLoopRange();
    songEventWindow->panel.setTransportState(edit->tempoSequence.toBeats(transport.getPosition()).inBeats() * 4,
        playbackMode == PlaybackMode::song, transport.isPlaying(),
        edit->tempoSequence.toBeats(range.getStart()).inBeats() * 4,
        edit->tempoSequence.toBeats(range.getEnd()).inBeats() * 4, transport.looping.get());
}

SongTarget targetForParameter(te::AutomatableParameter& parameter)
{
    for (const auto& module : songEventModules())
        for (auto* plugin : songModulePlugins(module))
            for (auto* candidate : plugin->getAutomatableParameters()) if (candidate == &parameter)
                return {module.id, plugin->itemID.toString(), plugin->getName(), parameter.paramID, parameter.getParameterName(), {}};
    return {};
}
void openParameterEvents(const SongTarget& target, bool pattern)
{
    if (songEventParameter(target) == nullptr) return;
    if (!pattern)
    {
        showSongEventEditor(target.trackId); auto targets = songEventWindow->panel.targets;
        for (int i = 0; i < int(targets.size()); ++i) if (orbit::automation::sameTarget(targets[size_t(i)],target))
        { setSongEventTarget(std::move(targets),i); break; }
    }
    else if (songLegacyTargetIsUnique(target))
    {
        for (int row=0;row<project.getTrackCount();++row) if(project.getTrackId(row)==target.trackId)
        {
            trackSelector.setSelectedId(row+1,juce::sendNotificationSync);showPianoRoll();refreshPianoRollAutomationTargets();
            pianoRollWindow->selectParameterEvents(target.pluginName,target.parameterId);break;
        }
    }
}
void showParameterEventMenu(te::AutomatableParameter& parameter, juce::Component* anchor)
{
    const auto target=targetForParameter(parameter);if(target.parameterId.isEmpty())return;
    const auto projectId=project.getProjectId();
    juce::PopupMenu menu;menu.addSectionHeader(target.parameterName);menu.addItem(1,"Edit Song Events");
    menu.addItem(2,"Edit Pattern Events",songEventTrack(target.trackId)!=nullptr && songLegacyTargetIsUnique(target));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(anchor),
        [safe=juce::Component::SafePointer<ProjectView>(this),target,projectId](int choice)
        {if(safe!=nullptr && safe->project.getProjectId()==projectId && (choice==1 || choice==2))safe->openParameterEvents(target,choice==2);});
}

void discoverAutomationRecordingTargets()
{
    if(!automationTakeActive || edit==nullptr)return;
    std::set<std::tuple<juce::String,juce::String,juce::String>> known;
    for(const auto& t:automationTakeTargets)known.emplace(t.target.trackId,t.target.pluginId,t.target.parameterId);
    for(const auto& module:songEventModules())for(auto* plugin:songModulePlugins(module))
        for(auto* p:plugin->getAutomatableParameters())if(p!=nullptr && !known.contains({module.id,plugin->itemID.toString(),p->paramID}))
        {
            SongTarget target{module.id,plugin->itemID.toString(),plugin->getName(),p->paramID,p->getParameterName(),{}};
            const auto slot=automationTakePatternSlots.find(module.id);
            const int pattern=slot!=automationTakePatternSlots.end() && songLegacyTargetIsUnique(target)?slot->second:-1;
            automationTakeTargets.push_back({target,pattern,{},p->getCurrentNormalisedValue()});
            automationTouchListeners.push_back(std::make_unique<orbit::automation::AutomationTouchListener>(p,[this,target]
                {if(automationTakeActive)automationTouched.emplace(target.trackId,target.pluginId,target.parameterId);}));
        }
}
bool beginAutomationRecording()
{
    if(edit==nullptr || !hasProject || automationTakeActive)return false;
    if(!finishMidiRecording())return false;
    automationTakeSnapshot=captureProjectSnapshot();automationTakeProjectId=project.getProjectId();automationTakeTargets.clear();automationTouchListeners.clear();automationTouched.clear();
    automationTakePatternSlots.clear();
    if(playbackMode==PlaybackMode::pattern)for(int row=0;row<project.getTrackCount();++row)
        automationTakePatternSlots[project.getTrackId(row)]=project.getTrackSelectedPatternIndex(project.getTrackId(row));
    automationTakeActive=true;discoverAutomationRecordingTargets();edit->getAutomationRecordManager().setWritingAutomation(true);
    recordButton.setToggleState(true,juce::dontSendNotification);
    if(songEventWindow!=nullptr)songEventWindow->panel.setRecording(true);
    return true;
}
void cancelAutomationRecording()
{
    automationTakeActive=false;
    if(edit!=nullptr)edit->getAutomationRecordManager().setWritingAutomation(false);
    automationTouchListeners.clear();automationTouched.clear();
    automationTakeTargets.clear();automationTakeSnapshot.reset();
    if(songEventWindow!=nullptr)songEventWindow->panel.setRecording(false);
}
void applyAutomationTake(std::vector<AutomationTakeLane> lanes)
{
    automationTakeActive=false;
    if(edit!=nullptr)edit->getAutomationRecordManager().setWritingAutomation(false);
    recordButton.setToggleState(false,juce::dontSendNotification);
    if(songEventWindow!=nullptr)songEventWindow->panel.setRecording(false);
    if(!automationTakeSnapshot || automationTakeProjectId!=project.getProjectId()) {cancelAutomationRecording();return;}
    auto staged=project;int count=0;
    for(const auto& lane:lanes)
    {
        if(songEventParameter(lane.target)==nullptr || lane.points.isEmpty())continue;
        if(lane.slot<0)staged.replaceSongAutomationPoints(lane.target.trackId,lane.target.pluginName,lane.target.parameterId,
            lane.target.parameterName,lane.points,lane.target.pluginId);
        else staged.replacePatternAutomationPoints(lane.target.trackId,lane.slot,lane.target.pluginName,lane.target.parameterId,
            lane.target.parameterName,lane.points);
        if(auto* p=songEventParameter(lane.target))if(auto plugin=p->getPlugin())
        {
            auto bases=plugin->state.getChildWithName("ORBITAUTOMATIONBASES");
            if(!bases.isValid()){bases=juce::ValueTree("ORBITAUTOMATIONBASES");plugin->state.addChild(bases,-1,nullptr);}
            if(!bases.getChildWithProperty("parameterId",p->paramID).isValid())
            {
                juce::ValueTree base("PARAMETER");base.setProperty("parameterId",p->paramID,nullptr);
                base.setProperty("value",lane.baseline,nullptr);bases.addChild(base,-1,nullptr);
            }
        }
        ++count;
    }
    if(count>0)
    {
        rememberUndoSnapshot(std::move(*automationTakeSnapshot),"Record automation take");project=std::move(staged);isDirty=true;
    }
    automationTouchListeners.clear();automationTouched.clear();
    automationTakeSnapshot.reset();automationTakeTargets.clear();
    readPatternSteps();refreshEnginePlaylist();refreshSongEventEditor();
    status.setText(count>0?"Automation take recorded. Undo restores the whole take.":"Automation recording ended without parameter changes.",juce::dontSendNotification);
}
void finishAutomationRecording()
{
    if(!automationTakeActive || edit==nullptr)return;
    discoverAutomationRecordingTargets();
    auto& recorder=edit->getAutomationRecordManager();std::vector<AutomationTakeLane> touched;
    for(const auto& target:automationTakeTargets)
        if(auto* p=songEventParameter(target.target);p!=nullptr && (recorder.isParameterRecording(p) || automationTouched.contains({target.target.trackId,target.target.pluginId,target.target.parameterId})))touched.push_back(target);
    recorder.punchOut(false);recorder.setWritingAutomation(false);
    for(auto& lane:touched)if(auto* p=songEventParameter(lane.target))
    {
        std::map<double,float> ordered;
        for(int i=0;i<p->getCurve().getNumPoints();++i)
        {
            const auto raw=edit->tempoSequence.toBeats(p->getCurve().getPointTime(i)).inBeats()*4;
            if(!std::isfinite(raw))continue;
            double step=juce::jlimit(0.0,std::nextafter(65536.0,0.0),raw);
            if(lane.slot>=0)
            {
                const auto loopEnd=edit->tempoSequence.toBeats(edit->getTransport().getLoopRange().getEnd()).inBeats()*4;
                if(edit->getTransport().looping.get() && raw>=loopEnd-0.000001)continue;
                step=std::fmod(step,double(juce::jmax(1,project.getPatternLengthSteps(lane.target.trackId,lane.slot))));
            }
            ordered[step]=p->valueRange.convertTo0to1(p->getCurve().getPointValue(i));
        }
        for(auto [position,value]:ordered)lane.points.add({position,value});
    }
    applyAutomationTake(std::move(touched));
}
void toggleAutomationRecording()
{
    if(automationTakeActive){finishAutomationRecording();return;}
    if(edit==nullptr)return;
    if(engine.getDeviceManager().deviceManager.getCurrentAudioDevice()==nullptr)
    {status.setText("Choose an audio output before recording automation.",juce::dontSendNotification);return;}
    if(!beginAutomationRecording())return;
    if(!edit->getTransport().isPlaying())togglePlayback(true);
    if(!edit->getTransport().isPlaying()){cancelAutomationRecording();recordButton.setToggleState(false,juce::dontSendNotification);return;}
    status.setText("Recording automation. Move knobs or assigned MIDI controls; Stop commits one take.",juce::dontSendNotification);
}


void showAutomationLfo(bool pattern)
{
    if(!hasProject || edit==nullptr || pianoRollWindow==nullptr)return;
    if(lfoWindow!=nullptr && lfoWindow->panel.onCancel)lfoWindow->panel.onCancel();
    const auto projectId=project.getProjectId(),id=selectedProjectTrackId();const int slot=selectedPatternIndex();
    const auto pluginName=automationPluginName,parameterId=automationParameterId,parameterName=automationParameterName;
    SongTarget target;SongPoints before;
    if(pattern){if(pluginName.isEmpty() || parameterId.isEmpty())return;}
    else {if(songEventWindow==nullptr)return;auto& p=songEventWindow->panel;if(!juce::isPositiveAndBelow(p.selectedTarget,int(p.targets.size())))return;target=p.targets[size_t(p.selectedTarget)];before=songEventPoints(target);}
    double start=0;int steps=pattern?project.getPatternLengthSteps(id,slot):64;
    if(!pattern){const auto loop=edit->getTransport().getLoopRange();start=edit->tempoSequence.toBeats(loop.getStart()).inBeats()*4;steps=juce::jmax(1,juce::roundToInt((edit->tempoSequence.toBeats(loop.getEnd()).inBeats()*4)-start));}
    const double bpm=project.getInitialTempoBpm();
    lfoWindow=std::make_unique<orbit::automation::LfoWindow>();addChildComponent(*lfoWindow);lfoWindow->constrainToWorkspace();
    auto valid=[this,projectId,pattern,id,slot,pluginName,parameterId,target]{return project.getProjectId()==projectId && (pattern?(selectedProjectTrackId()==id && selectedPatternIndex()==slot && automationPluginName==pluginName && automationParameterId==parameterId):songEventParameter(target)!=nullptr);};
    auto combine=[before,start,steps](SongPoints generated){SongPoints result;for(const auto& p:before)if(p.position<start || p.position>=start+steps)result.add(p);result.addArray(generated);std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.position<b.position;});return result;};
    lfoWindow->panel.onPreview=[this,pattern,start,steps,bpm,valid,combine,target](auto settings)
    {
        if(!valid())return;auto points=orbit::automation::lfoCurve(start,steps,bpm,settings);
        if(pattern){std::vector<std::pair<int,float>> p;for(const auto& point:points)p.emplace_back(int(point.position),point.value);pianoRollWindow->previewPatternLfo(p);}
        else if(songEventWindow!=nullptr && juce::isPositiveAndBelow(songEventWindow->panel.selectedTarget,int(songEventWindow->panel.targets.size())) && orbit::automation::sameTarget(target,songEventWindow->panel.targets[size_t(songEventWindow->panel.selectedTarget)]))songEventWindow->panel.setPreview(combine(std::move(points)),true);
    };
    lfoWindow->panel.onCancel=[this]{if(songEventWindow!=nullptr)songEventWindow->panel.setPreview({},false);if(pianoRollWindow!=nullptr)pianoRollWindow->previewPatternLfo({});if(lfoWindow!=nullptr)lfoWindow->setVisible(false);};
    lfoWindow->panel.onApply=[this,pattern,start,steps,bpm,valid,combine,projectId,target,before](auto settings)
    {
        if(valid())
        {auto points=orbit::automation::lfoCurve(start,steps,bpm,settings);if(pattern){std::vector<std::pair<int,float>> p;for(const auto& point:points)p.emplace_back(int(point.position),point.value);pianoRollWindow->applyPatternLfo(p);}else commitSongEvents(projectId,target,before,combine(std::move(points)));}
        if(lfoWindow!=nullptr && lfoWindow->panel.onCancel)lfoWindow->panel.onCancel();
    };
    lfoWindow->onWorkspaceChanged=[this]{syncGeneratorNativeViews();};lfoWindow->setVisible(true);lfoWindow->toFront(true);lfoWindow->panel.changed();
}
