// Native Freeze integration: real renderer, edit projections and Undo snapshots.
// Isolated prefs/files; no physical audio or user project is touched.
static void freezeCheck(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static juce::Array<ProjectDocument::AutomationPoint> freezePoints(std::initializer_list<std::pair<double,float>> values)
{juce::Array<ProjectDocument::AutomationPoint> result;for(const auto& [position,value]:values)result.add(ProjectDocument::AutomationPoint(position,value));return result;}
static void freezeRender(te::Renderer::Parameters p)
{
    te::Renderer::RenderTask task("Freeze validation",p,nullptr,nullptr);int n=0;
    while(task.runJob()==juce::ThreadPoolJob::jobNeedsRunningAgain && ++n<100000){}
    freezeCheck(n<100000 && task.errorMessage.isEmpty(),task.errorMessage.toRawUTF8());
}
static juce::AudioBuffer<float> freezeRead(const juce::File& file)
{
    juce::WavAudioFormat format;std::unique_ptr<juce::AudioFormatReader> r(format.createReaderFor(new juce::FileInputStream(file),true));freezeCheck(r && r->numChannels==2,"unreadable stereo WAV");
    juce::AudioBuffer<float> audio(2,int(r->lengthInSamples));r->read(&audio,0,audio.getNumSamples(),0,true,true);return audio;
}
static juce::AudioBuffer<float> freezeMix(ProjectView& host,const juce::String& name,double duration)
{
    host.edit->flushState();auto clone=te::loadEditFromState(host.engine,host.edit->state.createCopy(),te::Edit::forRendering);host.configureFreezeAudioInEdit(*clone);clone->dispatchPendingUpdatesSynchronously();
    juce::WavAudioFormat wav;te::Renderer::Parameters p(*clone);p.destFile=juce::File("OPTIONS_CHECK_FOLDER/"+name+".wav");p.destFile.deleteFile();p.audioFormat=&wav;p.sampleRateForAudio=48000;p.bitDepth=32;p.canRenderInMono=false;p.useMasterPlugins=false;
    p.time={tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(duration)};freezeRender(p);return freezeRead(p.destFile);
}
static void freezeCompare(const juce::AudioBuffer<float>& a,const juce::AudioBuffer<float>& b,const char* why)
{
    freezeCheck(a.getNumSamples()==b.getNumSamples(),"print changed the rendered duration");double diff=0,power=0;float peak=0;
    for(int c=0;c<2;++c)for(int n=0;n<a.getNumSamples();++n){const double x=a.getSample(c,n),y=b.getSample(c,n);diff+=(x-y)*(x-y);power+=x*x;peak=juce::jmax(peak,float(std::abs(x-y)));}
    std::cout<<why<<": relative error="<<std::sqrt(diff/juce::jmax(1.e-20,power))<<", peak="<<peak<<"\n";
    freezeCheck(power>.001 && std::sqrt(diff/power)<.0005,why);
}
static int freezeWaveCount(ProjectView& host,const juce::String& id)
{
    int n=0;for(auto* clip:host.engineTrackForProjectId(id)->getClips())if(bool(clip->state.getProperty("orbitFreezeProjection",false)))++n;return n;
}
int main()
{
 try
 {
    juce::ScopedJuceInitialiser_GUI gui;ProjectView host;host.stopTimer();host.setSize(1440,1000);
    auto& dm=host.engine.getDeviceManager();auto& device=dm.getHostedAudioDeviceInterface();te::HostedAudioDeviceInterface::Parameters hw;hw.sampleRate=48000;hw.inputChannels=0;hw.outputChannels=2;device.initialise(hw);device.prepareToPlay(48000,512);dm.dispatchPendingUpdates();dm.setCpuLimitBeforeMuting(1000);
    host.createProjectDiscardingCurrent();host.addInstrumentTrack(true);const auto id=host.selectedProjectTrackId();host.createAcidDemo();host.closePianoRollView();
    const auto songFile=juce::File("OPTIONS_CHECK_FOLDER/freeze.orbit");host.saveToFile(songFile);
    juce::String error;freezeCheck(!host.prepareFreeze(id,true,.5,error) && error.contains("Arrange"),"empty arrangement accepted");
    host.project.setPlaylistClip(id,host.project.getPatternId(id,0),1,1,true);host.setPlaybackMode(ProjectView::PlaybackMode::song);host.refreshEnginePlaylist();
    auto* acid=dynamic_cast<orbit::synth::AcidBassPlugin*>(host.generatorControlPlugin(id));freezeCheck(acid,"missing source synth");
    acid->parameters[orbit::synth::AcidBassPlugin::phaseRetriggerIndex]->setParameter(1,juce::sendNotificationSync);
    // Internal distortion is printed; the mixer EQ, fader and pan must stay live.
    acid->parameters[10]->setParameter(35,juce::sendNotificationSync);host.setTrackGainDb(0,-6);host.setTrackPan(0,.3);
    auto* eq=host.ensureTrackEqualiser(0);freezeCheck(eq,"missing live mixer EQ");
    host.project.replacePatternAutomationPoints(id,0,acid->getName(),acid->parameters[2]->paramID,acid->parameters[2]->getParameterName(),freezePoints({{0,.15f},{8,.8f},{15,.25f}}));
    host.project.replaceSongAutomationPoints(id,acid->getName(),acid->parameters[1]->paramID,acid->parameters[1]->getParameterName(),freezePoints({{0,.35f},{16,.6f},{32,.8f}}),acid->itemID.toString());host.refreshEnginePlaylist();
    const auto notes=host.project.getPatternNotes(id,0);const auto placements=host.project.getPlaylistClips(id);
    const auto undoBefore=host.undoHistory.size();auto job=host.prepareFreeze(id,true,.5,error);freezeCheck(job!=nullptr,error.toRawUTF8());
    freezeCheck(!host.isGeneratorFrozen(id) && acid->isEnabled() && host.undoHistory.size()==undoBefore,"preparation mutated the live source or Undo");
    const auto cache=job->file;const auto duration=job->duration;freezeRender(host.freezeRenderParameters(*job));const auto print=freezeRead(cache);
    freezeCheck(print.getMagnitude(0,print.getNumSamples())>.001,"generator print is silent");freezeCheck(print.getMagnitude(0,48000)<1.e-8,"leading Song silence was trimmed");
    const auto before=freezeMix(host,"live-before-freeze",duration);freezeCheck(host.commitFreeze(*job,error),error.toRawUTF8());job.reset();
    freezeCheck(host.isGeneratorFrozen(id) && freezeWaveCount(host,id)==1 && host.undoHistory.size()==undoBefore+1,"freeze did not create one marked projection/Undo");
    for(auto* p:host.freezeSourcePlugins(id))freezeCheck(!p->isEnabled(),"Song still processes source DSP");
    freezeCheck(host.engineTrackForProjectId(id)->getVolumePlugin()->isEnabled() && eq->isEnabled(),"Freeze bypassed live mixer processing");
    freezeCompare(before,freezeMix(host,"frozen-live-mixer",duration),"Song print + live mixer differs from original");
    host.refreshEnginePlaylist();freezeCheck(freezeWaveCount(host,id)==1,"refresh duplicated the print");
    // Sound/notes/arrangement are locked, selection and mixer remain usable.
    freezeCheck(!host.canEditPattern() && !host.requireUnfrozenGenerator(id),"frozen source remains editable");
    host.paintPlaylistClip(0,5);host.resizePlaylistPatternClip(0,1,3);host.deletePlaylistClip(0,1);
    freezeCheck(host.project.getPlaylistClips(id).size()==placements.size() && host.project.getPlaylistClips(id)[0].lengthBars==1,"playlist edit changed frozen arrangement");
    host.auditionPatternPlayback=true;host.playbackMode=ProjectView::PlaybackMode::pattern;host.refreshEnginePlaylist();
    freezeCheck(freezeWaveCount(host,id)==1 && host.audioClipForID(id,host.selectablePlaylistItems()[0].key.id)->isMuted(),"Song audio leaked into Pattern audition");for(auto* p:host.freezeSourcePlugins(id))freezeCheck(p->isEnabled(),"live Pattern audition did not enable source");
    freezeCheck(!host.engineTrackForProjectId(id)->getClips().isEmpty(),"Pattern MIDI was not projected");
    auto patternAudio=freezeMix(host,"live-pattern-audition",1);freezeCheck(patternAudio.getMagnitude(0,patternAudio.getNumSamples())>.001,"live Pattern audition is silent");
    host.auditionPatternPlayback=false;host.playbackMode=ProjectView::PlaybackMode::song;host.refreshEnginePlaylist();
    for(auto* p:host.freezeSourcePlugins(id))freezeCheck(!p->isEnabled(),"return to Song left source DSP enabled");
    host.setTrackGainDb(0,-12);host.setTrackPan(0,-.4);const auto mixed=freezeMix(host,"frozen-mixer-changed",duration);
    freezeCheck(host.unfreezeGenerator(id),"Unfreeze failed");freezeCheck(!host.isGeneratorFrozen(id) && freezeWaveCount(host,id)==0 && cache.existsAsFile(),"Unfreeze lost source/cache");
    freezeCheck(std::abs(host.engineTrackForProjectId(id)->getVolumePlugin()->getVolumeDb()+12)<.01,"Unfreeze discarded live mixer gain");
    freezeCompare(mixed,freezeMix(host,"unfrozen-mixer-changed",duration),"Unfreeze changed source or retained mixer processing");
    freezeCheck(host.project.getPatternNotes(id,0).size()==notes.size(),"Unfreeze lost pattern notes");
    host.undoProjectAction();freezeCheck(host.isGeneratorFrozen(id) && freezeWaveCount(host,id)==1,"Undo Unfreeze failed");host.redoProjectAction();freezeCheck(!host.isGeneratorFrozen(id),"Redo Unfreeze failed");
    host.undoProjectAction();host.saveToFile(songFile);freezeCheck(host.loadProjectFile(songFile),"frozen project reopen failed");freezeCheck(host.isGeneratorFrozen(id) && freezeWaveCount(host,id)==1,"save/reopen lost freeze");
    freezeCompare(mixed,freezeMix(host,"reopened-freeze",duration),"reopened frozen audio differs");
    host.freezeState(id).setProperty("keepLivePattern",false,nullptr);host.auditionPatternPlayback=true;host.playbackMode=ProjectView::PlaybackMode::pattern;host.refreshEnginePlaylist();
    freezeCheck(freezeWaveCount(host,id)==1 && host.engineTrackForProjectId(id)->getClips().size()==1 && host.audioClipForID(id,host.selectablePlaylistItems()[0].key.id)->isMuted(),"disabled Pattern audition leaked MIDI/audio");for(auto* p:host.freezeSourcePlugins(id))freezeCheck(!p->isEnabled(),"disabled Pattern audition ran DSP");
    host.auditionPatternPlayback=false;host.playbackMode=ProjectView::PlaybackMode::song;host.refreshEnginePlaylist();
    const auto renamed=cache.getSiblingFile("temporarily-missing.wav");freezeCheck(cache.moveFileTo(renamed),"could not stage missing-cache fixture");host.refreshEnginePlaylist();
    freezeCheck(!host.isGeneratorFrozen(id) && host.isDirty && host.status.getText().contains("restored"),"missing cache did not recover original with warning");for(auto* p:host.freezeSourcePlugins(id))freezeCheck(p->isEnabled(),"missing cache left source disabled");freezeCheck(renamed.moveFileTo(cache),"could not restore cache fixture");
    // Partial, stale and failed renders are atomic: no freeze and no Undo.
    job=host.prepareFreeze(id,false,.5,error);freezeCheck(job!=nullptr,error.toRawUTF8());const auto beforeFailure=host.undoHistory.size();freezeCheck(!host.commitFreeze(*job,error) && !host.isGeneratorFrozen(id) && host.undoHistory.size()==beforeFailure,"missing render partially froze the generator");
    freezeRender(host.freezeRenderParameters(*job));host.project.appendPatternNoteTicks(id,0,60,64,32,80);freezeCheck(!host.commitFreeze(*job,error) && host.undoHistory.size()==beforeFailure,"stale render committed over edited source");job.reset();
    // Preserve a pre-existing source bypass flag through freeze, Pattern and restore.
    host.generatorControlPlugin(id)->setEnabled(false);job=host.prepareFreeze(id,false,.5,error);freezeCheck(job!=nullptr,error.toRawUTF8());freezeRender(host.freezeRenderParameters(*job));freezeCheck(host.commitFreeze(*job,error),error.toRawUTF8());job.reset();
    host.unfreezeGenerator(id);freezeCheck(!host.generatorControlPlugin(id)->isEnabled(),"Unfreeze enabled a previously bypassed source");
    host.generatorControlPlugin(id)->setEnabled(true);host.refreshEnginePlaylist();
    host.showFreezeOptions(id);freezeCheck(checkedOptions && checkedOptions->checked("live") && !checkedOptions->checked("mixer") && checkedOptions->index("action")==0,"Freeze menu defaults lost");
    checkedOptions->choices.at("action")->setSelectedItemIndex(1,juce::dontSendNotification);checkedOptions->choices.at("action")->onChange();
    freezeCheck(!checkedOptions->checks.at("live")->isEnabled() && !checkedOptions->checked("live") && checkedOptions->checked("mixer") && checkedOptions->apply.getButtonText().contains("Permanently"),"permanent action did not clearly select destructive settings");checkedApply={};checkedOptions.reset();
    host.addInsert(0,9);host.addInsert(0,10);host.setTrackEqBand(0,1,1500,-5);
    auto inserts=host.getInsertPlugins(0);freezeCheck(inserts.size()==2,"FX bake fixture missing");
    auto* tube=dynamic_cast<orbit::mixing::TubeDistortionPlugin*>(inserts[0]);freezeCheck(tube,"tube fixture missing");tube->parameters[0]->setParameter(25,juce::sendNotificationSync);
    host.project.replaceSongAutomationPoints(id,tube->getName(),tube->parameters[0]->paramID,tube->parameters[0]->getParameterName(),freezePoints({{0,.2f},{16,.65f},{32,.4f}}),tube->itemID.toString());host.updateEngineAutomation();
    inserts[1]->setEnabled(false); // Preserve a bypassed FX, including Pattern audition.
    job=host.prepareFreeze(id,true,.5,error,true);freezeCheck(job!=nullptr,error.toRawUTF8());const auto bakeDuration=job->duration;
    const auto liveFX=freezeMix(host,"bake-live-fx",bakeDuration);freezeRender(host.freezeRenderParameters(*job));freezeCheck(host.commitFreeze(*job,error),error.toRawUTF8());job.reset();
    freezeCheck(!host.getTrackEqualiser(0)->isEnabled() && !host.getInsertPlugins(0)[0]->isEnabled(),"baked FX/EQ still run live");
    freezeCompare(liveFX,freezeMix(host,"bake-cached-fx",bakeDuration),"baked mixer chain differs from live source");
    host.toggleInsert(0,0);host.setTrackEqBand(0,1,1500,12);freezeCheck(!host.getInsertPlugins(0)[0]->isEnabled() && std::abs(host.getTrackEqParameter(0,4)+5)<.01,"baked EQ/FX could still be edited");
    host.auditionPatternPlayback=true;host.playbackMode=ProjectView::PlaybackMode::pattern;host.refreshEnginePlaylist();
    freezeCheck(host.getInsertPlugins(0)[0]->isEnabled() && !host.getInsertPlugins(0)[1]->isEnabled() && host.getTrackEqualiser(0)->isEnabled(),"Pattern audition lost original effect bypass flags");
    host.auditionPatternPlayback=false;host.playbackMode=ProjectView::PlaybackMode::song;host.refreshEnginePlaylist();host.setTrackGainDb(0,-9);
    freezeCheck(host.unfreezeGenerator(id),"baked Unfreeze failed");freezeCheck(host.getInsertPlugins(0)[0]->isEnabled() && !host.getInsertPlugins(0)[1]->isEnabled(),"baked Unfreeze did not restore original bypass flags");
    freezeCheck(std::abs(dynamic_cast<orbit::mixing::TubeDistortionPlugin*>(host.getInsertPlugins(0)[0])->parameters[0]->getCurrentValue()-25)<.01,"baked Unfreeze changed FX parameters");
    // Multi FX rack: independent rack renders in its original order, is omitted
    // from the Song graph, and restores its contents without altering the fader.
    host.removeInsert(0,1);host.removeInsert(0,0);host.addInsert(0,11);auto* rack=dynamic_cast<te::RackInstance*>(host.getInsertPlugins(0)[0]);freezeCheck(rack,"Multi FX rack missing");
    freezeCheck(orbit::mixing::addRackEffect(*rack->type,host.edit->getPluginCache().createNewPlugin(orbit::mixing::TubeDistortionPlugin::xmlTypeName,{})),"rack effect insert failed");
    job=host.prepareFreeze(id,false,.5,error,true);freezeCheck(job!=nullptr,error.toRawUTF8());const auto rackDuration=job->duration;const auto liveRack=freezeMix(host,"rack-live",rackDuration);freezeRender(host.freezeRenderParameters(*job));freezeCheck(host.commitFreeze(*job,error),error.toRawUTF8());job.reset();
    freezeCompare(liveRack,freezeMix(host,"rack-frozen",rackDuration),"baked isolated rack differs");freezeCheck(host.unfreezeGenerator(id),"rack Unfreeze failed");
    rack=dynamic_cast<te::RackInstance*>(host.getInsertPlugins(0)[0]);freezeCheck(rack && rack->type->getPlugins().size()==1,"Unfreeze lost rack contents");
    // A cancellation follows the same completion path as a real cancelled job.
    const auto cancelUndo=host.undoHistory.size();host.freezeJob=host.prepareFreeze(id,false,.5,error,true);freezeCheck(bool(host.freezeJob),error.toRawUTF8());host.cancelFreeze();host.finishFreeze({});
    freezeCheck(!host.freezeJob && !host.isGeneratorFrozen(id) && host.undoHistory.size()==cancelUndo,"cancelled freeze mutated source/Undo");
    // The production pump runs native blocks in bounded message-thread batches.
    // The console has no display for AlertWindow; test the exact pump/finish path
    // without constructing that presentation. Timer delivery is simulated here.
    host.freezeJob=host.prepareFreeze(id,false,.5,error,true);freezeCheck(bool(host.freezeJob),error.toRawUTF8());host.beginFreezeRender();
    const auto started=juce::Time::getMillisecondCounter();
    while(host.freezeJob && juce::Time::getMillisecondCounter()-started<30000)host.processFreezeBatch();
    freezeCheck(!host.freezeJob && !host.freezeTask && !host.freezePump && host.isGeneratorFrozen(id),"batched render completion failed");freezeCheck(host.unfreezeGenerator(id),"batched freeze could not Unfreeze");
    const auto cancelBeforeBatch=host.undoHistory.size();host.freezeJob=host.prepareFreeze(id,false,120,error,true);freezeCheck(bool(host.freezeJob),error.toRawUTF8());host.beginFreezeRender();
    // First batch constructs the graph and streams a partial file; Cancel must
    // tear it down directly without a worker/message-thread join or any commit.
    host.processFreezeBatch();if(host.freezeJob){host.cancelFreeze();host.processFreezeBatch();}
    freezeCheck(!host.freezeJob && !host.isGeneratorFrozen(id) && host.undoHistory.size()==cancelBeforeBatch,"active render cancellation changed source or Undo");
    // Permanent conversion is opt-in; after a complete verified WAV it removes
    // source processors/patterns, drops the isolated rack, and clears both Undo
    // stacks while keeping the rendered audio and live level/routing controls.
    job=host.prepareFreeze(id,true,.5,error,false,true);freezeCheck(job && job->permanent && bool(job->metadata.getProperty("bakeMixer")) && !bool(job->metadata.getProperty("keepLivePattern")),"permanent options did not enforce full source baking");
    const auto permanentDuration=job->duration;const auto original=freezeMix(host,"permanent-before",permanentDuration);const auto permanentFile=job->file;freezeRender(host.freezeRenderParameters(*job));freezeCheck(host.commitFreeze(*job,error),error.toRawUTF8());job.reset();
    freezeCheck(host.project.getTrackKind(0)==ProjectDocument::TrackKind::audio && !host.isGeneratorFrozen(id) && host.generatorControlPlugin(id)==nullptr,"permanent conversion retained synth/freeze source");
    freezeCheck(host.getInsertPlugins(0).isEmpty() && host.getTrackEqualiser(0)==nullptr && host.undoHistory.empty() && host.redoHistory.empty(),"permanent conversion retained baked effects or source Undo");
    freezeCheck(host.project.getPatternNotes(id,0).empty() && host.project.getPlaylistClips(id).empty() && permanentFile.existsAsFile(),"permanent conversion lost audio or kept editable patterns");
    freezeCompare(original,freezeMix(host,"permanent-after",permanentDuration),"permanent audio differs");host.undoProjectAction();freezeCheck(host.generatorControlPlugin(id)==nullptr,"permanent conversion was undoable");
    host.saveToFile(songFile);freezeCheck(host.loadProjectFile(songFile) && host.project.getTrackKind(0)==ProjectDocument::TrackKind::audio && host.generatorControlPlugin(id)==nullptr,"permanent audio did not reopen");
    freezeCompare(original,freezeMix(host,"permanent-reopened",permanentDuration),"reopened permanent audio differs");
    std::cout<<"PASS: Song print/phase alignment/tails, live/baked mixer comparisons, optional Pattern audition, source locks, one-action Undo/Redo, exact bypass and rack restore, persistent media, missing-cache recovery, failed/stale/cancel atomicity, permanent source/FX removal with no Undo, and all-mode project reopen. Native renderer/isolated hosted audio.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
